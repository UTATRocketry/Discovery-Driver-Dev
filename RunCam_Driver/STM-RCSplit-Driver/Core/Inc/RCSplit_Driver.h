/*
 * RCSplit_Driver.h
 *
 *  Created on: Jan 21, 2025
 *      Author: Amelia Ellis
 */

#ifndef INC_RCSPLIT_DRIVER_H_
#define INC_RCSPLIT_DRIVER_H_



#include "stm32l4xx_hal.h"

/*
 * resources used:
 *
 * (1) RunCam device protocol: https://support.runcam.com/hc/en-us/articles/360014537794-RunCam-Device-Protocol
 * (2) Manufacturer Manual: https://www.runcam.com/download/split4k/RC_Split_4k_Manual_EN.pdf
 * (3) Getting Started with UART: https://wiki.st.com/stm32mcu/wiki/Getting_started_with_UART
 * (4) Implementing UART Tx and Rx: https://community.st.com/t5/stm32-mcus/implementing-uart-receive-and-transmit-functions-on-an-stm32/ta-p/694926
 *
 */

/*---REGISTERS---*/

// WHO_AM_I register, used to verify the device is connected
// #define WHO_AM_I_ADDR 0x75


/*---DRIVER FUNCTIONS SECTION---*/

// Initialize the Run Cam, returns 0 if successful, 1 if failed
int runcam_init(UART_HandleTypeDef* huart3);

// Checks if the Run Cam is connected, returns 0 if successful, 1 if failed
int runcam_check_connection(UART_HandleTypeDef* huart3);

// Read device information
int runcam_get_device_info(UART_HandleTypeDef* huart3, uint8_t* p_device_info);



#endif /* INC_RCSPLIT_DRIVER_H_ */
