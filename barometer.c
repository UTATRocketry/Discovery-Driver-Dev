/*
 * File Name: barometer.c
 * Author: Sebastian Southworth
 * Description: SPI driver for MS5611 barometer. Displays temperature, pressure, and altitude.
 * Date: 2026-1-25
 *
 */

#include "barometer.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* --- Private Defines --- */
#define CMD_RESET 0x1E
#define CMD_ADC   0x00
#define CMD_PROM  0xA0

/* --- Private Static Helpers --- */

static void MS5611_CS_Select(MS5611_t *dev) {
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
}

static void MS5611_CS_Unselect(MS5611_t *dev) {
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
}

/**
 * @brief Internal helper to trigger conversion and read 24-bit ADC result.
 * @param base_cmd 0x40 for Pressure (D1), 0x50 for Temperature (D2).
 */
static uint32_t MS5611_GetRaw(MS5611_t *dev, uint8_t base_cmd, MS5611_OSR_t osr) {
    uint8_t start_cmd = base_cmd + (uint8_t)osr;
    uint8_t read_cmd[4] = {CMD_ADC, 0, 0, 0};
    uint8_t rx[4];

    // 1. Start Conversion
    MS5611_CS_Select(dev);
    HAL_SPI_Transmit(dev->hspi, &start_cmd, 1, HAL_MAX_DELAY);
    MS5611_CS_Unselect(dev);

    // 2. Wait for Conversion (10ms is safe for OSR 4096)
    HAL_Delay(10);

    // 3. Read ADC
    MS5611_CS_Select(dev);
    HAL_SPI_TransmitReceive(dev->hspi, read_cmd, rx, 4, HAL_MAX_DELAY);
    MS5611_CS_Unselect(dev);

    return ((uint32_t)rx[1] << 16) | ((uint32_t)rx[2] << 8) | rx[3];
}

/* --- Public API Implementation --- */

HAL_StatusTypeDef MS5611_Init(MS5611_t *dev, SPI_HandleTypeDef *hspi, GPIO_TypeDef *port, uint16_t pin) {
    dev->hspi    = hspi;
    dev->cs_port = port;
    dev->cs_pin  = pin;

    // Reset
    uint8_t reset = CMD_RESET;
    MS5611_CS_Select(dev);
    HAL_SPI_Transmit(dev->hspi, &reset, 1, HAL_MAX_DELAY);
    MS5611_CS_Unselect(dev);
    HAL_Delay(5);

    // Read PROM Coefficients (C0-C7)
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t tx[3] = {CMD_PROM + (i * 2), 0, 0};
        uint8_t rx[3];
        MS5611_CS_Select(dev);
        HAL_SPI_TransmitReceive(dev->hspi, tx, rx, 3, HAL_MAX_DELAY);
        MS5611_CS_Unselect(dev);
        dev->prom[i] = (uint16_t)((rx[1] << 8) | rx[2]);
    }

    return HAL_OK;
}

BaroData MS5611_Readings(MS5611_t *dev, MS5611_OSR_t osr) {
    uint32_t D1 = MS5611_GetRaw(dev, 0x40, osr);
    uint32_t D2 = MS5611_GetRaw(dev, 0x50, osr);

    // First Order Temperature
    int32_t dT   = (int32_t)D2 - ((int32_t)dev->prom[5] << 8);
    int32_t temp = 2000 + (int32_t)(((int64_t)dT * dev->prom[6]) >> 23);

    // Second Order Compensation
    int64_t T2 = 0, OFF2 = 0, SENS2 = 0;
    if (temp < 2000) {
        T2 = ((int64_t)dT * dT) >> 31;
        int64_t diff = (int64_t)temp - 2000;
        OFF2 = (5 * (diff * diff)) >> 1;
        SENS2 = (5 * (diff * diff)) >> 2;

        if (temp < -1500) {
            int64_t v_diff = (int64_t)temp + 1500;
            OFF2 += 7 * (v_diff * v_diff);
            SENS2 += (11 * (v_diff * v_diff)) >> 1;
        }
    }

    // Pressure Calculation (using 64-bit for offsets)
    int64_t off  = ((int64_t)dev->prom[2] << 16) + (((int64_t)dev->prom[4] * dT) >> 7) - OFF2;
    int64_t sens = ((int64_t)dev->prom[1] << 15) + (((int64_t)dev->prom[3] * dT) >> 8) - SENS2;

    int32_t press = (int32_t)(((((int64_t)D1 * sens) >> 21) - off) >> 15);

    // Final Struct Assignment
    BaroData data;
    data.temp     = (double)(temp - (int32_t)T2) / 100.0;
    data.pressure = (double)press / 100.0;
    data.altitude = 44330.0 * (1.0 - pow((data.pressure / 1013.25), 0.1902949));

    return data;
}

void MS5611_Print(UART_HandleTypeDef *huart, BaroData data) {
    char msg[128];
    int len = snprintf(msg, sizeof(msg),
             "\r\nTemp:     %6.2f C\r\n"
             "Pressure: %6.2f mbar\r\n"
             "Altitude: %6.2f m\r\n"
             "------------------------\r\n",
             data.temp, data.pressure, data.altitude);

    HAL_UART_Transmit(huart, (uint8_t*)msg, len, HAL_MAX_DELAY);
}
