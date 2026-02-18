/**
 * @file main.c
 * @brief DJI 电机通用集成测试用例
 * @note  功能：驱动总线上所有配置的电机，使其每隔 1 秒转动 90 度 (阶跃响应测试)
 */
#include "drivers/motor/vendors/dji/dji_motor_drv.h"
#include "drivers/peripheral/can/pal_can_dev.h"

/* Framework Includes */
#include "awlib.h"
#include "core/algorithm/controller/pid.h"
#include "core/aw_cpu.h"
#include "osal/osal.h"
#include "osal/osal_time.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "osal/osal_config.h"

/* --- 1. 用户配置区 (USER CONFIGURATION) --- */

/* 任务优先级配置 (数值越大优先级越高，取决于 OSALConfig.h) */
#define TASK_PRIO_CONTROL 7 // 最高优先级
#define TASK_PRIO_LOGIC 4   // 普通优先级

/* 任务堆栈大小（字节，按历史 word 配置显式换算） */
#define TASK_STACK_CONTROL (2048u * OSAL_STACK_WORD_BYTES)
#define TASK_STACK_LOGIC (1024u * OSAL_STACK_WORD_BYTES)

/* 控制参数 */
#define CONTROL_FREQ_HZ 1000  /* 1kHz 控制频率 */
#define STEP_INTERVAL_MS 1000 /* 动作间隔 1000ms */
#define STEP_ANGLE_DEG 90.0f  /* 每次转动角度 */

/* 定义测试电机配置结构体 */
typedef struct
{
    /* 用户配置项 */
    DJIMotorType_e type;     // 电机型号
    uint8_t id;              // 电机 ID
    DJIMotorCtrlMode_e mode; // 控制模式

    /* 位置环 PID 参数 */
    struct
    {
        float kp, ki, kd;
        float max_out; // 输出限幅
    } pos_pid_cfg;

    /* 速度环 PID 参数 */
    struct
    {
        float kp, ki, kd;
        float max_out; // 输出限幅
    } spd_pid_cfg;

    /* --- 运行时对象 (系统自动维护) --- */
    DJIMotorDrv_s handle;
    PidController_s pid_pos;
    PidController_s pid_spd;
    float target_deg;
} MotorTestNode_s;

#define MOTOR_TEST_NODE_INIT(_type, _id, _mode, _pos_pid_cfg, _spd_pid_cfg)                                                                \
    {                                                                                                                                      \
        .type = (_type),                                                                                                                   \
        .id = (_id),                                                                                                                       \
        .mode = (_mode),                                                                                                                   \
        .pos_pid_cfg = (_pos_pid_cfg),                                                                                                     \
        .spd_pid_cfg = (_spd_pid_cfg),                                                                                                     \
    }

/**
 * @brief 待测电机列表 (配置表)
 * @note  在此处添加任意数量的电机
 */
static MotorTestNode_s g_motor_configs[] = {
    /* [Case 1] GM6020 ID:1 (电压模式) */
    {
        .type = DJI_MOTOR_TYPE_GM6020,
        .id = 1,
        .mode = DJI_CTRL_MODE_VOLTAGE,
        .pos_pid_cfg = {32.0f, 1.3f, 0.0f, 320.0f},
        .spd_pid_cfg = {50.0f, 500.0f, 0.0f, 25000.0f},
    },
    /* [Case 3] GM6020 ID:2 (电压模式) */
    // {
    //     .type = DJI_MOTOR_TYPE_GM6020,
    //     .id = 2,
    //     .mode = DJI_CTRL_MODE_VOLTAGE,
    //     .pos_pid_cfg = {32.0f, 1.3f, 0.0f, 320.0f},
    //     .spd_pid_cfg = {50.0f, 500.0f, 0.0f, 25000.0f},
    // },
    // /* [Case 4] GM6020 ID:3 (电压模式) */
    // {
    //     .type = DJI_MOTOR_TYPE_GM6020,
    //     .id = 3,
    //     .mode = DJI_CTRL_MODE_VOLTAGE,
    //     .pos_pid_cfg = {32.0f, 1.3f, 0.0f, 320.0f},
    //     .spd_pid_cfg = {50.0f, 500.0f, 0.0f, 25000.0f},
    // },
};

#define TEST_MOTOR_CNT (sizeof(g_motor_configs) / sizeof(MotorTestNode_s))

/* --- 2. 全局对象 --- */

static DJIMotorBus_s g_can1_bus;
static Device_t g_can1_device_handle;

/* 任务句柄 */
osal_thread_t g_control_task_handler;
osal_thread_t g_logic_task_handler;

/* --- 3. 辅助函数 --- */

/**
 * @brief 获取系统时间 (秒)
 * @note  基于 OSAL Tick
 */
static uint32_t get_time_ms(void)
{
    return (uint32_t)osal_time_now_monotonic();
}

static void sleep_until_ms(osal_time_ms_t* last_ms, uint32_t period_ms)
{
    if (!last_ms)
        return;
    if (*last_ms == 0U)
        *last_ms = get_time_ms();
    (void)osal_delay_until(last_ms, period_ms, NULL);
}

static float get_time_s(void)
{
    return (float)get_time_ms() / 1000.0f;
}

