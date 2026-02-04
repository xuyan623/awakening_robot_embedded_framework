#ifndef __AWLF_OSAL_THREAD_H__
#define __AWLF_OSAL_THREAD_H__

#include <stdint.h>

#include "osal_core.h"

typedef void* osal_thread_t;
typedef void (*osal_thread_entry_t)(void* arg);

typedef struct
{
    const char* name;    /* 线程名称，用于调试 */
    uint32_t stack_size; /* 栈大小（字节或字，视端口而定） */
    uint32_t priority;   /* 线程优先级 */
} osal_thread_attr_t;

/**
 * @brief 创建线程
 * @param thread 输出线程句柄
 * @param attr 线程属性
 * @param entry 线程入口函数
 * @param arg 入口参数
 * @return OSAL_OK 成功；OSAL_ERR 失败
 */
int osal_thread_create(osal_thread_t* thread, const osal_thread_attr_t* attr, osal_thread_entry_t entry, void* arg);

/**
 * @brief 获取当前线程句柄
 */
osal_thread_t osal_thread_self(void);

/**
 * @brief 线程休眠指定毫秒数
 * @param ms 休眠时长（ms）
 */
void osal_thread_sleep_ms(uint32_t ms);

/**
 * @brief 主动让出CPU
 */
void osal_thread_yield(void);

/**
 * @brief 退出当前线程
 */
void osal_thread_exit(void);

/**
 * @brief 删除指定线程
 * @param thread 线程句柄
 * @return OSAL_OK 成功；OSAL_ERR 失败
 */
int osal_thread_delete(osal_thread_t thread);

/**
 * @brief 启动调度器（RTOS场景）
 * @return OSAL_OK 成功；OSAL_ERR 失败
 */
int osal_kernel_start(void);

#endif
