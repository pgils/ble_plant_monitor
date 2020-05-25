/*
 * ble_central_functions.h
 *
 *  Created on: May 25, 2020
 *      Author: ssuser
 */

#ifndef BLE_CENTRAL_FUNCTIONS_H_
#define BLE_CENTRAL_FUNCTIONS_H_

#include <stdbool.h>

void get_node_data_cb(uint8_t **value, uint16_t *length);
bool gap_scan_start();
bool gap_connect(const bd_address_t *addr);
void handle_ble_evt_gap_adv_report(ble_evt_gap_adv_report_t *info);
void handle_ble_evt_gap_scan_completed(const ble_evt_gap_scan_completed_t *info);
bool pmp_ble_handle_event(const ble_evt_hdr_t *evt);

#endif /* BLE_CENTRAL_FUNCTIONS_H_ */
