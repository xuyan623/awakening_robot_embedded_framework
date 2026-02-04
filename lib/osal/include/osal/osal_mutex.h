#ifndef __AWLF_OSAL_MUTEX_H__
#define __AWLF_OSAL_MUTEX_H__

#include <stdint.h>

#include "osal_core.h"

typedef void* osal_mutex_t;

/**
 * @brief 创建互斥锁
 * @param mutex 输出互斥锁句柄
 * @return OSAL_OK 成功；OSAL_ERR 失败
 */
int osal_mutex_create(osal_mutex_t* mutex);

/**
 * @brief 删除互斥锁
 * @param mutex 互斥锁句柄
 * @return OSAL_OK 成功；OSAL_ERR 失败
 */
int osal_mutex_delete(osal_mutex_t mutex);

/**
 * @brief 加锁
 * @param mutex 互斥锁句柄
 * @param timeout_ms 超时时间（ms），可用 OSAL_WAIT_FOREVER
 * @return OSAL_OK 成功；OSAL_ERR 失败或超时
 */
int osal_mutex_lock(osal_mutex_t mutex, uint32_t timeout_ms);

/**
 * @brief 解锁
 * @param mutex 互斥锁句柄
 * @return OSAL_OK 成功；OSAL_ERR 失败
 */
int osal_mutex_unlock(osal_mutex_t mutex);

#endif
