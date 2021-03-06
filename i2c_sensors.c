/*
 * i2c_sensors.c
 *
 *  Created on: Mar 18, 2020
 *      Author: ssuser
 */

#include <stdio.h>
#include <string.h>
#include "osal.h"
#include "ad_i2c.h"
#include "hw_i2c.h"
#include "peripheral_setup.h"
#include "platform_devices.h"
#include "i2c_sensors.h"

/*
 * Drivers
 */
#if dg_configSENSOR_BMP180
#include "bmp180.h"
#endif /* dg_configSENSOR_BMP180 */

/*
 * Error code returned after an I2C operation. It can be used
 * to identify the reason of a failure.
 */
__RETAINED static HW_I2C_ABORT_SOURCE I2C_error_code;


static int8_t i2c_write_reg(i2c_device dev, uint8_t reg, uint8_t *val, uint8_t len)
{
        I2C_error_code = HW_I2C_ABORT_NONE;

        /* Open the device */
        ad_i2c_handle_t dev_hdr = ad_i2c_open((ad_i2c_controller_conf_t *)dev);

        /*
         * Prepend the register to data to be written.
         */
        uint8_t data[len+1];
        memcpy(&data[1], val, len);
        data[0] = reg;

        /*
         * Write the data
         */
        I2C_error_code = ad_i2c_write(dev_hdr, data, len+1, HW_I2C_F_ADD_STOP);
        if (HW_I2C_ABORT_NONE != I2C_error_code) {
                printf("I2C write failure: %u\n", I2C_error_code);
                return I2C_error_code;
        }

        /* Close the device */
        ad_i2c_close(dev_hdr, false);

        return I2C_error_code;
}

static int8_t i2c_read_reg(i2c_device dev, uint8_t reg, uint8_t *val, uint8_t len)
{
        I2C_error_code = HW_I2C_ABORT_NONE;

        /* Open the device */
        ad_i2c_handle_t dev_hdr = ad_i2c_open((ad_i2c_controller_conf_t *)dev);

        /*
         * Before reading values from sensor registers we need to send one byte information to it
         * to inform which sensor register will be read now.
         */
        I2C_error_code = ad_i2c_write(dev_hdr, &reg, 1, HW_I2C_F_ADD_STOP);
        if (HW_I2C_ABORT_NONE != I2C_error_code) {
                printf("I2C write failure: %u\n", I2C_error_code);
                return I2C_error_code;
        }

        /*
         * Read <len> bytes.
         */
        I2C_error_code = ad_i2c_read(dev_hdr, val, len, HW_I2C_F_ADD_STOP);
        if (HW_I2C_ABORT_NONE != I2C_error_code) {
                printf("I2C read failure: %u\n", I2C_error_code);
                return I2C_error_code;
        }

        /* Close the device */
        ad_i2c_close(dev_hdr, false);

        return I2C_error_code;
}

#if dg_configSENSOR_BMP180
/*
 * Wrapper functions for BMP180 driver
 */
void bmp_os_delay (u32 millisec)
{
        OS_DELAY_MS(millisec);
}

static int8_t bmp_write_reg(uint8_t dev_addr, uint8_t reg, uint8_t *val, uint8_t len)
{
        /* Ignore unused parameter dev_addr */
        (void)(dev_addr);
        return i2c_write_reg(BMP180, reg, val, len);
}

static int8_t bmp_read_reg(uint8_t dev_addr, uint8_t reg, uint8_t *val, uint8_t len)
{
        /* Ignore unused parameter dev_addr */
        (void)(dev_addr);
        return i2c_read_reg(BMP180, reg, val, len);
}

/*
 * Read ambient temperature and atmospheric pressure from BMP180 sensor.
 * Based on
 * https://github.com/BoschSensortec/BMP180_driver/blob/49a796a0d675d5b3a7573d9240cf829e658ed79c/bmp180_support.c#L107
 */
int read_bmp_sensor(struct sensor_data_t *data)
{
        struct bmp180_t bmp180;
        int32_t com_rslt = E_BMP_COMM_RES;
        uint16_t v_uncomp_temp_u16 = BMP180_INIT_VALUE;
        uint32_t v_uncomp_press_u32 = BMP180_INIT_VALUE;

        // Set up function pointers
        bmp180.bus_write = bmp_write_reg;
        bmp180.bus_read = bmp_read_reg;
        bmp180.dev_addr = BMP180_I2C_ADDR;
        bmp180.delay_msec = bmp_os_delay;

        com_rslt = bmp180_init(&bmp180);
        com_rslt += bmp180_get_calib_param();

        v_uncomp_temp_u16 = bmp180_get_uncomp_temperature();
        v_uncomp_press_u32 = bmp180_get_uncomp_pressure();

        com_rslt += bmp180_get_temperature(v_uncomp_temp_u16);

        com_rslt += bmp180_get_pressure(v_uncomp_press_u32);
        uint32_t temp = bmp180_get_temperature(v_uncomp_temp_u16);
        uint32_t pres = bmp180_get_pressure(v_uncomp_press_u32);

        data->temperature = temp * 10;
        printf("BMP: Temp: %lu, Pressure: %lu\r\n", temp, pres);

        return 0;
}
#endif /* dg_configSENSOR_BMP180 */

#if dg_configSENSOR_HIH6130
/*
 * Read ambient temperature and relative humidity from HIH6130 sensor.
 */
int read_hih_sensor(struct sensor_data_t *data)
{
        uint8_t raw_data[4];
        uint16_t raw_humidity, raw_temperature;
        uint32_t rel_humidity, amb_temperature;

        /*
         * Send a measurement request.
         */
        i2c_write_reg(HIH6130, 0x0, NULL, 0);
        /*
         * Wait for the measurement to complete.
         * The measurement cycle duration is typically
         * 36.65ms for temperature and humidity readings.
         * https://sensing.honeywell.com/i2c-comms-humidicon-tn-009061-2-en-final-07jun12.pdf
         */
        OS_DELAY_MS(40);

        /*
         * Read 4 bytes from the sensor
         */
        if (0 != i2c_read_reg(HIH6130, 0x0, raw_data, 4)) {
                printf("HIH6130 sensor read failed.");
                return 1;
        }

        /*
         * Calculate the Relative humidity and Ambient temperature values.
         * calculations based on:
         * https://github.com/stevemarple/HIH61xx/blob/c8f90c5c30ba24ab2d017caa72c98508180187e9/src/HIH61xx.h#L168
         */
        raw_humidity = ((((uint16_t)raw_data[0] & 0x3F) << 8) | (uint16_t)raw_data[1]);
        raw_temperature = ((uint16_t)raw_data[2] << 6) | ((uint16_t)raw_data[3] >> 2);
        rel_humidity = ((uint32_t)raw_humidity * 10000) / 16382;
        amb_temperature = (((uint32_t)raw_temperature * 16500) / 16382) - 4000;

        data->temperature = amb_temperature;
        data->humidity = rel_humidity;
        printf("HIH6130: Temp: %lu, Humidity: %lu\r\n", amb_temperature, rel_humidity);

        return 0;
}
#endif /* dg_configSENSOR_HIH6130 */

