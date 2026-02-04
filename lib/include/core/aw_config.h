#ifndef __AWLF_CONFIG_H__
#define __AWLF_CONFIG_H__

/* 功能裁剪 */
#define __AWLF_USE_ASSERT // TODO: 使能框架断言

/* 抽象层裁剪 */
#define __AWLF_USE_HAL_SERIALS
#define __AWLF_USE_HAL_CAN

/*
 * sync 加速开关（统一入口）：
 *
 * - AWLF_SYNC_ACCEL: 全局加速开关，用于标记是否启用加速后端
 * - AWLF_SYNC_ACCEL_COMPLETION: completion 原语的加速开关
 *
 * 说明：
 * - 默认不启用加速，具体由构建系统按需开启。
 */
#ifndef AWLF_SYNC_ACCEL
#define AWLF_SYNC_ACCEL 0
#endif

#ifndef AWLF_SYNC_ACCEL_COMPLETION
#define AWLF_SYNC_ACCEL_COMPLETION 0
#endif

#ifndef AWLF_FREERTOS_COMPLETION_NOTIFY_INDEX
#define AWLF_FREERTOS_COMPLETION_NOTIFY_INDEX 1u // 默认1，请不要轻易改动
#endif

#endif // __AWLF_CONFIG_H__
