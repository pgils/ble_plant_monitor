/*
 * i2c_sensors.h
 *
 *  Created on: Mar 18, 2020
 *      Author: ssuser
 */

#ifndef I2C_SENSORS_H_
#define I2C_SENSORS_H_

struct sensor_data_t {
        uint16_t temperature;
        uint16_t humidity;
} sensor_data;

#if dg_configI2C_ADAPTER || dg_configUSE_HW_I2C

#if dg_configSENSOR_BMP180
/**
 * \brief BMP180 read function
 */
int read_bmp_sensor(uint16_t *temperature);

#endif /* dg_configSENSOR_BMP180 */
#if dg_configSENSOR_HIH6130
/**
 * \brief HIH6130 read function
 */
int read_hih_sensor();

#endif /* dg_configSENSOR_HIH6130 */

#endif /* dg_configI2C_ADAPTER || dg_configUSE_HW_I2C */

#endif /* I2C_SENSORS_H_ */
