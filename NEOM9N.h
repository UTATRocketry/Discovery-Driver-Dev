/*
 * NEOM9N.h
 *
 *  Created on: Mar 18, 2025
 *      Author: William Gomez
 *      Modified by: Pierce Luu
 */

 #ifndef INC_NEOM9N_H_
 #define INC_NEOM9N_H_
 
 #include "stm32l4xx_hal.h" /*Needed for UART*/
 #include <stdio.h>
 #include <string.h>
 #include <stdlib.h>

extern UART_HandleTypeDef* uartAddress;

/* Driver Functions */

//Initializes NEO-M9N, returns 0 if successful, 1 if failed
int NEOM9N_init(UART_HandleTypeDef* uartAddressPin);

//Gets GPS data from interrupt-based buffer
//Returns HAL_OK if new data available, HAL_ERROR if no new data
int NEOM9N_getData(unsigned char *GPSData);

//Checks if new GPS data is ready
int NEOM9N_isDataReady(void);

//Parse functions - extract specific data from GPS buffer
int NEOM9N_getTime(float* time, unsigned char* GPSData, int size);
int NEOM9N_getSpeed(float* speed, unsigned char* GPSData);
int NEOM9N_getPosition(float* latitude, char* latitudeHemisphere, 
                       float* longitude, char* longitudeHemisphere, 
                       unsigned char* GPSData, int size);
int NEOM9N_getAltitude(float* altitude, unsigned char* GPSData);
 
 #endif /* INC_NEOM9N_H_ */