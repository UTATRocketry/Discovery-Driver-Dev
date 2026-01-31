/*
 * NEOM9N.h
 *
 *  Created on: Mar 18, 2025
 *      Author: willi
 */

#ifndef INC_NEOM9N_H_
#define INC_NEOM9N_H_

#include "stm32l4xx_hal.h" /*Needed for UART*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

extern UART_HandleTypeDef* uartAddress;

/* Driver Functions */

//Initializes NEO-M9N, returns 0 if successful, 1 if failed
int NEOM9N_init(UART_HandleTypeDef* uartAddressPin);

//Checks if the NEO-M9N is connected to the UART line, returns 0 if successful, 1 if failed
int NEOM9N_CheckConnection();

//Returns current state of GPS

int NEOM9N_getData();
int NEOM9N_getTime();
int NEOM9N_getSpeed();
int NEOM9N_getPosition();
int NEOM9N_getAltitude();

#endif /* INC_NEOM9N_H_ */
