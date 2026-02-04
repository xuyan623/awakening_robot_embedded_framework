#ifndef __AWLF_OSAL_CORE_H__
#define __AWLF_OSAL_CORE_H__

#include <stddef.h>
#include <stdint.h>

#include "osal_config.h"

/* OSAL通用返回码定义*/
#define OSAL_OK (0)               /* 成功 */
#define OSAL_ERR (-1)             /* 通用失败 */
#define OSAL_ERR_TIMEOUT (-2)     /* 超时 */
#define OSAL_ERR_PARAM (-3)       /* 参数错误 */
#define OSAL_ERR_CTX (-4)         /* 上下文错误（中断/线程不匹配） */
#define OSAL_ERR_NOMEM (-5)       /* 资源不足/内存不足 */
#define OSAL_ERR_UNSUPPORTED (-6) /* 不支持的操作 */

/**
 * @brief 判断当前是否处于中断上下文
 * @return 1 表示在中断中；0 表示在线程上下文
 */
int osal_in_isr(void);

/**
 * @brief 进入临界区（线程上下文）
 * @note ISR中请使用 osal_irq_save/osal_irq_restore
 */
void osal_critical_enter(void);

/**
 * @brief 退出临界区（线程上下文）
 */
void osal_critical_exit(void);

/**
 * @brief 保存中断状态并进入临界区（线程/中断皆可）
 * @return 用于恢复的状态值
 */
uint32_t osal_irq_save(void);

/**
 * @brief 恢复中断状态（配合 osal_irq_save 使用）
 * @param state 之前保存的状态值
 */
void osal_irq_restore(uint32_t state);

/**
 * @brief OSAL内存申请
 * @param size 申请字节数
 * @return 成功返回指针，失败返回NULL
 */
void* osal_malloc(size_t size);

/**
 * @brief OSAL内存释放
 * @param ptr 指针
 */
void osal_free(void* ptr);

#ifdef __AWLF_USE_ASSERT
/* 断言失败后进入死循环，便于调试定位 */
#define OSAL_ASSERT(expr)                                                                                                                  \
    do                                                                                                                                     \
    {                                                                                                                                      \
        if (!(expr))                                                                                                                       \
        {                                                                                                                                  \
            for (;;)                                                                                                                       \
            {                                                                                                                              \
            }                                                                                                                              \
        }                                                                                                                                  \
    } while (0)
#else
#define OSAL_ASSERT(expr) ((void)0)
#endif

/* 断言当前处于线程上下文 */
#define OSAL_ASSERT_IN_TASK() OSAL_ASSERT(!osal_in_isr())

#endif
