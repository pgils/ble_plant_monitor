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
#include "ble_att.h"
#include "ble_common.h"
#include "ble_gap.h"
#include "ble_gatts.h"
#include "ble_service.h"
#include "ble_uuid.h"


/* Required libraries for the target application */
#include "ble_custom_service.h"

/*
 * The maximum length of name in scan response
 */
#define MAX_NAME_LEN    (BLE_SCAN_RSP_LEN_MAX - 2)

/*
 * The device's advertised name
 */
#define DEVICE_NAME     "BlueTanist Node-dev"

/* Enable/disable changing the default Maximum Protocol Unit (MTU). */
#define CHANGE_MTU_SIZE_ENABLE        (0)


/* Enable/disable debugging aid. Valid values */
#define DBG_SERIAL_CONSOLE_ENABLE      (1)


/*
 * Macro used for setting the maximum length, expressed in bytes,
 * of Characteristic Attributes.
 *
 * \warning The remote device must not exceed the max value when updating the
 *          Characteristic Attribute value. Otherwise, the system might crush.
 *
 **/
#define CHARACTERISTIC_ATTR_VALUE_MAX_BYTES       (50)

/*
 * Array used for holding the value of the Characteristic Attribute registered
 * in Dialog BLE database.
 */
__RETAINED_RW uint8_t _characteristic_attr_val[CHARACTERISTIC_ATTR_VALUE_MAX_BYTES] = { 0 };
__RETAINED_RW uint8_t _temperature_attr_val[CHARACTERISTIC_ATTR_VALUE_MAX_BYTES] = { 0 };
__RETAINED_RW uint8_t _humidity_attr_val[CHARACTERISTIC_ATTR_VALUE_MAX_BYTES] = { 0 };
__RETAINED_RW uint8_t _water_attr_val[CHARACTERISTIC_ATTR_VALUE_MAX_BYTES] = { 0 };



/*
 * Notification bits reservation
 * bit #0 is always assigned to BLE event queue notification
 */


/*
 * BLE peripheral advertising data
 */
static const gap_adv_ad_struct_t adv_data[] = {
        GAP_ADV_AD_STRUCT_BYTES(GAP_DATA_TYPE_UUID128_LIST_INC,
                                0x00, 0x00, 0x00, 0x90, 0x06, 0x42,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x11, 0x11, 0x11, 0x11)
};

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
void get_var_value_cb(uint8_t **value, uint16_t *length)
{

        /* Return the requested data back to the peer device */
        *value  = _characteristic_attr_val;       // A pointer that points to the returned data
        *length = sizeof(CHARACTERISTIC_ATTR_VALUE_MAX_BYTES);  // The size of the returned data, expressed in bytes.

        /*
         * This is just for debugging/demonstration purposes. UART is a slow interface
         * and will add significant delays compared to BLE speeds.
         */
#if (DBG_SERIAL_CONSOLE_ENABLE == 1)
        printf("\nRead callback function hit! - Returned value: %s\n\r", (char *)_characteristic_attr_val);
#endif
}


/*
 *
 * @brief Write request callback
 *
 * This callback is fired when a peer device issues a write request. This implies that
 * the peer device requested to modify the Characteristic Attribute value.
 *
 * \param [out] value:  The value written by the peer device.
 *
 * \param [out] length: The length of the written value, expressed in bytes/octets.
 *
 *
 * \warning: The callback function should have that specific prototype
 *
 * \warning: The BLE stack will not proceed with the next BLE event until the
 *        callback returns.
 *
 */
void set_var_value_cb(const uint8_t *value, uint16_t length)
{
        /* Clear the current Characteristic Attribute value */
        memset((void *)_characteristic_attr_val, 0x20, sizeof(_characteristic_attr_val));

        /* Update the Characteristic Attribute value as requested by the peer device */
        memcpy((void *)_characteristic_attr_val, (void *)value, length);

        /*
         * This is just for debugging/demonstration purposes. UART is a slow interface
         * and will add significant delays compared to BLE speeds.
         */
#if (DBG_SERIAL_CONSOLE_ENABLE == 1)
        printf("\nWrite callback function hit! - Written value: %s, length: %d\n\r",
                                                        _characteristic_attr_val, length);
#endif
}


