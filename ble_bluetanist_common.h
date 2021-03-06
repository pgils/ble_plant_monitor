/*
 * ble_common.h
 *
 *  Created on: May 24, 2020
 *      Author: ssuser
 */

#ifndef BLE_BLUETANIST_COMMON_H_
#define BLE_BLUETANIST_COMMON_H_

#include "ble_gap.h"
#include "ble_gatt.h"

/*
 * The maximum length of name in scan response
 */
#define MAX_NAME_LEN    (BLE_SCAN_RSP_LEN_MAX - 2)

/*
 * The device's advertised name
 */
#define DEVICE_NAME     "BlueTanist Node"

/* Enable/disable changing the default Maximum Protocol Unit (MTU). */
#define CHANGE_MTU_SIZE_ENABLE        (0)


/* Enable/disable debugging aid. Valid values */
#define DBG_SERIAL_CONSOLE_ENABLE      (1)

/*
 * UUID's for common node services and attributes
 */
#define NODE_MASTER_SVC_UUID    "11111111-0000-0000-0000-111111111111"
#define NODE_MASTER_ATTR_SET    "11111111-0000-0000-0000-000000000001"
#define NODE_MASTER_ATTR_DATA   "11111111-0000-0000-0000-000000000010"

#define NODE_DATA_SVC_UUID      "22222222-0000-0000-0000-222222222222"
#define NODE_DATA_ATTR_TEMP     "22222222-0000-0000-0000-000000000001"
#define NODE_DATA_ATTR_HUMID    "22222222-0000-0000-0000-000000000002"
#define NODE_DATA_ATTR_WATER    "22222222-0000-0000-0000-000000000003"

att_uuid_t node_data_svc_uuid;
att_uuid_t node_data_attr_temp;
att_uuid_t node_data_attr_humid;
att_uuid_t node_data_attr_water;

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


/*
 * BLE peripheral advertising data
 */
static const gap_adv_ad_struct_t adv_data[] = {
        GAP_ADV_AD_STRUCT_BYTES(GAP_DATA_TYPE_UUID128_LIST_INC,
                                0x00, 0x00, 0x00, 0x90, 0x06, 0x42,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x11, 0x11, 0x11, 0x11)
};

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
 * sensor attribute
 * holds handles and converted sensor data
 */
struct sensor_attr_list_elem {
        struct sensor_attr_list_elem *next;
        att_uuid_t uuid;
        uint16_t handle;
        uint8_t value[2];
};

/*
 * device node list item for linked list
 */
struct node_list_elem {
        struct node_list_elem *next;
        bd_address_t addr;
        uint16_t conn_idx;
        void *attr_list;
};

#define NODE_SENSOR_DATA_TRANSFER_SIZE       8

void event_sent_cb(uint16_t conn_idx, bool status, gatt_event_t type);
void handle_evt_gap_connected(ble_evt_gap_connected_t *evt);
void handle_evt_gap_disconnected(ble_evt_gap_disconnected_t *evt);
void handle_evt_gap_adv_completed(ble_evt_gap_adv_completed_t *evt);
void handle_ble_evt_gap_connection_completed(const ble_evt_gap_connection_completed_t *info);

#endif /* BLE_BLUETANIST_COMMON_H_ */
