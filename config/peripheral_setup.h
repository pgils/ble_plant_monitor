/*
 * peripheral_setup.h
 *
 *  Created on: Mar 18, 2020
 *      Author: ssuser
 */

#ifndef CONFIG_PERIPHERAL_SETUP_H_
#define CONFIG_PERIPHERAL_SETUP_H_

#define BMP180_I2C_ADDRESS    ( 0xEE >> 1 )

/**
 * I2C 1 configuration
 */
#define I2C1_SCL_PORT       ( HW_GPIO_PORT_0 )
#define I2C1_SCL_PIN        ( HW_GPIO_PIN_30 )

#define I2C1_SDA_PORT       ( HW_GPIO_PORT_0 )
#define I2C1_SDA_PIN        ( HW_GPIO_PIN_31 )

#endif /* CONFIG_PERIPHERAL_SETUP_H_ */
