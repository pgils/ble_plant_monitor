/**
 ****************************************************************************************
 *
 * @file custom_config_qspi.h
 *
 * @brief Board Support Package. User Configuration file for cached QSPI mode.
 *
 * Copyright (C) 2015-2019 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */
#ifndef CUSTOM_CONFIG_QSPI_H_
#define CUSTOM_CONFIG_QSPI_H_

#include "bsp_definitions.h"

#define CONFIG_USE_BLE


/* Enable retarget functionality */
#define CONFIG_RETARGET

/*
 * UART2 (HW_UART2) is the default selected UART block. If needed, use the
 * following macro to change to UART1 (HW_UART1) serial block:
 *
 * #define CONFIG_RETARGET_UART  HW_UART1
 */

/*
 * unique byte to identify this devkit
 * this is used to create a unique MAC address and as a base value for dummy data
 */
#define devkitUNIQUE_BYTE       0x1

/*
 * Whether to use dummy data for sensor data
 */
#define USE_DUMMY_DATA          ( 1 )

/* Change the static BLE address of the device */
#define defaultBLE_STATIC_ADDRESS   { devkitUNIQUE_BYTE, 0x01, 0x01, 0x06, 0x06, 0x06 }


/*************************************************************************************************\
 * System configuration
 */
#define dg_configUSE_LP_CLK                     ( LP_CLK_RCX )
#define dg_configEXEC_MODE                      ( MODE_IS_CACHED )
#define dg_configCODE_LOCATION                  ( NON_VOLATILE_IS_FLASH )
#define dg_configEMULATE_OTP_COPY               ( 0 )
//#define dg_configUSER_CAN_USE_TIMER1            ( 0 )

#define dg_configUSE_WDOG                       ( 1 )

#define dg_configFLASH_CONNECTED_TO             ( FLASH_CONNECTED_TO_1V8 )
#define dg_configFLASH_POWER_DOWN               ( 0 )
#define dg_configPOWER_1V8_ACTIVE               ( 1 )
#define dg_configPOWER_1V8_SLEEP                ( 1 )

#define dg_configBATTERY_TYPE                   ( BATTERY_TYPE_LIMN2O4 )
#define dg_configBATTERY_CHARGE_CURRENT         ( 2 )    // 30mA
#define dg_configBATTERY_PRECHARGE_CURRENT      ( 20 )   // 2.1mA
#define dg_configBATTERY_CHARGE_NTC             ( 1 )    // disabled

#define dg_configUSE_USB                        ( 0 )
#define dg_configUSE_USB_CHARGER                ( 0 )
#define dg_configALLOW_CHARGING_NOT_ENUM        ( 1 )

#define dg_configUSE_SW_CURSOR                  ( 1 )

#define dg_configCACHEABLE_QSPI_AREA_LEN        ( NVMS_PARAM_PART_start - MEMORY_QSPIF_BASE )

#define dg_configTESTMODE_MEASURE_SLEEP_CURRENT ( 0 )


/*************************************************************************************************\
 * FreeRTOS configuration
 */
#define OS_FREERTOS                              /* Define this to use FreeRTOS */
#define configTOTAL_HEAP_SIZE                    ( 22972 )   /* FreeRTOS Total Heap Size */

/*************************************************************************************************\
 * Peripherals configuration
 */
#define dg_configFLASH_ADAPTER                  ( 1 )
#define dg_configNVMS_ADAPTER                   ( 1 )
#define dg_configNVMS_VES                       ( 1 )
#define dg_configNVPARAM_ADAPTER                ( 1 )
#define dg_configI2C_ADAPTER                    ( 1 )
#define dg_configUSE_HW_I2C                     ( 1 )

/* RF is not accessed by SysCPU in DA1469x */
#define dg_configRF_ENABLE_RECALIBRATION        ( 1 )

/*************************************************************************************************\
 * Sensor configuration
 */
#define dg_configSENSOR_BMP180                  ( 0 )
#define dg_configSENSOR_HIH6130                 ( 0 )


/*************************************************************************************************\
 * BLE configuration
 */
#define CONFIG_USE_BLE_SERVICES

#define dg_configBLE_CENTRAL                    ( 1 )
#define dg_configBLE_GATT_CLIENT                ( 1 )
#define dg_configBLE_OBSERVER                   ( 1 )
#define dg_configBLE_BROADCASTER                ( 1 )
#define dg_configBLE_L2CAP_COC                  ( 1 )

#define defaultBLE_ATT_DB_CONFIGURATION         ( 0x30 )
#define CFG_AUTO_CONN_PARAM_REPLY               ( 1 )
#define CFG_AUTO_PAIR_REPLY                     ( 1 )

/* Include bsp default values */
#include "bsp_defaults.h"
/* Include middleware default values */
#include "middleware_defaults.h"
/* Include memory layout */
//#include "bsp_memory_layout.h"

#endif /* CUSTOM_CONFIG_QSPI_H_ */