/*
 * @brief Notification event callback
 *
 *  A notification callback function is fired for each connected device.
 *  It's a prerequisite that peer devices will have their notifications/
 *  indications enabled.
 *
 * \param [in] conn_idx: Connection index
 *
 * \param [in] status: The status of the aforementioned operation:
 *
 *                     0 --> notification/indication wasn't sent successfully
 *                     1 --> notification/indication was sent successfully
 *
 * \param [in] type: Signifies whether a notification or indication has been sent
 *                   to the peer device:
 *
 *                   0 --> when a notification is sent
 *                   1 --> when an indications is sent
 *
 *
 * \warning: The BLE stack will not proceed with the next BLE event until the
 *        callback returns.
 */
void event_sent_cb(uint16_t conn_idx, bool status, gatt_event_t type)
{
        /*
         * This is just for debugging/demonstration purposes. UART is a slow interface
         * and will add significant delay compared to the BLE speeds.
         */
#if (DBG_SERIAL_CONSOLE_ENABLE == 1)
        printf("\nNotify callback - Connection idx: %d, Status: %d, Type: %d\n\r",
                                                                conn_idx, status, type);
#endif
}

void get_temperature_value_cb(uint8_t **value, uint16_t *length)
{
        /* Generate new random value */
        uint8_t newVal[1] = { rand() % 20 + 20 };

        /* Clear the current Temperature Attribute value */
        memset((void *)_temperature_attr_val, 0x00, sizeof(_temperature_attr_val));

        /* Update the Characteristic Attribute value as requested by the peer device */
        memcpy((void *)_temperature_attr_val, (void *)&newVal, 1);

        /* Return the requested data back to the peer device */
        *value  = _temperature_attr_val;       // A pointer that points to the returned data
        *length = sizeof(CHARACTERISTIC_ATTR_VALUE_MAX_BYTES);  // The size of the returned data, expressed in bytes.
}

void get_humidity_value_cb(uint8_t **value, uint16_t *length)
{
        /* Generate new random value */
        uint8_t newVal[1] = { rand() % 70 + 30 };

        /* Clear the current Temperature Attribute value */
        memset((void *)_humidity_attr_val, 0x00, sizeof(_water_attr_val));

        /* Update the Characteristic Attribute value as requested by the peer device */
        memcpy((void *)_humidity_attr_val, (void *)&newVal, 1);

        /* Return the requested data back to the peer device */
        *value  = _humidity_attr_val;       // A pointer that points to the returned data
        *length = sizeof(CHARACTERISTIC_ATTR_VALUE_MAX_BYTES);  // The size of the returned data, expressed in bytes.
}

void get_water_value_cb(uint8_t **value, uint16_t *length)
{
        /* Generate new random value */
        uint8_t newVal[1] = { rand() % 90 + 10 };

        /* Clear the current Temperature Attribute value */
        memset((void *)_water_attr_val, 0x00, sizeof(_water_attr_val));

        /* Update the Characteristic Attribute value as requested by the peer device */
        memcpy((void *)_water_attr_val, (void *)&newVal, 1);

        /* Return the requested data back to the peer device */
        *value  = _water_attr_val;       // A pointer that points to the returned data
        *length = sizeof(CHARACTERISTIC_ATTR_VALUE_MAX_BYTES);  // The size of the returned data, expressed in bytes.
}



/*
 * Main code
 */
static void handle_evt_gap_connected(ble_evt_gap_connected_t *evt)
{
        /*
         * Manage connection information
         */
}

static void handle_evt_gap_disconnected(ble_evt_gap_disconnected_t *evt)
{
        /*
         * Manage disconnection information
         */
}

