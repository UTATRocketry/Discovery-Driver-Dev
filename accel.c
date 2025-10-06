/**
 * @file accel.c
 * @brief Implementation of accelerometer sensor functions
 * 
 * This file contains the definitions for functions to initialize,
 * configure, and read data from the accelerometer sensor.
 * 
 * @authors Amelia Ellis
 * @date 2025-04-10
 * @version 0.1
 */

#include "accel.h"
#include "sensors_defs.h"



static void adxl375_write_reg(uint8_t reg, uint8_t value) {
    uint8_t tx[] = { reg, value };
    HAL_StatusTypeDef status;
    ADXL375_CS_LOW();
    status = HAL_SPI_Transmit(hspi1, tx, 2, SENSORS_SPI_TIMEOUT);
    if (status != HAL_OK) {
        printf("Error writing to ADXL375 register 0x%02X: %d\n", reg, status);
        if (status == HAL_TIMEOUT) {
            printf("ADXL375 SPI write timeout\n");
        } else if (status == HAL_ERROR) {
            printf("ADXL375 SPI write error\n");
        } else if (status == HAL_BUSY) {
            printf("ADXL375 SPI is busy\n");
        }
    }
    ADXL375_CS_HIGH();
}

static uint8_t adxl375_read_reg(uint8_t reg) {
    uint8_t tx = 0x80 | reg;
    uint8_t rx;
    HAL_StatusTypeDef status;
    ADXL375_CS_LOW();
    status = HAL_SPI_Transmit(hspi1, &tx, 1, SENSORS_SPI_TIMEOUT);
    if (status != HAL_OK) {
        printf("Error reading ADXL375 register 0x%02X: %d\n", reg, status);
        if (status == HAL_TIMEOUT) {
            printf("ADXL375 SPI read timeout\n");
        } else if (status == HAL_ERROR) {
            printf("ADXL375 SPI read error\n");
        } else if (status == HAL_BUSY) {
            printf("ADXL375 SPI is busy\n");
        }
    }
    status = HAL_SPI_Receive(hspi1, &rx, 1, SENSORS_SPI_TIMEOUT);
    if (status != HAL_OK) {
        printf("Error receiving data from ADXL375 register 0x%02X: %d\n", reg, status);
        if (status == HAL_TIMEOUT) {
            printf("ADXL375 SPI receive timeout\n");
        } else if (status == HAL_ERROR) {
            printf("ADXL375 SPI receive error\n");
        } else if (status == HAL_BUSY) {
            printf("ADXL375 SPI is busy\n");
        }
    }
    ADXL375_CS_HIGH();
    return rx;
}

static void adxl375_read_axes(int16_t *x, int16_t *y, int16_t *z) {
    uint8_t tx = 0xC0 | DATAX0; // multibyte + read
    uint8_t rx[6];

    ADXL375_CS_LOW();
    HAL_SPI_Transmit(hspi1, &tx, 1, SENSORS_SPI_TIMEOUT);
    HAL_SPI_Receive(hspi1, rx, 6, SENSORS_SPI_TIMEOUT);
    ADXL375_CS_HIGH();

    *x = (int16_t)((rx[1] << 8) | rx[0]);
    *y = (int16_t)((rx[3] << 8) | rx[2]);
    *z = (int16_t)((rx[5] << 8) | rx[4]);
}

uint8_t Accel_Init() {
	printf("Initializing Accelerometer...\n");
    if (adxl375_read_reg(DEVID_REG) != ADXL375_DEVICE_ID) {
        //printf("Initialization error: ADXL375 not found\n");
    	printf("Initialization Failed: Accelerometer not responding\n");
        return 1; // Device ID mismatch
    }
    
    // Setup: full resolution, ±200g range (use 0x0B = 0b00001011)
    adxl375_write_reg(DATA_FORMAT, 0x0B);

    // Set output data rate: 800 Hz (value = 0x0D)
    adxl375_write_reg(BW_RATE, 0x0D);

    // Enter measurement mode
    adxl375_write_reg(POWER_CTL, 0x08);

    //printf("ADXL375 Initialized Successfully\n");
    return 0;
}

uint8_t Accel_Read(vector_t *pData) {
    int16_t x, y, z;
    adxl375_read_axes(&x, &y, &z);

    // Convert from raw LSB to g (49 mg/LSB) then to m/s²
    const float scale = 0.049f * 9.80665f;
    pData->x = x * scale;
    pData->y = y * scale;
    pData->z = z * scale;

    return 0;
}



uint8_t Accel_Test() {
    printf("Starting ADXL375 Driver Test\n");
    // Initialize ADXL375
    if (Accel_Init() != 0) {
        // Handle error: ADXL375 not found
        printf("Initialization error: ADXL375 not found\n");
        return 1;
    }
    printf("ADXL375 Initialized Successfully \n");
    DelayMs(1000);
    vector_t accelData;
    if (Accel_Read(&accelData) != 0) {
    	printf("Read Error: ADXL375 Read Failed ");
    	return 1;
    }
    printf("X: %4.2f Y: %4.2f Z: %4.2f \n", accelData.x, accelData.y, accelData.z);
    return 0;
}
