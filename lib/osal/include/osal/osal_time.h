#ifndef __AWLF_OSAL_TIME_H__
#define __AWLF_OSAL_TIME_H__

#include <stdint.h>

/* 时间单位换算常量 */
#define OSAL_MS_PER_SEC 1000ULL
#define OSAL_US_PER_MS 1000ULL
#define OSAL_NS_PER_MS 1000000ULL
#define OSAL_NS_PER_SEC 1000000000ULL

/**
 * @brief 向上取整的 us->ms 转换
 */
static inline uint32_t osal_time_us_to_ms_ceil(uint64_t us)
{
    return (uint32_t)((us + OSAL_US_PER_MS - 1ULL) / OSAL_US_PER_MS);
}

/**
 * @brief 向上取整的 ns->ms 转换
 */
static inline uint32_t osal_time_ns_to_ms_ceil(uint64_t ns)
{
    return (uint32_t)((ns + OSAL_NS_PER_MS - 1ULL) / OSAL_NS_PER_MS);
}

/**
 * @brief 获取系统运行时间（毫秒，32位）
 * @note 可能发生溢出，适合短周期逻辑
 */
uint32_t osal_time_ms(void);

/**
 * @brief 获取系统运行时间（毫秒，64位）
 * @note 适合长时间运行计时
 */
uint64_t osal_time_ms64(void);

/**
 * @brief 获取系统运行时间（微秒，32位）
 * @note 分辨率可能受系统tick影响，适合短周期逻辑
 */
uint32_t osal_time_us(void);

/**
 * @brief 获取系统运行时间（微秒，64位）
 * @note 分辨率可能受系统tick影响，适合长时间运行计时
 */
uint64_t osal_time_us64(void);

/**
 * @brief 获取系统运行时间（纳秒，64位）
 * @note 分辨率可能受系统tick影响
 */
uint64_t osal_time_ns64(void);

/**
 * @brief 周期延时（保持固定周期）
 * @param last_ms 上一次时间戳（首次需初始化）
 * @param period_ms 周期（ms）
 */
void osal_delay_until_ms(uint32_t* last_ms, uint32_t period_ms);

#endif
