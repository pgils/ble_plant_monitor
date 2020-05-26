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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "osal.h"
#include "time.h"
#include "sys_watchdog.h"
#include "sdk_list.h"
#include "ble_att.h"
#include "ble_gap.h"
#include "ble_gattc.h"
#include "ble_gatts.h"
#include "ble_service.h"
#include "ble_uuid.h"

#include "ble_common.h"

/* Required libraries for the target application */
#include "ble_central_functions.h"
#include "ble_bluetanist_common.h"
#include "ble_custom_service.h"


/* List of devices waiting for connection */
// TODO: use a queue for this
__RETAINED static void *node_devices_scanned;
/* List of devices connected */
__RETAINED static void *node_devices_connected;
/* Retained return data array for slave sensor data */
__RETAINED static uint8_t *node_data;

/*
 * helper function for finding a node by connection id in a linked list
 */
void *list_find_node_by_connid(void *head, const uint8_t idx)
{
        struct node_list_elem *e = head;

        while (e && !(e->conn_idx == idx)) {
                e = e->next;
        }

        return e;
}

/*
 * helper function for finding an attribute list item by handle
 */
void *list_find_attr_by_handle(void *head, const uint16_t handle)
{
        struct sensor_attr_list_elem *e = head;

        while (e && !(e->handle == handle)) {
                e = e->next;
        }

        return e;
}

void list_foreach_nonconst(void *head, void (* cb) (const void *, void *), void *ud)
{
        struct list_elem *e = head;

        while (e) {
                cb(e, ud);
                e = e->next;
        }
}

void discover_node_service(const void *elem, const void *ud)
{
        ble_error_t status;
        const struct node_list_elem *node = elem;
        const att_uuid_t *svc_uuid = ud;

        printf("discover for: %d\r\n", node->conn_idx);
        status = ble_gattc_discover_svc(node->conn_idx, svc_uuid);
}

void copy_attribute_value(const void *elem, void *ud)
{
        const struct sensor_attr_list_elem *attr = elem;
        uint8_t *target = ud;
        uint16_t offset = 0;

        // compare this attribute's UUID to make sure data ordering is identical between nodes and read cycles
        // the data frame: [connid][temp][humid][water]
        // TODO: make some something better for this
        if(ble_uuid_equal(&attr->uuid, &node_data_attr_temp)) {
                offset = sizeof(attr->value);
        } else if(ble_uuid_equal(&attr->uuid, &node_data_attr_humid)) {
                offset = sizeof(attr->value)*2;
        } else if(ble_uuid_equal(&attr->uuid, &node_data_attr_water)) {
                offset = sizeof(attr->value)*3;
        } else {
                printf("copy_attribute_value(): unknown attribute uuid: %s\r\n", ble_uuid_to_string(&attr->uuid));
                return;
        }

        memcpy(target+offset, &attr->value, sizeof(attr->value));
}

