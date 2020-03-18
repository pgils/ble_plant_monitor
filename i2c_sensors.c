/*
 * i2c_sensors.c
 *
 *  Created on: Mar 18, 2020
 *      Author: ssuser
 */

#include <stdio.h>
#include <string.h>
#include "ad_i2c.h"
#include "hw_i2c.h"
#include "peripheral_setup.h"
#include "platform_devices.h"

/*
 * Drivers
 */
#include "bmp180.h"

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

        I2C_error_code = ad_i2c_read(dev_hdr, val, len, HW_I2C_F_ADD_STOP);
        if (HW_I2C_ABORT_NONE != I2C_error_code) {
                printf("I2C read failure: %u\n", I2C_error_code);
                return I2C_error_code;
        }

        ad_i2c_close(dev_hdr, false);

        return I2C_error_code;
}

void bmp_os_delay (u32 millisec)
{
        OS_DELAY_MS(millisec);
}

static int8_t bmp_write_reg(uint8_t dev_addr, uint8_t reg, uint8_t *val, uint8_t len)
{
        return i2c_write_reg(BMP180, reg, val, len);
}

static int8_t bmp_read_reg(uint8_t dev_addr, uint8_t reg, uint8_t *val, uint8_t len)
{
        return i2c_read_reg(BMP180, reg, val, len);
}

int read_bmp_sensor()
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
        uint16_t temp = bmp180_get_temperature(v_uncomp_temp_u16);
        uint32_t pres = bmp180_get_pressure(v_uncomp_press_u32);


        printf("Temp: %u (0.1)°C, Pressure: %lu (1.0)Pa\r\n", temp, pres);

        return 0;
}

