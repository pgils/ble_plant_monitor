/**
 ****************************************************************************************
 *
 * @file ble_peripheral_task.c
 *
 * @brief BLE peripheral task
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
#include "time.h"
#include "sys_watchdog.h"
#include "sdk_list.h"
#include "ble_att.h"
#include "ble_gap.h"
#include "ble_gatts.h"
#include "ble_service.h"
#include "ble_uuid.h"

#include "ble_common.h"

/* Required libraries for the target application */
#include "version.h"
#include "i2c_sensors.h"
#include "ble_central_functions.h"
#include "ble_custom_service.h"
#include "ble_bluetanist_common.h"

/*
 * Flag whether this node acts as a Master node
 */
__RETAINED_RW bool _is_master_node = false;

/*
 * Arrays used for holding the value of the Characteristic Attributes registered
 * in Dialog BLE database.
 */
__RETAINED_RW node_data data;

/* Task handle */
__RETAINED_RW static OS_TASK ble_task_handle = NULL;


/*
 * @brief Read request callback
 *
 * This callback is fired when a peer device issues a read request. This implies that
 * that the peer device wants to read the Characteristic Attribute value. User should
 * provide the requested data.
 *
 * \param [in] value: The value returned back to the peer device
 *
 * \param [in] length: The number of bytes/octets returned
 *
 *
 * \warning: The callback function should have that specific prototype
 *
 * \warning: The BLE stack will not proceed with the next BLE event until the
 *        callback returns.
 */
void get_temperature_value_cb(uint8_t **value, uint16_t *length)
{
        uint16_t return_value;;

        taskENTER_CRITICAL();
        return_value = sensor_data.temperature;
        taskEXIT_CRITICAL();

        /* Update the Characteristic Attribute value as requested by the peer device */
        memcpy((void *)data.temperature, (void *)&return_value, sizeof(return_value));
        /* Return the requested data back to the peer device */
        *value  = data.temperature;       // A pointer that points to the returned data
        *length = sizeof(data.temperature);  // The size of the returned data, expressed in bytes.
}

void get_humidity_value_cb(uint8_t **value, uint16_t *length)
{
        uint16_t return_value;;

        taskENTER_CRITICAL();
        return_value = sensor_data.humidity;
        taskEXIT_CRITICAL();

        /* Update the Characteristic Attribute value as requested by the peer device */
        memcpy((void *)data.humidity, (void *)&return_value, sizeof(return_value));
        /* Return the requested data back to the peer device */
        *value  = data.humidity;       // A pointer that points to the returned data
        *length = sizeof(data.humidity);  // The size of the returned data, expressed in bytes.
}

void get_water_value_cb(uint8_t **value, uint16_t *length)
{
        uint16_t return_value;;

        taskENTER_CRITICAL();
        return_value = sensor_data.water;
        taskEXIT_CRITICAL();

        /* Update the Characteristic Attribute value as requested by the peer device */
        memcpy((void *)data.water, (void *)&return_value, sizeof(return_value));
        /* Return the requested data back to the peer device */
        *value  = data.water;       // A pointer that points to the returned data
        *length = sizeof(data.water);  // The size of the returned data, expressed in bytes.
}

void set_master_node_cb(const uint8_t *value, uint16_t length)
{
        _is_master_node = (*value >= 0);
        if(_is_master_node) {
                gap_scan_start();
        }
}


/*
 * Main code
 */

