/*
 * i2c_sensors.h
 *
 *  Created on: Mar 18, 2020
 *      Author: ssuser
 */

#ifndef I2C_SENSORS_H_
#define I2C_SENSORS_H_


#if dg_configI2C_ADAPTER || dg_configUSE_HW_I2C

#if dg_configSENSOR_BMP180
/**
 * \brief BMP180 read function
 */
int read_bmp_sensor();

#endif /* dg_configSENSOR_BMP180 */

#endif /* dg_configI2C_ADAPTER || dg_configUSE_HW_I2C */

#endif /* I2C_SENSORS_H_ */
