/*
 * File Name: barometer.h
 * Author: Sebastian Southworth
 * Description: SPI driver for MS5611 barometer. Displays temperature, pressure, and altitude.
 * Date: 2026-1-25
 *
 */

#ifndef BAROMETER_H_
#define BAROMETER_H_

#include "stm32l4xx_hal.h"
#include <stdint.h>

/**
 * @brief Over Sampling Ratio (OSR) offsets for D1 and D2 commands.
 */
typedef enum {
    MS5611_OSR_256  = 0x00,
    MS5611_OSR_512  = 0x02,
    MS5611_OSR_1024 = 0x04,
    MS5611_OSR_2048 = 0x06,
    MS5611_OSR_4096 = 0x08
} MS5611_OSR_t;

/**
 * @brief Device Handle to allow multiple sensors on different pins/buses.
 */
typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef      *cs_port;
    uint16_t           cs_pin;
    uint16_t           prom[8];
} MS5611_t;

/**
 * @brief Final processed sensor data.
 */
typedef struct {
    double temp;
    double pressure;
    double altitude;
} BaroData;

/* --- API Function Prototypes --- */

HAL_StatusTypeDef MS5611_Init(MS5611_t *dev, SPI_HandleTypeDef *hspi, GPIO_TypeDef *port, uint16_t pin);
BaroData          MS5611_Readings(MS5611_t *dev, MS5611_OSR_t osr);
void              MS5611_Print(UART_HandleTypeDef *huart, BaroData data);

#endif /* BAROMETER_H_ */
