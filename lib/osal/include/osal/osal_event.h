#ifndef __AWLF_OSAL_EVENT_H__
#define __AWLF_OSAL_EVENT_H__

#include <stdint.h>

#include "osal_core.h"

/* 任务通知索引（FreeRTOS使用） */
typedef void* osal_event_t;

/* osal_event_wait options */
#define OSAL_EVENT_OPT_WAIT_ALL (1u << 0) /* wait_all，否则 wait_any */
#define OSAL_EVENT_OPT_NO_CLEAR (1u << 1) /* 不清除位，否则 wait 成功后清除 wait_mask */

int osal_event_create(osal_event_t* event);
int osal_event_delete(osal_event_t event);

int osal_event_set(osal_event_t event, uint32_t flags);
int osal_event_set_isr(osal_event_t event, uint32_t flags);
int osal_event_clear(osal_event_t event, uint32_t flags);

/**
 * @brief 等待事件通知
 * @param wait_mask 等待的事件掩码
 * @param out_value 输出的事件值（可为NULL）
 * @param timeout_ms 超时时间（ms），可用 OSAL_WAIT_FOREVER
 * @return OSAL_OK 成功；OSAL_ERR 失败或超时
 */
int osal_event_wait(osal_event_t event, uint32_t wait_mask, uint32_t* out_value, uint32_t timeout_ms, uint32_t options);

/**
 * @brief 发送事件通知（线程上下文）
 * @param thread 目标线程
 * @param flags 事件标志
 * @return OSAL_OK 成功；OSAL_ERR 失败
 */
/* 旧的“线程通知位”事件 API 已移除：改为 osal_event_set */

/**
 * @brief 发送事件通知（中断上下文）
 * @param thread 目标线程
 * @param flags 事件标志
 * @return OSAL_OK 成功；OSAL_ERR 失败
 */
/* 旧的“线程通知位”事件 API 已移除：改为 osal_event_set_isr */

#endif
