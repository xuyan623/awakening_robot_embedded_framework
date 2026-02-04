#ifndef __AWLF_OSAL_SEM_H__
#define __AWLF_OSAL_SEM_H__

#include <stdint.h>

#include "osal_core.h"

typedef void* osal_sem_t;

/**
 * @brief 创建信号量
 * @param sem 输出信号量句柄
 * @param max_count 最大计数
 * @param init_count 初始计数
 * @return OSAL_OK 成功；OSAL_ERR 失败
 */
int osal_sem_create(osal_sem_t* sem, uint32_t max_count, uint32_t init_count);

/**
 * @brief 删除信号量
 * @param sem 信号量句柄
 * @return OSAL_OK 成功；OSAL_ERR 失败
 */
int osal_sem_delete(osal_sem_t sem);

/**
 * @brief 等待信号量
 * @param sem 信号量句柄
 * @param timeout_ms 超时时间（ms），可用 OSAL_WAIT_FOREVER
 * @return OSAL_OK 成功；OSAL_ERR 失败或超时
 */
int osal_sem_wait(osal_sem_t sem, uint32_t timeout_ms);

/**
 * @brief 释放信号量（线程上下文）
 * @param sem 信号量句柄
 * @return OSAL_OK 成功；OSAL_ERR 失败
 */
int osal_sem_post(osal_sem_t sem);

/**
 * @brief 释放信号量（中断上下文）
 * @param sem 信号量句柄
 * @return OSAL_OK 成功；OSAL_ERR 失败
 */
int osal_sem_post_isr(osal_sem_t sem);

#endif
