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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "osal.h"


/* Required libraries for the target application */


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
                OS_BASE_TYPE ret;
                uint32_t notif;

                /*
                 * Wait on any of the notification bits, then clear them all
                 */
                ret = OS_TASK_NOTIFY_WAIT(0, OS_TASK_NOTIFY_ALL_BITS, &notif, OS_TASK_NOTIFY_FOREVER);
                /* Blocks forever waiting for task notification. The return value must be OS_OK */
                OS_ASSERT(ret == OS_OK);

        }
}