void ble_peripheral_task(void *params)
{
        ble_service_t *svc;
        ble_error_t status;
        int8_t wdog_id;

        uint16_t name_len;
        char name_buf[MAX_NAME_LEN + 1];        /* 1 byte for '\0' character */

        /* Scan Response object to be populated with <Complete Local Name> AD type */
        gap_adv_ad_struct_t *scan_rsp;

        /* Own BT address */
        own_address_t my_addr;
        ble_gap_address_get(&my_addr);

        printf("\n\r*** %s started ***\n\r", DEVICE_NAME);
        printf("*** Firmware %s ***\n\r", FW_VERSION);
        printf("*** My address is %s ***\n\n\r", ble_address_to_string((bd_address_t*)&my_addr));

        // in case services which do not use svc are all disabled, just suppress -Wunused-variable
        (void) svc;

        /* register ble_peripheral task to be monitored by watchdog */
        wdog_id = sys_watchdog_register(false);

        /* Get task's handler */
        ble_task_handle = OS_GET_CURRENT_TASK();

        srand(time(NULL));

        status = ble_enable();

        if ((status == BLE_STATUS_OK)) {
                ble_gap_role_set(GAP_PERIPHERAL_ROLE | GAP_CENTRAL_ROLE);
        } else {
                printf("%s: failed. Status=%d\r\n", __func__, status);
        }

        ble_register_app();

        /* Get device name from DEVICE_NAME */
        strcpy(name_buf, DEVICE_NAME);
        name_len = strlen(name_buf);
        name_buf[name_len] = '\0';

        /* Set device name */
        ble_gap_device_name_set(name_buf, ATT_PERM_READ);

        /* Define Scan Response object internals dealing with retrieved name */
        scan_rsp = GAP_ADV_AD_STRUCT_DECLARE(GAP_DATA_TYPE_LOCAL_NAME, name_len, name_buf);

        /* Set advertising data and start advertising */
        ble_gap_adv_ad_struct_set(ARRAY_LENGTH(adv_data), adv_data, 1 , scan_rsp);
        ble_gap_adv_start(GAP_CONN_MODE_UNDIRECTED);


#if (CHANGE_MTU_SIZE_ENABLE == 1)
        uint16_t mtu_size;
        ble_error_t mtu_err;

        /*
         * Get the old MTU size and print it on the serial console.
         */
        mtu_err = ble_gap_mtu_size_get(&mtu_size);
        printf("Old MTU size: %d, Status: %d\n\r", mtu_size, mtu_err);

        /*
         * @brief Change the MTU size.
         *
         * \note: The maximum supported MTU size is 512 octets.  The minimum supported MTU size,
         *        as defined by Bluetooth SIG, is 65 octets when LE secure connections are used,
         *        23 otherwise.
         *
         * \warning: The MTU size change should take place prior to creating the BLE attribute database.
         *           Otherwise, any already defined attribute database will be deleted!!!
         */
        mtu_err = ble_gap_mtu_size_set(125);

        /*
         * Get the updated MTU size and print it on the serial console.
         */
        mtu_err = ble_gap_mtu_size_get(&mtu_size);
        printf("New MTU size: %d, Status: %d\n\r", mtu_size, mtu_err);
#endif

        //************ Characteristic declarations for the master node init Service  *************
        const mcs_characteristic_config_t master_node_service[] = {

                /* Start scan Attribute */
                CHARACTERISTIC_DECLARATION(NODE_MASTER_ATTR_SET, CHARACTERISTIC_ATTR_VALUE_MAX_BYTES,
                        CHAR_WRITE_PROP_EN, CHAR_READ_PROP_DIS, CHAR_NOTIF_NONE, Set Master,
                                                        NULL, set_master_node_cb, NULL),

                /* Get connected node data Attribute */
                CHARACTERISTIC_DECLARATION(NODE_MASTER_ATTR_DATA, 0,
                        CHAR_WRITE_PROP_DIS, CHAR_READ_PROP_EN, CHAR_NOTIF_NONE, Get node data,
                                                                                 get_node_data_cb, NULL, NULL),

        };
        // ***************** Register the Bluetooth Service in Dialog BLE framework *****************
        SERVICE_DECLARATION(master_node_service, NODE_MASTER_SVC_UUID)

        //************ Characteristic declarations for the sensor_data BLE Service *************
        const mcs_characteristic_config_t sensor_data_service[] = {

                /* Temperature Characteristic Attribute */
                CHARACTERISTIC_DECLARATION(NODE_DATA_ATTR_TEMP, 0,
                          CHAR_WRITE_PROP_DIS, CHAR_READ_PROP_EN, CHAR_NOTIF_NONE, Temperature,
                                                                                   get_temperature_value_cb, NULL,NULL),


                /* Humidity Characteristic Attribute */
                CHARACTERISTIC_DECLARATION(NODE_DATA_ATTR_HUMID, 0,
                          CHAR_WRITE_PROP_DIS, CHAR_READ_PROP_EN, CHAR_NOTIF_NONE, Humidity,
                                                                                     get_humidity_value_cb, NULL, NULL),


               /* Water Characteristic Attribute */
               CHARACTERISTIC_DECLARATION(NODE_DATA_ATTR_WATER, 0,
                          CHAR_WRITE_PROP_DIS, CHAR_READ_PROP_EN, CHAR_NOTIF_NONE, Water,
                                                                                    get_water_value_cb, NULL, NULL),


       };
       // ****************** Register the Bluetooth Service in Dialog BLE framework *****************
        SERVICE_DECLARATION(sensor_data_service, NODE_DATA_SVC_UUID)

        /* Set advertising data and start advertising */
        ble_gap_adv_ad_struct_set(ARRAY_LENGTH(adv_data), adv_data, 1 , scan_rsp);
        ble_gap_adv_start(GAP_CONN_MODE_UNDIRECTED);

        for (;;) {
                OS_BASE_TYPE ret;
                uint32_t notif;

                /* notify watchdog on each loop */
                sys_watchdog_notify(wdog_id);

                /* suspend watchdog while blocking on OS_TASK_NOTIFY_WAIT() */
                sys_watchdog_suspend(wdog_id);

                /*
                 * Wait on any of the notification bits, then clear them all
                 */
                ret = OS_TASK_NOTIFY_WAIT(0, OS_TASK_NOTIFY_ALL_BITS, &notif, OS_TASK_NOTIFY_FOREVER);
                /* Blocks forever waiting for task notification. The return value must be OS_OK */
                OS_ASSERT(ret == OS_OK);

                /* resume watchdog */
                sys_watchdog_notify_and_resume(wdog_id);

                /* notified from BLE manager, can get event */
                if (notif & BLE_APP_NOTIFY_MASK) {
                        ble_evt_hdr_t *hdr;

                        hdr = ble_get_event(false);
                        if (!hdr) {
                                goto no_event;
                        }

                        if (pmp_ble_handle_event(hdr)) {
                                goto handled;
                        }

                        if (ble_service_handle_event(hdr)) {
                                goto handled;
                        }

                        switch (hdr->evt_code) {
                        case BLE_EVT_GAP_CONNECTED:
                                handle_evt_gap_connected((ble_evt_gap_connected_t *) hdr);
                                break;
                        case BLE_EVT_GAP_ADV_COMPLETED:
                                handle_evt_gap_adv_completed((ble_evt_gap_adv_completed_t *) hdr);
                                break;
                        case BLE_EVT_GAP_DISCONNECTED:
                                handle_evt_gap_disconnected((ble_evt_gap_disconnected_t *) hdr);
                                break;
                        case BLE_EVT_GAP_PAIR_REQ:
                        {
                                ble_evt_gap_pair_req_t *evt = (ble_evt_gap_pair_req_t *) hdr;
                                ble_gap_pair_reply(evt->conn_idx, true, evt->bond);
                                break;
                        }
                        default:
                                ble_handle_event_default(hdr);
                                break;
                        }

handled:
                        OS_FREE(hdr);

no_event:
                        // notify again if there are more events to process in queue
                        if (ble_has_event()) {
                                OS_TASK_NOTIFY(OS_GET_CURRENT_TASK(), BLE_APP_NOTIFY_MASK, eSetBits);
                        }

                }

        }
}
