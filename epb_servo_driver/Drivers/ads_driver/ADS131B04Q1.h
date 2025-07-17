/*
 * ADS131B04Q1.h
 *
 *  Created on: Jan 31, 2025
 *      Author: William Gomez
 */

#ifndef ADS131B04Q1_DRIVER_ADS131B04Q1_H_
#define ADS131B04Q1_DRIVER_ADS131B04Q1_H_

#include "stm32l4xx_hal.h" /* Needed for SPI */

extern SPI_HandleTypeDef* ADS131_hspi;
extern GPIO_TypeDef* ADS131pinLetter;
extern uint16_t ADS131_PIN_Number;


/* Driver Functions */

//Initializes ADS131B04-Q1, returns 0 if successful, 1 if failed
int ADS131B04Q1_Init(SPI_HandleTypeDef* hspi, GPIO_TypeDef* GPIOpinLetter, uint16_t GPIO_PIN_Number);

// Checks if the ADS131B04-Q1 is connected to the SPI bus, returns 0 if successful, 1 if failed
int ADS131B04Q1_CheckConnection();

// intakes a value between 0 and 7, which corresponds to an OSR setting, and configures ADC to match
// returns 0 if successful, 1 if failed (or if parameter was not within [0,7])
int ADS131B04Q1_OSRConfig(int configNum);

// Reads voltage calculated from ADS131B04-Q1, returns 0 if successful, 1 if failed
// Writes signed 2s complement number as a fraction of range to each channelReading pointer
int ADS131B04Q1_ReadChannels(int32_t *channelReading1, int32_t *channelReading2, int32_t *channelReading3, int32_t *channelReading4);

// Manually calibrates gain for a specific channel based on input values, returns 0 if successful
int ADS131B04Q1_Calibrate(float x1, float y1, float x2, float y2, int channelNum);

// Applies gain, offset, and conversion to 2s complement value, returns float
float ADS131B04Q1_RawToVoltage(int channelReading, int channelNum);

#endif /* ADS131B04Q1_DRIVER_ADS131B04Q1_H_ */
