# BLE Plant Monitor (prototype)
1. clone this reposity to `<DIALOG SDK>/projects`  
   e.g:
   ```
   $ cd ~/DIALOG/SDK_10.0.8.105/projects
   $ git clone git@github.com:pgils/ble_plant_monitor.git
   ```
2. Update drivers (if needed; currently only BMP180 uses external driver)
   ```
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