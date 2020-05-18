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
#include "ble_common.h"
#include "ble_gap.h"
#include "ble_gatts.h"
#include "ble_service.h"
#include "ble_uuid.h"


/* Required libraries for the target application */
#include "ble_custom_service.h"
#include "i2c_sensors.h"

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
 * BLE scan defaults
 */
#define CFG_SCAN_TYPE           (GAP_SCAN_ACTIVE)
#define CFG_SCAN_MODE           (GAP_SCAN_GEN_DISC_MODE)
#define CFG_SCAN_INTERVAL       BLE_SCAN_INTERVAL_FROM_MS(0x64)
#define CFG_SCAN_WINDOW         BLE_SCAN_WINDOW_FROM_MS(0x32)
#define CFG_SCAN_FILT_WLIST     (false)
#define CFG_SCAN_FILT_DUPLT     (false)

/**
 * Default connection parameters
 */
#define CFG_CONN_PARAMS                                         \
        {                                                       \
                .interval_min = 0x28,                           \
                .interval_max = 0x38,                           \
                .slave_latency = 0,                             \
                .sup_timeout = 0x2a,                            \
        }

/*
 * Function prototypes
 */
static bool gap_scan_start(void);

/*
 * Arrays used for holding the value of the Characteristic Attributes registered
 * in Dialog BLE database.
 */
__RETAINED_RW uint8_t _temperature_attr_val[CHARACTERISTIC_ATTR_VALUE_MAX_BYTES] = { 0 };
__RETAINED_RW uint8_t _humidity_attr_val[CHARACTERISTIC_ATTR_VALUE_MAX_BYTES] = { 0 };
__RETAINED_RW uint8_t _water_attr_val[CHARACTERISTIC_ATTR_VALUE_MAX_BYTES] = { 0 };

/* List of devices waiting for connection */
__RETAINED static void *node_devices;



/*
 * BLE peripheral advertising data
 */
static const gap_adv_ad_struct_t adv_data[] = {
        GAP_ADV_AD_STRUCT_BYTES(GAP_DATA_TYPE_UUID128_LIST_INC,
                                0x00, 0x00, 0x00, 0x90, 0x06, 0x42,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x11, 0x11, 0x11, 0x11)
};

/*
 * device node list item for linked list
 */
struct node_list_elem {
        struct node_list_elem *next;
        bd_address_t addr;
};

/* Task handle */
__RETAINED_RW static OS_TASK ble_task_handle = NULL;



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

/*
 *
 * @brief Write start node scan callback
 *
 * \param [out] value:  >0 initiates a BLE scan
 *
 * \param [out] length: always 1
 *
 */
void set_var_start_scan(const uint8_t *value, uint16_t length)
{
        if(*value <= 0) {
                // TODO: disconnect all slave nodes
                return;
        }
        printf("Received 'start scan' command.\r\n");
        gap_scan_start();
}


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
        uint32_t return_value;

        taskENTER_CRITICAL();
        return_value = sensor_data.temperature;
        taskEXIT_CRITICAL();

        /* Clear the current Temperature Attribute value */
        memset((void *)_temperature_attr_val, 0x00, sizeof(_temperature_attr_val));

        /* Update the Characteristic Attribute value as requested by the peer device */
        memcpy((void *)_temperature_attr_val, (void *)&return_value, sizeof(return_value));

        /* Return the requested data back to the peer device */
        *value  = _temperature_attr_val;       // A pointer that points to the returned data
        *length = sizeof(CHARACTERISTIC_ATTR_VALUE_MAX_BYTES);  // The size of the returned data, expressed in bytes.
}

void get_humidity_value_cb(uint8_t **value, uint16_t *length)
{
        uint32_t return_value;

        taskENTER_CRITICAL();
        return_value = sensor_data.humidity;
        taskEXIT_CRITICAL();

        /* Clear the current Temperature Attribute value */
        memset((void *)_humidity_attr_val, 0x00, sizeof(_water_attr_val));

        /* Update the Characteristic Attribute value as requested by the peer device */
        memcpy((void *)_humidity_attr_val, (void *)&return_value, sizeof(return_value));

        /* Return the requested data back to the peer device */
        *value  = _humidity_attr_val;       // A pointer that points to the returned data
        *length = sizeof(CHARACTERISTIC_ATTR_VALUE_MAX_BYTES);  // The size of the returned data, expressed in bytes.
}

void get_water_value_cb(uint8_t **value, uint16_t *length)
{
        uint32_t return_value;

        taskENTER_CRITICAL();
        return_value = sensor_data.water;
        taskEXIT_CRITICAL();

        /* Clear the current Temperature Attribute value */
        memset((void *)_water_attr_val, 0x00, sizeof(_water_attr_val));

        /* Update the Characteristic Attribute value as requested by the peer device */
        memcpy((void *)_water_attr_val, (void *)&return_value, sizeof(return_value));

        /* Return the requested data back to the peer device */
        *value  = _water_attr_val;       // A pointer that points to the returned data
        *length = sizeof(CHARACTERISTIC_ATTR_VALUE_MAX_BYTES);  // The size of the returned data, expressed in bytes.
}



/*
 * Main code
 */

/*
 * Handler for ble_gap_scan_start call.
 * Initiates a scan procedure.
 * Prints scan parameters and status returned by call
 */
