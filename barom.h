/**
 * @file barom.h
 * @brief Header file for barometer driver
 * 
 * This file contains the definitions and function prototypes for
 * initializing and reading data from the barometer. It includes
 * the necessary structures, function prototypes, and constants
 * required for the barometer driver.
 * 
 * @authors Amelia Ellis
 * @date 2025-04-11
 * @version 0.1
 * @bug No known bugs.
 */
#ifndef BAROM_H
#define BAROM_H

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "stm32h7xx_hal.h"
#include "sensors_defs.h"


#define MS5611_CMD_RESET      0x1E
#define MS5611_CMD_ADC_READ   0x00
#define MS5611_CMD_PROM_READ  0xA0
#define MS5611_CMD_CONVERT_D1 0x48  // Pressure (OSR = 4096)
#define MS5611_CMD_CONVERT_D2 0x58  // Temperature (OSR = 4096)

// Pin Definitions
#define MS5611_CS_PORT GPIOG
#define MS5611_CS_PIN GPIO_PIN_14
#define MS5611_CS_LOW()   HAL_GPIO_WritePin(MS5611_CS_PORT, MS5611_CS_PIN, GPIO_PIN_RESET)
#define MS5611_CS_HIGH()  HAL_GPIO_WritePin(MS5611_CS_PORT, MS5611_CS_PIN, GPIO_PIN_SET)


static uint8_t ms5611_spi_write(uint8_t cmd);
static uint8_t ms5611_spi_read(uint8_t cmd, uint8_t *buf, uint8_t len);
static uint32_t ms5611_read_adc(uint8_t cmd);


uint8_t Barom_Init();
uint8_t Barom_Read(vector_t *pData);
uint8_t Barom_SetConfig(uint8_t *settings);
uint8_t Barom_GetConfig();
uint8_t Barom_Test(Sensor_t *self);

#endif // BAROM_H