static void handle_evt_gap_adv_completed(ble_evt_gap_adv_completed_t *evt)
{
        // restart advertising so we can connect again
        ble_gap_adv_start(GAP_CONN_MODE_UNDIRECTED);
}



void ble_peripheral_task(void *params)
{
        int8_t wdog_id;
        ble_service_t *svc;

        uint16_t name_len;
        char name_buf[MAX_NAME_LEN + 1];        /* 1 byte for '\0' character */

        /* Scan Response object to be populated with <Complete Local Name> AD type */
        gap_adv_ad_struct_t *scan_rsp;

        printf("\n\r*** %s started ***\n\n\r", DEVICE_NAME);

        // in case services which do not use svc are all disabled, just suppress -Wunused-variable
        (void) svc;

        /* register ble_peripheral task to be monitored by watchdog */
        wdog_id = sys_watchdog_register(false);

        /* Get task's handler */
        ble_task_handle = OS_GET_CURRENT_TASK();

        srand(time(NULL));

        ble_peripheral_start();
        ble_register_app();

        /* Get device name from DEVICE_NAME */
        strcpy(name_buf, DEVICE_NAME);
        name_len = strlen(name_buf);
        name_buf[name_len] = '\0';

        /* Set device name */
        ble_gap_device_name_set(name_buf, ATT_PERM_READ);

        /* Define Scan Response object internals dealing with retrieved name */
        scan_rsp = GAP_ADV_AD_STRUCT_DECLARE(GAP_DATA_TYPE_LOCAL_NAME, name_len, name_buf);


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


        //************ Characteristic declarations for the 1st custom BLE Service  *************
        const mcs_characteristic_config_t custom_service_1[] = {

                /* Initialized Characteristic Attribute */
                CHARACTERISTIC_DECLARATION(11111111-0000-0000-0000-000000000001, CHARACTERISTIC_ATTR_VALUE_MAX_BYTES,
                        CHAR_WRITE_PROP_EN, CHAR_READ_PROP_EN, CHAR_NOTIF_INDIC_EN, Initialized Characteristic,
                                                        get_var_value_cb, set_var_value_cb, event_sent_cb),


                 // -----------------------------------------------------------------
                 // -- Here you can continue adding more Characteristic Attributes --
                 // -----------------------------------------------------------------


        };
        // ***************** Register the Bluetooth Service in Dialog BLE framework *****************
        SERVICE_DECLARATION(custom_service_1, 11111111-0000-0000-0000-111111111111)



        //************ Characteristic declarations for the 2nd BLE Service *************
        const mcs_characteristic_config_t custom_service_2[] = {

                /* Uninitialized Characteristic Attribute. You can defined your preferred settings */
                CHARACTERISTIC_DECLARATION(22222222-0000-0000-0000-000000000001, 0,
                          CHAR_WRITE_PROP_DIS, CHAR_READ_PROP_EN, CHAR_NOTIF_NONE, Temperature,
                                                                                   get_temperature_value_cb, NULL,NULL),


                /* Uninitialized Characteristic Attribute. You can defined your preferred settings */
                CHARACTERISTIC_DECLARATION(22222222-0000-0000-0000-000000000002, 0,
                          CHAR_WRITE_PROP_DIS, CHAR_READ_PROP_EN, CHAR_NOTIF_NONE, Humidity,
                                                                                     get_humidity_value_cb, NULL, NULL),


               /* Uninitialized Characteristic Attribute. You can defined your preferred settings */
               CHARACTERISTIC_DECLARATION(22222222-0000-0000-0000-000000000003, 0,
                          CHAR_WRITE_PROP_DIS, CHAR_READ_PROP_EN, CHAR_NOTIF_NONE, Water,
                                                                                    get_water_value_cb, NULL, NULL),


                // -----------------------------------------------------------------
                // -- Here you can continue adding more Characteristic attributes --
                // -----------------------------------------------------------------

       };
       // ****************** Register the Bluetooth Service in Dialog BLE framework *****************
       SERVICE_DECLARATION(custom_service_2, 22222222-0000-0000-0000-222222222222)



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
