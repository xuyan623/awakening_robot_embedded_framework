#ifndef __AWLF_OSAL_SEM_H__
#define __AWLF_OSAL_SEM_H__

#include <stdint.h>

#include "osal_core.h"

typedef void* osal_sem_t;

/**
 * @brief 创建信号量（线程上下文）
 * @param sem 输出信号量句柄
 * @param max_count 最大计数
 * @param init_count 初始计数
 * @return `OSAL_OK` 成功；失败返回 `OSAL_INVALID/OSAL_NO_RESOURCE`
 * @note 禁止在 ISR 中调用。
 */
osal_status_t osal_sem_create(osal_sem_t* sem, uint32_t max_count, uint32_t init_count);

/**
 * @brief 删除信号量（线程上下文）
 * @param sem 信号量句柄
 * @return `OSAL_OK` 成功；失败返回 `OSAL_INVALID`
 * @note 禁止在 ISR 中调用。
 * @note 严格前置条件：调用方需确保无并发访问和无等待者。
 */
osal_status_t osal_sem_delete(osal_sem_t sem);

/**
 * @brief 等待信号量（线程上下文）
 * @param sem 信号量句柄
 * @param timeout_ms 超时时间（ms），可用 OSAL_WAIT_FOREVER
 * @return `OSAL_OK` 成功；失败返回 `OSAL_WOULD_BLOCK/OSAL_TIMEOUT/OSAL_INVALID/OSAL_INTERNAL`
 * @note 禁止在 ISR 中调用。
 */
osal_status_t osal_sem_wait(osal_sem_t sem, uint32_t timeout_ms);

/**
 * @brief 释放信号量（线程上下文）
 * @param sem 信号量句柄
 * @return `OSAL_OK` 成功；失败返回 `OSAL_INVALID/OSAL_NO_RESOURCE`
 * @note 禁止在 ISR 中调用。
 * @note 当计数已满时返回 `OSAL_NO_RESOURCE`（非阻塞失败）。
 */
osal_status_t osal_sem_post(osal_sem_t sem);

/**
 * @brief 释放信号量（ISR 上下文）
 * @param sem 信号量句柄
 * @return `OSAL_OK` 成功；失败返回 `OSAL_INVALID/OSAL_NO_RESOURCE`
 * @note 禁止在线程上下文调用。
 * @note 当计数已满时返回 `OSAL_NO_RESOURCE`（非阻塞失败）。
 */
osal_status_t osal_sem_post_from_isr(osal_sem_t sem);

/**
 * @brief 获取信号量当前计数（线程上下文）
 * @param sem 信号量句柄
 * @param out_count 输出计数
 * @return `OSAL_OK` 成功；失败返回 `OSAL_INVALID`
 * @note 禁止在 ISR 中调用。
 * @note 该接口仅用于观测，不作为删除安全性的充分条件。
 */
osal_status_t osal_sem_get_count(osal_sem_t sem, uint32_t* out_count);

/**
 * @brief 获取信号量当前计数（ISR 上下文）
 * @param sem 信号量句柄
 * @param out_count 输出计数
 * @return `OSAL_OK` 成功；失败返回 `OSAL_INVALID`
 * @note 禁止在线程上下文调用。
 * @note 该接口仅用于观测，不作为删除安全性的充分条件。
 */
osal_status_t osal_sem_get_count_from_isr(osal_sem_t sem, uint32_t* out_count);

#endif
