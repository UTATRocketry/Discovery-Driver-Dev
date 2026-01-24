/*
 * NEOM9N.h
 *
 *  Created on: Mar 18, 2025
 *      Author: William Gomez
 *      Modified by: Pierce Luu
 */

 #ifndef INC_NEOM9N_H_
 #define INC_NEOM9N_H_
 
 #include "stm32l4xx_hal.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdlib.h>
 #include <stdint.h>

extern UART_HandleTypeDef* uartAddress;

int NEOM9N_init(UART_HandleTypeDef* uartAddressPin);
int NEOM9N_getData(unsigned char *GPSData);
int NEOM9N_isDataReady(void);
void NEOM9N_getDiagnostics(uint32_t* interrupt_count_out, uint32_t* total_bytes_out);
int NEOM9N_getTime(float* time, unsigned char* GPSData, int size);
int NEOM9N_getSpeed(float* speed, unsigned char* GPSData);
int NEOM9N_getPosition(float* latitude, char* latitudeHemisphere, 
                       float* longitude, char* longitudeHemisphere, 
                       unsigned char* GPSData, int size);
int NEOM9N_getAltitude(float* altitude, unsigned char* GPSData);
 
 #endif /* INC_NEOM9N_H_ */