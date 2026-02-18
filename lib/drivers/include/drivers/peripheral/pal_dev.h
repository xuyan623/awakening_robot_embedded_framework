#ifndef __PAL_DEV_H__
#define __PAL_DEV_H__

#include "core/aw_config.h"
#include "drivers/model/device.h"

#ifdef __AWLF_USE_HAL_SERIALS
#include "serial/pal_serial_dev.h"
#endif

#ifdef __AWLF_USE_HAL_CAN
#include "can/pal_can_dev.h"
#endif

#endif // __PAL_DEV_H__
