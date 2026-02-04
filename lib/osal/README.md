
# OSAL 总览

本目录提供操作系统抽象层（OSAL）。上层只依赖 OSAL 接口，具体 RTOS/系统由端口实现适配。

## 目录结构
- `include/osal`：OSAL 核心接口（线程、时间、事件、互斥锁、信号量、队列、定时器）
- `platform/osal/freertos`：FreeRTOS 端口实现（MCU 优先）
- `platform/osal/posix`：POSIX 端口实现（Linux 用户态）

## OSAL 接口层
`include/osal` 下的头文件是对外唯一接口：
- `osal_core.h`：中断判断、临界区、内存申请释放、通用返回码
- `osal_thread.h`：线程创建/休眠/退出/删除/让出
- `osal_time.h`：时间获取（ms/us/ns）与周期延时
- `osal_event.h`：事件对象（event group，支持 ISR set）
- `osal_timer.h`：软件定时器
- `osal_mutex.h`：互斥锁（线程上下文）
- `osal_sem.h`：信号量（ISR 支持 post）
- `osal_queue.h`：队列（ISR 支持收发）

## 端口层说明
- `platform/osal/freertos`：基于 FreeRTOS 的实现
- `platform/osal/posix`：基于 POSIX 系统调用的实现（用于 Linux）

## 时间单位约定
- OSAL 公共接口默认使用 **毫秒**。
- FreeRTOS 端口将毫秒转换为 tick（向上取整），再依据 `configTICK_RATE_HZ` 返回毫秒。
- POSIX 端口使用 `clock_gettime(CLOCK_MONOTONIC)` 获取高精度时间。

## ISR 使用规则
- ISR 中禁止调用阻塞接口。
- 仅使用 ISR 版本的 API：
  - `osal_event_set_isr`
  - `osal_sem_post_isr`
  - `osal_queue_send_isr`
  - `osal_queue_recv_isr`

## 配置项
主要配置位于 `lib/include/core/aw_config.h` 与 `lib/osal/include/osal/osal_config.h`：
- `OSAL_WAIT_FOREVER`：无限等待超时值
- `OSAL_STACK_WORD_BYTES`：栈单位大小（字节）
- `OSAL_PRIORITY_MAX`：最大优先级数量
- `OSAL_TASK_NAME_MAX`：任务名称最大长度
- `OSAL_QUEUE_REGISTRY_MAX`：队列注册表最大数量

## 构建开关
- `AWLF_OSAL_PORT`：选择 OSAL 后端（`freertos` 或 `posix`）

## 测试策略
当前阶段（架构早期、跨平台构建刚搭起）：优先做“接口一致性验证”。行为测试可等 BSP 与样例稳定后，再在板级或 HIL 环境引入。

## 典型使用流程
1. 初始化系统。
2. 创建 OSAL 资源（线程、队列、互斥锁、信号量、定时器等）。
3. 启动调度器（``osal_kernel_start()`）。
4. ISR 中仅调用 ISR 版本接口，避免阻塞。

示例：
```c
#include "osal/osal.h"

static void app_thread(void* arg)
{
    (void)arg;
    while (1)
    {
        osal_thread_sleep_ms(10);
    }
}

void app_start(void)
{
    osal_thread_t thread;
    osal_thread_attr_t attr = { "app", 512, 2 };
    (void)osal_thread_create(&thread, &attr, app_thread, NULL);
    (void)osal_kernel_start();
}
```
