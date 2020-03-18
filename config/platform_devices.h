/*
 * platform_devices.h
 *
 *  Created on: Mar 18, 2020
 *      Author: ssuser
 */

#ifndef CONFIG_PLATFORM_DEVICES_H_
#define CONFIG_PLATFORM_DEVICES_H_

#include <ad_i2c.h>
#include "peripheral_setup.h"

/**
 * \brief I2C device handle
 */
typedef const void* i2c_device;


/*
 * I2C DEVICES
 *****************************************************************************************
 */
#if dg_configI2C_ADAPTER || dg_configUSE_HW_I2C

/**
 * \brief Generic I2C device
 */
extern i2c_device GENERIC;

/**
 * \brief BMP180 device
 */
extern i2c_device BMP180;

#endif /* dg_configI2C_ADAPTER || dg_configUSE_HW_I2C */

#endif /* CONFIG_PLATFORM_DEVICES_H_ */