/**
 * @brief 初始化单个电机的 PID 和驱动
 */
static void motor_node_init(MotorTestNode_s* node)
{
    /* 1. 注册电机 */
    dji_motor_register(&g_can1_bus, &node->handle, node->type, node->id, node->mode);

    /* 2. 初始化位置环 PID */
    pid_init(&node->pid_pos, PID_POSITIONAL_MODE, node->pos_pid_cfg.kp, node->pos_pid_cfg.ki, node->pos_pid_cfg.kd);
    pid_set_output_limit(&node->pid_pos, -node->pos_pid_cfg.max_out, node->pos_pid_cfg.max_out);
    // 自动置位功能使能(依赖 PID 库实现，若库未自动置位需手动设置 settings)

    /* 3. 初始化速度环 PID */
    pid_init(&node->pid_spd, PID_POSITIONAL_MODE, node->spd_pid_cfg.kp, node->spd_pid_cfg.ki, node->spd_pid_cfg.kd);
    pid_set_output_limit(&node->pid_spd, -node->spd_pid_cfg.max_out, node->spd_pid_cfg.max_out);

    /* 4. 初始目标设为当前角度，防止上电飞车 */
    node->target_deg = 0.0f;
}

/* --- 4. 任务函数 --- */

/**
 * @brief 逻辑任务 (1Hz)
 * @note  负责更新目标角度
 */
void logic_task_func(void* pvParameters)
{
    uint32_t period_ms = STEP_INTERVAL_MS;
    osal_time_ms_t last_ms = get_time_ms();
    (void)pvParameters;

    /* 延时 1s 等待系统稳定 */
    osal_sleep_ms(1000);

    /* 读取当前电机角度作为初始目标(可选) */
    for (int i = 0; i < TEST_MOTOR_CNT; i++)
    {
        g_motor_configs[i].target_deg = dji_motor_get_total_angle(&g_motor_configs[i].handle);
    }

    for (;;)
    {
        /* 更新所有电机的目标角度 */
        for (int i = 0; i < TEST_MOTOR_CNT; i++)
        {
            g_motor_configs[i].target_deg += STEP_ANGLE_DEG;
        }
        sleep_until_ms(&last_ms, period_ms);
    }
}

/**
 * @brief 控制任务 (1kHz)
 * @note  负责 PID 计算和 CAN 发送
 */
void control_task_func(void* pvParameters)
{
    uint32_t period_ms = 1000 / CONTROL_FREQ_HZ; // 1ms
    osal_time_ms_t last_ms = get_time_ms();
    (void)pvParameters;
    g_can1_device_handle = device_find("can1");
    device_open(g_can1_device_handle, CAN_O_INT_RX | CAN_O_INT_TX);
    /* 2. 驱动层初始化 */
    dji_motor_bus_init(&g_can1_bus, g_can1_device_handle);
    device_ctrl(g_can1_device_handle, CAN_CMD_START, NULL);

    /* 3. 初始化配置表中的所有电机 */
    for (int i = 0; i < TEST_MOTOR_CNT; i++)
    {
        motor_node_init(&g_motor_configs[i]);
    }
    for (;;)
    {
        /* tick handled by OSAL */
        /* 绝对延时，确保 1kHz 频率稳定 */
        float current_time = get_time_s();
        /* 1. 遍历计算 PID */
        for (int i = 0; i < TEST_MOTOR_CNT; i++)
        {
            MotorTestNode_s* node = &g_motor_configs[i];

            /* 获取反馈 */
            float now_deg = dji_motor_get_total_angle(&node->handle);
            float now_spd = dji_motor_get_velocity(&node->handle);

            /* 位置环计算 */
            float tgt_spd = pid_compute(&node->pid_pos, node->target_deg, now_deg, current_time);

            /* 速度环计算 */
            float output = pid_compute(&node->pid_spd, tgt_spd, now_spd, current_time);

            /* 写入驱动缓存 */
            dji_motor_set_output(&node->handle, (int16_t)output);
        }

        /* 2. 同步发送 CAN 报文 */
        dji_motor_bus_sync(&g_can1_bus);
        sleep_until_ms(&last_ms, period_ms);
    }
}

/* --- 5. 主函数 --- */

/**
 * @brief 硬件初始化与任务创建
 */
int main(void)
{
    aw_board_init();
    aw_core_init();
    /* 4. 创建 OSAL 任务 */
    osal_thread_attr_t control_attr = {0};
    osal_thread_attr_t logic_attr = {0};
    control_attr.name = "ControlTask";
    logic_attr.name = "LogicTask";
    control_attr.stack_size = TASK_STACK_CONTROL;
    logic_attr.stack_size = TASK_STACK_LOGIC;
    control_attr.priority = TASK_PRIO_CONTROL;
    logic_attr.priority = TASK_PRIO_LOGIC;
    int ret = osal_thread_create(&g_control_task_handler, &control_attr, control_task_func, NULL);
    while (ret != OSAL_OK)
    {
    }
    ret = osal_thread_create(&g_logic_task_handler, &logic_attr, logic_task_func, NULL);
    while (ret != OSAL_OK)
    {
    };

    osal_kernel_start();
    /* 正常情况不会运行到这里 */
    for (;;)
        ;
    return 0;
}
