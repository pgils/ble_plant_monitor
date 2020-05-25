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
#include "stdio.h"
#include "ble_bluetanist_common.h"

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
 * Main code
 */


void handle_evt_gap_connected(ble_evt_gap_connected_t *evt)
{
        printf("gap connected: %s\r\n", ble_address_to_string(&evt->peer_address));
        printf("my address: %s\r\n", ble_address_to_string(&evt->own_addr));
}

void handle_evt_gap_disconnected(ble_evt_gap_disconnected_t *evt)
{
        /*
         * Manage disconnection information
         */
}

void handle_evt_gap_adv_completed(ble_evt_gap_adv_completed_t *evt)
{
        // restart advertising so we can connect again
        ble_gap_adv_start(GAP_CONN_MODE_UNDIRECTED);
}

/*
 * Handle an initiated connection completed event
 */
void handle_ble_evt_gap_connection_completed(const ble_evt_gap_connection_completed_t *info)
{
        printf("BLE_EVT_GAP_CONNECTION_COMPLETED\r\n");
        printf("Status: 0x%02x\r\n", info->status);
}
