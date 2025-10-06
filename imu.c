/**
 * @file imu.c
 * @brief Implementation of IMU driver
 * 
 * This file contains the implementation of the IMU driver
 * using the LSM6DSO sensor. It includes functions for initializing,
 * configuring, and reading data from the IMU.
 * 
 * @authors Amelia Ellis
 * @date 2025-04-11
 * @version 0.1
 * @bug No known bugs.
 */
#include "imu.h"
//#include "logs/logs_defs.h"



static HAL_StatusTypeDef IMU_ReadReg(uint8_t reg, uint8_t *data, uint16_t len) {
    LSM6DSOX_CS_LOW();
    reg |= 0x80; // Read
    HAL_StatusTypeDef status = HAL_SPI_Transmit(hspi1, &reg, 1, SENSORS_SPI_TIMEOUT);
    if (status != HAL_OK) {
        printf("Error writing to LSM6DSO register 0x%02X: %d\n", reg, status);
        if (status == HAL_TIMEOUT) {
            printf("LSM6DSO SPI write timeout\n");
        } else if (status == HAL_ERROR) {
            printf("LSM6DSO SPI write error\n");
        } else if (status == HAL_BUSY) {
            printf("LSM6DSO SPI is busy\n");
        }
        LSM6DSOX_CS_HIGH();
        return status;
    }
    status = HAL_SPI_Receive(hspi1, data, len, SENSORS_SPI_TIMEOUT);
    if (status != HAL_OK) {
        printf("Error reading from LSM6DSO register 0x%02X: %d\n", reg, status);
        if (status == HAL_TIMEOUT) {
            printf("LSM6DSO SPI read timeout\n");
        } else if (status == HAL_ERROR) {
            printf("LSM6DSO SPI read error\n");
        } else if (status == HAL_BUSY) {
            printf("LSM6DSO SPI is busy\n");
        }
    }
    LSM6DSOX_CS_HIGH();
    return status;
}

static HAL_StatusTypeDef IMU_WriteReg(uint8_t reg, uint8_t data) {
    uint8_t tx[2] = {reg & 0x7F, data}; // Write
    LSM6DSOX_CS_LOW();
    HAL_StatusTypeDef status = HAL_SPI_Transmit(hspi1, tx, 2, SENSORS_SPI_TIMEOUT);
    if (status != HAL_OK) {
        printf("Error writing to LSM6DSO register 0x%02X: %d\n", reg, status);
        if (status == HAL_TIMEOUT) {
            printf("LSM6DSO SPI write timeout\n");
        } else if (status == HAL_ERROR) {
            printf("LSM6DSO SPI write error\n");
        } else if (status == HAL_BUSY) {
            printf("LSM6DSO SPI is busy\n");
        }
    }
    LSM6DSOX_CS_HIGH();
    return status;
}

uint8_t IMU_Init() {
    printf("Initializing IMU...\n");
    uint8_t who_am_i = 0;
    if (IMU_ReadReg(LSM6DSOX_WHO_AM_I_ADDR, &who_am_i, 1) != HAL_OK || who_am_i != LSM6DSOX_WHO_AM_I_ID) {
        //printf("Initialization error: LSM6DSO not found\n");
		printf("Initialization Failed: IMU not responding\n");
        return 1;
    }

    // Reset device
    IMU_WriteReg(LSM6DSOX_CTRL3_C_ADDR, 0x01);
    HAL_Delay(100);

    // Accelerometer: ODR = 104 Hz, FS = ±2g
    IMU_WriteReg(LSM6DSOX_CTRL1_XL_ADDR, 0x40); // 0b01000000

    // Gyroscope: ODR = 104 Hz, FS = ±250 dps
    IMU_WriteReg(LSM6DSOX_CTRL2_G_ADDR, 0x40);

    return 0;
}

uint8_t IMU_Read(vector_t *pData) {
    uint8_t buffer[12];

    if (IMU_ReadReg(LSM6DSOX_OUTX_L_G_ADDR, buffer, 6) != HAL_OK) return 1;
    if (IMU_ReadReg(LSM6DSOX_OUTX_L_XL_ADDR, buffer + 6, 6) != HAL_OK) return 1;

    // Gyroscope (dps)
    pData[1].x = (int16_t)(buffer[1] << 8 | buffer[0]);
    pData[1].y = (int16_t)(buffer[3] << 8 | buffer[2]);
    pData[1].z = (int16_t)(buffer[5] << 8 | buffer[4]);

    // Accelerometer (g)
    pData[0].x = (int16_t)(buffer[7] << 8 | buffer[6]);
    pData[0].y = (int16_t)(buffer[9] << 8 | buffer[8]);
    pData[0].z = (int16_t)(buffer[11] << 8 | buffer[10]);

    return 0;
}