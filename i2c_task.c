/**
 ****************************************************************************************
 *
 * @file i2c_task.c
 *
 * @brief I2C task
 *
 * Copyright (C) 2019 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#include <stdio.h>

#include "osal.h"

/* Required libraries for the target application */
#include "i2c_sensors.h"


/* Enable/disable debugging aid. Valid values */
#define DBG_SERIAL_CONSOLE_ENABLE      (1)

/* Task handle */
__RETAINED_RW static OS_TASK i2c_task_handle = NULL;


void I2C_task(void *params)
{
        /* Get task's handler */
        i2c_task_handle = OS_GET_CURRENT_TASK();

        printf("\n\r*** I2C task started ***\n\n\r");


        for (;;) {
                struct sensor_data_t new_sensor_data = { 0 };

#if dg_configSENSOR_BMP180
                read_bmp_sensor(&new_sensor_data.temperature);
#endif /* dg_configSENSOR_BMP180 */
#if dg_configSENSOR_HIH6130
                read_hih_sensor(&new_sensor_data.temperature);
#endif /* dg_configSENSOR_HIH6130 */

                /*
                 * Copy new sensor data to the global data struct
                 */
                taskENTER_CRITICAL();
                sensor_data.temperature = new_sensor_data.temperature;
                taskEXIT_CRITICAL();

                OS_DELAY_MS(1000);

//                OS_BASE_TYPE ret;
//                uint32_t notif;
//
//                /*
//                 * Wait on any of the notification bits, then clear them all
//                 */
//                ret = OS_TASK_NOTIFY_WAIT(0, OS_TASK_NOTIFY_ALL_BITS, &notif, OS_TASK_NOTIFY_FOREVER);
//                /* Blocks forever waiting for task notification. The return value must be OS_OK */
//                OS_ASSERT(ret == OS_OK);

        }
}
