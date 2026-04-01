/*
 * NEOM9N.h
 *
 *  Created on: Mar 18, 2025
 *      Author: willi
 *      (modified by aisha, and prithika)
 */

#ifndef INC_NEOM9N_H_
#define INC_NEOM9N_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stm32l4xx_hal.h" /*Needed for UART*/

/*
    Functions:
    Initialize device
    Is the device on?
    getState (return 0 if ready, enum for other states)
    getData
    getPosition
    getVelocity
    getAltitude
    getNumOfSatellites
 */

/*
 * UART instance used by the GPS driver.
 * NOTE: This is set inside NEOM9N_init().
 */
extern UART_HandleTypeDef* uartAddress;

/*
 * Interrupt-based RX buffers/flags used by the driver.
 * main.c / HAL callbacks will interact with these through the functions below.
 */
extern volatile uint8_t NEOM9N_rxReady;
extern volatile uint16_t NEOM9N_rxSize;

/* Driver Functions */

// Initializes NEO-M9N, returns 0 if successful, 1 if failed
int NEOM9N_init(UART_HandleTypeDef* uartAddressPin);

// Checks if the NEO-M9N is connected to the UART line, returns 0 if successful, 1 if failed
int NEOM9N_CheckConnection();

/*
 * Starts UART reception using interrupt method (Receive-to-IDLE).
 * Call once after NEOM9N_init() in main.c.
 */
void NEOM9N_StartRxIT(void);

/*
 * Called from HAL_UARTEx_RxEventCallback when UART receives bytes (idle event).
 * This function stores incoming bytes into the driver buffer.
 */
void NEOM9N_OnRxEventIT(uint16_t size);

/*
 * Non-blocking "getData" for interrupt method:
 * Copies the most recent received chunk into GPSData (null-terminated).
 * Returns HAL_OK if new data was copied, HAL_BUSY if nothing new yet.
 */
HAL_StatusTypeDef NEOM9N_getDataIT(unsigned char* GPSData, uint16_t maxLen);

#endif /* INC_NEOM9N_H_ */