void copy_node_sensor_data(const void *elem, void *ud)
{
        const struct node_list_elem *node = elem;
        uint16_t *offset = ud;
        uint8_t attribute_data[NODE_SENSOR_DATA_TRANSFER_SIZE] = { 0 };

        // copy the connection id for identification
        // TODO: send the device MAC address as well
        memcpy(attribute_data, &node->conn_idx, sizeof(node->conn_idx));
        // index += 2 effectively; for each attribute as all attributes are 2 bytes
        list_foreach_nonconst(node->attr_list, copy_attribute_value, &attribute_data);

        memcpy(node_data+*offset, attribute_data, sizeof(attribute_data));

        *offset += sizeof(attribute_data);
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
void get_node_data_cb(uint8_t **value, uint16_t *length)
{
        ble_error_t status;
        uint8_t conn_count;
        uint16_t *conn_idx;
        int i;

        printf("get_node_data_cb\r\n");

        /*
         * 1: push all connected nodes in the connected node list
         */
        taskENTER_CRITICAL();
        status = ble_gap_get_connected(&conn_count, &conn_idx);
        taskEXIT_CRITICAL();
        if(status == BLE_STATUS_OK) {
                printf("connected nodes: %d\r\n", conn_count);
        }
        for (i = 0; i < conn_count; i++) {
                // skip if connected device already in list
                if(list_find_node_by_connid(node_devices_connected, conn_idx[i]) != NULL) {
                        continue;
                }
                // append the node to the linked list for later connection
                struct node_list_elem *node = OS_MALLOC(sizeof(*node));

                // initial zero values
                /* zero the struct */
                memset((void *)node, 0x00, sizeof(*node));

                memcpy(&node->conn_idx, &conn_idx[i], sizeof(node->conn_idx));
                list_add(&node_devices_connected, node);
        }
        OS_FREE(conn_idx);

        /*
         * 2: request new node data
         * Initiate a service scan for the node data service, this will trigger
         * a chain of async calls with eventually new sensor data.
         * TODO: this should be done periodically
         */
        att_uuid_t data_svc_uuid;
        ble_uuid_from_string(NODE_DATA_SVC_UUID, &data_svc_uuid);

        list_foreach(node_devices_connected, discover_node_service, &data_svc_uuid);

        /*
         * 3: return (old) node data
         */
        uint16_t offset = 0;
        node_data = OS_MALLOC(list_size(node_devices_connected) * NODE_SENSOR_DATA_TRANSFER_SIZE);
        list_foreach_nonconst(node_devices_connected, copy_node_sensor_data, &offset);

        *value = node_data;
        *length = offset;
}


/*
 * Main code
 */

/*
 * Handler for ble_gap_scan_start call.
 * Initiates a scan procedure.
 * Prints scan parameters and status returned by call
 */
bool gap_scan_start()
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
bool gap_connect(const bd_address_t *addr)
{
        gap_conn_params_t params = CFG_CONN_PARAMS;

        printf("connecting to: %s\r\n", ble_address_to_string(addr));

        // keep trying if busy connecting other node
        while(BLE_ERROR_BUSY == ble_gap_connect(addr, &params)) {
                // Arbitrary delay to not flood the connect
                OS_DELAY_MS(100);
        }

        return true;
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
        list_add(&node_devices_scanned, node);
}

/*
 * Print GAP scan completed event information
 */
void handle_ble_evt_gap_scan_completed(const ble_evt_gap_scan_completed_t *info)
{
        struct node_list_elem *element;

        printf("BlueTanist node scan completed. Found %d nodes\r\n", list_size(node_devices_scanned));

        // connect all found nodes
        while(node_devices_scanned != NULL) {
                element = (struct node_list_elem *) list_pop_back(&node_devices_scanned);
                gap_connect(&element->addr);
                OS_FREE(element);
        }
}

/*
 * Handle service discoverd
 */
void handle_ble_evt_gattc_discover_svc(const ble_evt_gattc_discover_svc_t *info)
{
        ble_error_t status;

        // sensor data service discovered, scan for attributes
        printf("service discovered for %d: %s\r\n", info->conn_idx, ble_uuid_to_string(&info->uuid));
        status = ble_gattc_discover_char(info->conn_idx, info->start_h, info->end_h, NULL);
}

/*
 * Handle characteristic discovered
 */
void handle_ble_evt_gattc_discover_char(const ble_evt_gattc_discover_char_t *info)
{
        ble_error_t status;

        printf("characteristic discovered for %d: %s\r\n", info->conn_idx, ble_uuid_to_string(&info->uuid));

        // add the attribute to the node's attribute list
        struct node_list_elem *node = list_find_node_by_connid(node_devices_connected, info->conn_idx);
        if(node == NULL) {
                return;
        }
        struct sensor_attr_list_elem *elem = OS_MALLOC(sizeof(*elem));
        memcpy(&elem->handle, &info->handle, sizeof(elem->handle));
        memcpy(&elem->uuid, &info->uuid, sizeof(elem->uuid));
        list_add(&node->attr_list, elem);

        // read the attribute
        status = ble_gattc_read(info->conn_idx, info->value_handle, 0);
}

/*
 * Handle characteristic data retrieved
 */
void handle_ble_evt_gattc_read_completed(ble_evt_gattc_read_completed_t *info)
{
        int i;

        printf("characteristic read for %d, length: %d, value: ", info->conn_idx, info->length);
        if(info->status == ATT_ERROR_OK)
        {
                /*
                 * copy the value to the list element
                 * if node or attribute do not yet exist something has gone wrong; ignore it
                 */
                struct node_list_elem *node = list_find_node_by_connid(node_devices_connected, info->conn_idx);
                if(node == NULL) {
                        return;
                }
                // TODO: why is `handle` off by 1?
                struct sensor_attr_list_elem *elem = list_find_attr_by_handle(node->attr_list, info->handle-1);
                if(elem == NULL) {
                        return;
                }
                memcpy(&elem->value, &info->value, sizeof(elem->value));

                for (i = 0; i < info->length; ++i) {
                        printf("%02x", elem->value[i]);
                }
                printf("\r\n");
        }
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
        case BLE_EVT_GATTC_DISCOVER_SVC:
                handle_ble_evt_gattc_discover_svc((ble_evt_gattc_discover_svc_t *) evt);
                break;
        case BLE_EVT_GATTC_DISCOVER_CHAR:
                handle_ble_evt_gattc_discover_char((ble_evt_gattc_discover_char_t *) evt);
                break;
        case BLE_EVT_GATTC_READ_COMPLETED:
                handle_ble_evt_gattc_read_completed((ble_evt_gattc_read_completed_t *) evt);
        }
        return false;
}
