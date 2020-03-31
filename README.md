# BlueTanist plant monitor (MCU)

Based on the [DA1469x Custom bluetooth service][1] example project.

# Introduction
This application reads values from i2c-connected sensors (which monitor a plant's environment) and provides this data via a BLE GATT service.

The [BlueTanist android app][2] can be used to interact with this application.

[1]:https://www.dialog-semiconductor.com/sites/default/files/da1469x_ble_custom_service_sample_code.zip
[2]:https://github.com/pgils/ble_plant_monitor_android

# Pre-requisites
- Dialog DA14695 Development Kit – USB
- Dialog SmartSnippets
- Dialog SDK >= 10.0.8.105

# Getting started

1. clone this reposity to `<DIALOG SDK>/projects`  
   ```
   $ cd ~/DIALOG/SDK_10.0.8.105/projects
   $ git clone https://github.com/pgils/ble_plant_monitor.git
   ```
2. Update drivers (if needed; currently only BMP180 uses external driver)
   ```
   $ cd ble_plant_monitor
   $ git submodule init
   $ git submodule update
   ```
3. Import project into SmartSnippets
4. Enable needed drivers in `custom_config_xxx.h`  
   e.g:
   ```
   #define dg_configSENSOR_BMP180                  ( 0 )
   #define dg_configSENSOR_HIH6130                 ( 1 )
   ```
