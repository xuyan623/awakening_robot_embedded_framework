#ifndef __AWLF_OSAL_QUEUE_H__
#define __AWLF_OSAL_QUEUE_H__

#include <stdint.h>

#include "osal_core.h"

typedef void* osal_queue_t;

/**
 * @brief 创建消息队列
 * @param queue 输出队列句柄
 * @param length 队列长度（元素个数）
 * @param item_size 单个元素大小（字节）
 * @return OSAL_OK 成功；OSAL_ERR 失败
 */
int osal_queue_create(osal_queue_t* queue, uint32_t length, uint32_t item_size);

/**
 * @brief 删除消息队列
 * @param queue 队列句柄
 * @return OSAL_OK 成功；OSAL_ERR 失败
 */
int osal_queue_delete(osal_queue_t queue);

/**
 * @brief 发送消息（线程上下文）
 * @param queue 队列句柄
 * @param item 数据指针
 * @param timeout_ms 超时时间（ms），可用 OSAL_WAIT_FOREVER
 * @return OSAL_OK 成功；OSAL_ERR 失败或超时
 */
int osal_queue_send(osal_queue_t queue, const void* item, uint32_t timeout_ms);

/**
 * @brief 发送消息（中断上下文）
 * @param queue 队列句柄
 * @param item 数据指针
 * @return OSAL_OK 成功；OSAL_ERR 失败
 */
int osal_queue_send_isr(osal_queue_t queue, const void* item);

/**
 * @brief 接收消息（线程上下文）
 * @param queue 队列句柄
 * @param item 输出数据指针
 * @param timeout_ms 超时时间（ms），可用 OSAL_WAIT_FOREVER
 * @return OSAL_OK 成功；OSAL_ERR 失败或超时
 */
int osal_queue_recv(osal_queue_t queue, void* item, uint32_t timeout_ms);

/**
 * @brief 接收消息（中断上下文）
 * @param queue 队列句柄
 * @param item 输出数据指针
 * @return OSAL_OK 成功；OSAL_ERR 失败
 */
int osal_queue_recv_isr(osal_queue_t queue, void* item);

/**
 * @brief 查看消息（线程上下文，不出队）
 * @param queue 队列句柄
 * @param item 输出数据指针
 * @param timeout_ms 超时时间（ms），可用 OSAL_WAIT_FOREVER
 * @return OSAL_OK 成功；OSAL_ERR 失败或超时
 */
int osal_queue_peek(osal_queue_t queue, void* item, uint32_t timeout_ms);

/**
 * @brief 查看消息（中断上下文，不出队）
 * @param queue 队列句柄
 * @param item 输出数据指针
 * @return OSAL_OK 成功；OSAL_ERR 失败
 */
int osal_queue_peek_isr(osal_queue_t queue, void* item);

/**
 * @brief 复位队列（清空）
 * @param queue 队列句柄
 * @return OSAL_OK 成功；OSAL_ERR 失败
 */
int osal_queue_reset(osal_queue_t queue);

/**
 * @brief 获取队列中消息数量
 * @param queue 队列句柄
 * @return 当前消息数量
 */
uint32_t osal_queue_messages_waiting(osal_queue_t queue);

/**
 * @brief 获取队列剩余空间数量
 * @param queue 队列句柄
 * @return 剩余可用槽位数
 */
uint32_t osal_queue_spaces_available(osal_queue_t queue);

#endif
