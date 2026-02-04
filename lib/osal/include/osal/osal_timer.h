#ifndef __AWLF_OSAL_TIMER_H__
#define __AWLF_OSAL_TIMER_H__

#include <stdint.h>

typedef void* osal_timer_t;
typedef void (*osal_timer_cb_t)(osal_timer_t timer);

/**
 * @brief 创建软件定时器
 * @param name 定时器名称
 * @param period_ms 周期（ms）
 * @param auto_reload 1-自动重装载；0-单次
 * @param user_id 用户自定义指针
 * @param cb 回调函数
 * @return 成功返回定时器句柄，失败返回NULL
 */
osal_timer_t osal_timer_create(const char* name, uint32_t period_ms, int auto_reload, void* user_id, osal_timer_cb_t cb);

/**
 * @brief 启动定时器
 * @param timer 定时器句柄
 * @param timeout_ms 超时时间（ms），可用 OSAL_WAIT_FOREVER
 * @return OSAL_OK 成功；OSAL_ERR 失败
 */
int osal_timer_start(osal_timer_t timer, uint32_t timeout_ms);

/**
 * @brief 停止定时器
 * @param timer 定时器句柄
 * @param timeout_ms 超时时间（ms），可用 OSAL_WAIT_FOREVER
 * @return OSAL_OK 成功；OSAL_ERR 失败
 */
int osal_timer_stop(osal_timer_t timer, uint32_t timeout_ms);

/**
 * @brief 删除定时器
 * @param timer 定时器句柄
 * @param timeout_ms 超时时间（ms），可用 OSAL_WAIT_FOREVER
 * @return OSAL_OK 成功；OSAL_ERR 失败
 */
int osal_timer_delete(osal_timer_t timer, uint32_t timeout_ms);

/**
 * @brief 获取用户指针
 * @param timer 定时器句柄
 * @return 用户指针
 */
void* osal_timer_get_id(osal_timer_t timer);

/**
 * @brief 设置用户指针
 * @param timer 定时器句柄
 * @param id 用户指针
 */
void osal_timer_set_id(osal_timer_t timer, void* id);

#endif
