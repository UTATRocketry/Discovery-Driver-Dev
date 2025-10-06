/**
 * @file barom.c
 * @brief Implementation of barometer driver
 * 
 * This file contains the implementation of the barometer driver
 * using the MS5611 or a fake barometer. It includes functions for
 * initializing, configuring, and reading data from the barometer.
 * 
 * @authors Amelia Ellis
 * @date 2025-04-11
 * @version 0.1
 * @bug No known bugs.
 */
#include "barom.h"



static uint16_t prom[6]; // Calibration coefficients

HAL_StatusTypeDef ms5611_spi_write(uint8_t cmd) {
    HAL_StatusTypeDef status;
    MS5611_CS_LOW();
    status = HAL_SPI_Transmit(hspi1, &cmd, 1, SENSORS_SPI_TIMEOUT);
    if (status != HAL_OK) {
        printf("Error writing to MS5611: %d\n", status);
        if (status == HAL_TIMEOUT) {
            printf("MS5611 SPI write timeout\n");
        } else if (status == HAL_ERROR) {
            printf("MS5611 SPI write error\n");
        } else if (status == HAL_BUSY) {
            printf("MS5611 SPI is busy\n");
        }
    }
    MS5611_CS_HIGH();
    return status;
}

HAL_StatusTypeDef ms5611_spi_read(uint8_t cmd, uint8_t *buf, uint8_t len) {
    HAL_StatusTypeDef status;
    MS5611_CS_LOW();
    status = HAL_SPI_Transmit(hspi1, &cmd, 1, SENSORS_SPI_TIMEOUT);
    if (status != HAL_OK) {
        printf("Error writing to MS5611: %d\n", status);
        if (status == HAL_TIMEOUT) {
            printf("MS5611 SPI write timeout\n");
        } else if (status == HAL_ERROR) {
            printf("MS5611 SPI write error\n");
        } else if (status == HAL_BUSY) {
            printf("MS5611 SPI is busy\n");
        }
    }
    status = HAL_SPI_Receive(hspi1, buf, len, SENSORS_SPI_TIMEOUT);
    if (status != HAL_OK) {
        printf("Error reading from MS5611: %d\n", status);
        if (status == HAL_TIMEOUT) {
            printf("MS5611 SPI read timeout\n");
        } else if (status == HAL_ERROR) {
            printf("MS5611 SPI read error\n");
        } else if (status == HAL_BUSY) {
            printf("MS5611 SPI is busy\n");
        }
    }
    MS5611_CS_HIGH();
    return status;
}

uint32_t ms5611_read_adc(uint8_t cmd) {
    ms5611_spi_write(cmd);
    DelayMs(10); // Wait for conversion (8.22ms max for OSR=4096)

    uint8_t buf[3];
    ms5611_spi_read(MS5611_CMD_ADC_READ, buf, 3);
    return (buf[0] << 16) | (buf[1] << 8) | buf[2];
}

uint8_t Barom_Init() {
	printf("Initializing Barometer...\n");
    ms5611_spi_write(MS5611_CMD_RESET);
    if (ms5611_spi_write(MS5611_CMD_RESET) != HAL_OK)
        //printf("Initialization error: MS5611 not found\n");
		printf("Initialization Failed: Barometer not responding\n");
		return 1; // Device ID mismatch
    DelayMs(3);

    for (int i = 0; i < 6; i++) {
        uint8_t buf[2];
        ms5611_spi_read(MS5611_CMD_PROM_READ + ((i + 1) * 2), buf, 2);
        prom[i] = (buf[0] << 8) | buf[1];
    }

    // Basic sanity check: make sure PROM isn't all zeros
    for (int i = 0; i < 6; i++) {
        if (prom[i] == 0x0000 || prom[i] == 0xFFFF) {
            printf("Initialization error: MS5611 PROM coefficients invalid\n");
            //printf("Initialization Failed: Barometer PROM coefficients invalid\n");
            return 1; // Failure
        }
    }

    return 0; // Success
}

uint8_t Barom_Read(vector_t *pData) {
    uint32_t D1 = ms5611_read_adc(MS5611_CMD_CONVERT_D1);
    uint32_t D2 = ms5611_read_adc(MS5611_CMD_CONVERT_D2);

    int32_t dT = D2 - ((uint32_t)prom[4] << 8);
    int32_t TEMP = 2000 + ((int64_t)dT * prom[5]) / (1 << 23);

    int64_t OFF  = ((int64_t)prom[1] << 16) + ((int64_t)prom[3] * dT) / (1 << 7);
    int64_t SENS = ((int64_t)prom[0] << 15) + ((int64_t)prom[2] * dT) / (1 << 8);
    int32_t P = (((int64_t)D1 * SENS) / (1 << 21) - OFF) / (1 << 15);

    pData->x = (float)P / 100.0f;    // Pressure in mbar
    pData->y = (float)TEMP / 100.0f; // Temperature in °C
    pData->z = 0;

    return 0;
}



uint8_t Barom_Test() {
	printf("Starting MS5611 Driver Test\n");
	// Initialize MS5611
	if (Barom_Init() != 0) {
		// Handle error: MS5611 not found
		printf("Initialization error: MS5611 not found\n");
	    return 1;
	}
	printf("MS5611 Initialized Successfully \n");
	DelayMs(1000);
	vector_t baromData;
	if (Barom_Read(&baromData) != 0) {
		printf("Read Error: MS5611 Read Failed ");
		return 1;
	}
	printf("Pressure: %4.2f mbar, Temperature: %4.2f °C\n", baromData.x, baromData.y);
	return 0;
}