static bool gap_scan_start()
{
        ble_error_t status;
        gap_scan_params_t scan_params;
        gap_scan_type_t type = CFG_SCAN_TYPE;
        gap_scan_mode_t mode = CFG_SCAN_MODE;
        bool filt_dup = CFG_SCAN_FILT_DUPLT;
        bool wlist = CFG_SCAN_FILT_WLIST;
        uint16_t interval, window;

        ble_gap_scan_params_get(&scan_params);

        window = scan_params.window;
        interval = scan_params.interval;

        status = ble_gap_scan_start(type, mode, interval, window, wlist, filt_dup);

        printf("BlueTanist node scan started [%d]\r\n", status);

        return true;
}

/*
 * Handler for ble_gap_connect call.
 * Initiates a direct connection procedure to a specified peer device.
 */
static bool gap_connect(const bd_address_t *addr)
{
        ble_error_t status;
        gap_conn_params_t params = CFG_CONN_PARAMS;

        printf("connecting to: %s\r\n", ble_address_to_string(addr));
        status = ble_gap_connect(addr, &params);

        printf("connect status: %d\r\n", status);

        return true;
}

static void handle_evt_gap_connected(ble_evt_gap_connected_t *evt)
{
        printf("gap connected: %s\r\n", ble_address_to_string(&evt->peer_address));
        printf("my address: %s\r\n", ble_address_to_string(&evt->own_addr));
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

/*
 * Print GAP advertising report event information
 * Whitelist management API is not present in this SDK release. so we scan for all devices
 * and filter them manually.
 */
void handle_ble_evt_gap_adv_report(ble_evt_gap_adv_report_t *info)
{
        int i;
        int offset;

        // the tail of the adv info is AD flags. Get the offset where the actual id starts.
        offset = info->length - adv_data->len;

        // compare the device advertised UUID against our advertised UUID
        for (i = info->length-1; i >= offset; i--) {
                if(info->data[i] != adv_data->data[i-offset]) {
                        return;
                }
        }
        printf("BlueTanist node found: [%s]\r\n", ble_address_to_string(&info->address));

        // append the node to the linked list for later connection
        struct node_list_elem *node = OS_MALLOC(sizeof(*node));
        memcpy(&node->addr, &info->address, sizeof(node->addr));
        list_add(&node_devices, node);
}

/*
 * Print GAP scan completed event information
 */
void handle_ble_evt_gap_scan_completed(const ble_evt_gap_scan_completed_t *info)
{
        struct node_list_elem *element;

        printf("BlueTanist node scan completed. Found %d nodes\r\n", list_size(node_devices));

        // connect all found nodes
        while(node_devices != NULL) {
                element = (struct node_list_elem *) list_pop_back(&node_devices);
                gap_connect(&element->addr);
                OS_FREE(element);
        }
}

/*
 * Print GAP connection completed event information
 */
static void handle_ble_evt_gap_connection_completed(const ble_evt_gap_connection_completed_t *info)
{
        printf("BLE_EVT_GAP_CONNECTION_COMPLETED\r\n");
        printf("Status: 0x%02x\r\n", info->status);
}

bool pmp_ble_handle_event(const ble_evt_hdr_t *evt)
{
        switch (evt->evt_code) {
        case BLE_EVT_GAP_ADV_REPORT:
                handle_ble_evt_gap_adv_report((ble_evt_gap_adv_report_t *) evt);
                break;
        case BLE_EVT_GAP_SCAN_COMPLETED:
                handle_ble_evt_gap_scan_completed((ble_evt_gap_scan_completed_t *) evt);
                break;
        case BLE_EVT_GAP_CONNECTION_COMPLETED:
                handle_ble_evt_gap_connection_completed((ble_evt_gap_connection_completed_t *) evt);
                break;
        }
        return false;
}

void ble_peripheral_task(void *params)
{
        int8_t wdog_id;
        ble_service_t *svc;
        ble_error_t status;

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

        status = ble_enable();

        // enable both peripheral and central roles for master/slave node configurations
        if (status == BLE_STATUS_OK) {
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
                CHARACTERISTIC_DECLARATION(11111111-0000-0000-0000-000000000001, CHARACTERISTIC_ATTR_VALUE_MAX_BYTES,
                        CHAR_WRITE_PROP_EN, CHAR_READ_PROP_DIS, CHAR_NOTIF_NONE, Start Scan,
                                                        NULL, set_var_start_scan, NULL),

        };
        // ***************** Register the Bluetooth Service in Dialog BLE framework *****************
        SERVICE_DECLARATION(master_node_service, 11111111-0000-0000-0000-111111111111)

        //************ Characteristic declarations for the sensor_data BLE Service *************
        const mcs_characteristic_config_t sensor_data_service[] = {

                /* Temperature Characteristic Attribute */
                CHARACTERISTIC_DECLARATION(22222222-0000-0000-0000-000000000001, 0,
                          CHAR_WRITE_PROP_DIS, CHAR_READ_PROP_EN, CHAR_NOTIF_NONE, Temperature,
                                                                                   get_temperature_value_cb, NULL,NULL),


                /* Humidity Characteristic Attribute */
                CHARACTERISTIC_DECLARATION(22222222-0000-0000-0000-000000000002, 0,
                          CHAR_WRITE_PROP_DIS, CHAR_READ_PROP_EN, CHAR_NOTIF_NONE, Humidity,
                                                                                     get_humidity_value_cb, NULL, NULL),


               /* Water Characteristic Attribute */
               CHARACTERISTIC_DECLARATION(22222222-0000-0000-0000-000000000003, 0,
                          CHAR_WRITE_PROP_DIS, CHAR_READ_PROP_EN, CHAR_NOTIF_NONE, Water,
                                                                                    get_water_value_cb, NULL, NULL),


       };
       // ****************** Register the Bluetooth Service in Dialog BLE framework *****************
       SERVICE_DECLARATION(sensor_data_service, 22222222-0000-0000-0000-222222222222)


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
