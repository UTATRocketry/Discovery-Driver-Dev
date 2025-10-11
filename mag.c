/**
 * @file mag.c
 * @brief Implementation of magnetometer driver
 * 
 * This file contains the implementation of the magnetometer driver
 * using the LIS2MDL sensor. It includes functions for initializing,
 * configuring, and reading data from the magnetometer.
 * 
 * @authors Amelia Ellis, Eric Chiang
 * @date 2025-04-11
 * @version 0.1
 * @bug No known bugs.
 */
#include "mag2.h"

/**
 * @brief Write a single byte to a LIS2MDL register.
 * @param reg: Register address.
 * @param value: Value to write.
 */
void LIS2MDL_WriteRegister(uint8_t addr, uint8_t value) {
    HAL_StatusTypeDef status;
    uint8_t txData[2] = {addr & 0x7F, value};  // MSB 0 for write
    LIS2MDL_CS_LOW();
    status = HAL_SPI_Transmit(&hspi1, txData, sizeof(txData), SENSORS_SPI_TIMEOUT);
    if (status != HAL_OK) {
        printf("Error writing to LIS2MDL register 0x%02X: %d\n", addr, status);
        if (status == HAL_TIMEOUT) {
            printf("LIS2MDL SPI write timeout\n");
        } else if (status == HAL_ERROR) {
            printf("LIS2MDL SPI write error\n");
        } else if (status == HAL_BUSY) {
            printf("LIS2MDL SPI is busy\n");
        }
    }
    LIS2MDL_CS_HIGH();
}

/**
 * @brief Read a single byte from a LIS2MDL register.
 * @param reg: Register address.
 * @return The value read.
 */
uint8_t LIS2MDL_ReadRegister(uint8_t addr) {
    HAL_StatusTypeDef status;
    uint8_t txData = reg | 0x80;  // MSB 1 for read
    uint8_t rxData;
    LIS2MDL_CS_LOW();
    status = HAL_SPI_Transmit(&hspi1, &txData, 1, SENSORS_SPI_TIMEOUT);
    if (status != HAL_OK) {
        printf("Error writing to LIS2MDL register 0x%02X: %d\n", addr, status);
        if (status == HAL_TIMEOUT) {
            printf("LIS2MDL SPI write timeout\n");
        } else if (status == HAL_ERROR) {
            printf("LIS2MDL SPI write error\n");
        } else if (status == HAL_BUSY) {
            printf("LIS2MDL SPI is busy\n");
        }
    }
    status = HAL_SPI_Receive(&hspi1, &rxData, 1, SENSORS_SPI_TIMEOUT);
    if (status != HAL_OK) {
        printf("Error reading from LIS2MDL register 0x%02X: %d\n", addr, status);
        if (status == HAL_TIMEOUT) {
            printf("LIS2MDL SPI read timeout\n");
        } else if (status == HAL_ERROR) {
            printf("LIS2MDL SPI read error\n");
        } else if (status == HAL_BUSY) {
            printf("LIS2MDL SPI is busy\n");
        }
    }
    LIS2MDL_CS_HIGH();
    return rxData;
}

/**
 * @brief Read multiple bytes starting from a LIS2MDL register.
 * @param reg: Starting register address.
 * @param buffer: Buffer to store the data.
 * @param len: Number of bytes to read.
 */
void LIS2MDL_ReadRegisters(uint8_t addr, uint8_t *buffer, uint8_t len) {
    HAL_StatusTypeDef status;
    uint8_t txData = addr | 0x80;  // MSB 1 for read
    LIS2MDL_CS_LOW();
    status = HAL_SPI_Transmit(&hspi1, &txData, 1, SENSORS_SPI_TIMEOUT);
    if (status != HAL_OK) {
        printf("Error writing to LIS2MDL register 0x%02X: %d\n", addr, status);
        if (status == HAL_TIMEOUT) {
            printf("LIS2MDL SPI write timeout\n");
        } else if (status == HAL_ERROR) {
            printf("LIS2MDL SPI write error\n");
        } else if (status == HAL_BUSY) {
            printf("LIS2MDL SPI is busy\n");
        }
    }
    status = HAL_SPI_Receive(&hspi1, buffer, len, SENSORS_SPI_TIMEOUT);
    if (status != HAL_OK) {
        printf("Error reading from LIS2MDL register 0x%02X: %d\n", addr, status);
        if (status == HAL_TIMEOUT) {
            printf("LIS2MDL SPI read timeout\n");
        } else if (status == HAL_ERROR) {
            printf("LIS2MDL SPI read error\n");
        } else if (status == HAL_BUSY) {
            printf("LIS2MDL SPI is busy\n");
        }
    }
    LIS2MDL_CS_HIGH();
}

// It seems like we only require ODR, performance mode and operation mode. We'll set only these three setting for now
// and if user will need to change other settings, they can write directly to config registers  
uint8_t LIS2MDL_Init(LIS2MDL_MAG* mag) {
	printf("Initializing Magnetometer...\n");
    uint8_t whoAmI = LIS2MDL_ReadRegister(LIS2MDL_WHO_AM_I_ADDR);
    if (whoAmI != LIS2MDL_WHO_AM_I_VALUE) {
        printf("Initialization error: LIS2MDL not found\n");
		printf("Initialization Failed: Magnetometer not responding\n");
        return 1;  // Device not found
    }
    uint8_t val = 0;

    // Configure CFG_REG_B: Enable temperature compensation, set high-performance mode, ODR=100Hz, set data mode 
    val = (1 << 7) | (mag->performance_mode << 4) | (mag->odr << 2) | (mag->data_mode);
    LIS2MDL_WriteRegister(LIS2MDL_CFG_REG_A_ADDR, val);

    // Configure CFG_REG_C: Disable I2C
    val = 1 << 6;
    LIS2MDL_WriteRegister(LIS2MDL_CFG_REG_C_ADDR, val);

    return 0;  // Initialization successful
}

uint8_t LIS2MDL_Read_Mag(LIS2MDL_MAG* mag) {
    uint8_t buffer[8];
    HAL_StatusTypeDef status = LIS2MDL_ReadRegisters(LIS2MDL_OUT_X_L_ADDR, buffer, 8);
    // Should we return a HAL Status here
    // Convert raw data to gauss
	mag->x = (float)((buffer[1] << 8 | buffer[0]) * MAG_BIT_TO_MILLIGAUSS);
    mag->y = (float)((buffer[3] << 8 | buffer[2]) * MAG_BIT_TO_MILLIGAUSS);
	mag->z = (float)((buffer[5] << 8 | buffer[4]) * MAG_BIT_TO_MILLIGAUSS);
    mag->temp = (float)((buffer[7] << 8 | buffer[6]) / 8.0f + 25.0f); // Taken from ST Drivers. IDK doesn't really line up with datasheet instructions

	return 0; // Success
}

uint8_t LIS2MDL_GetConfig(LIS2MDL_MAG* mag) {
    uint8_t cfg_A = LIS2MDL_ReadRegister(LIS2MDL_CFG_REG_A_ADDR);
    mag->data_mode = cfg_A & 0b00000011;
    mag->odr = cfg_A & 0b00001100;
    mag->performance_mode = cfg_A & 0b00010000;

    return 0;
}

uint8_t LIS2MDL_Test_Mag(LIS2MDL_MAG* mag) {
	printf("Starting LIS2MDL Driver Test\n");
    // Initialize LIS2MDL
    if (LIS2MDL_Init(mag) != 0) {
        // Handle error: LIS2MDL not found
        printf("Initialization error: LIS2MDL not found\n");
        return 1;
    }
    printf("LIS2MDL Initialized Successfully \n");
    HAL_Delay(1000); // Delay 1 Second
    if (LIS2MDL_Read_Mag(mag)) != 0) {
        printf("Read Error: LIS2MDL Read Failed ");
        return 1;
    }
    printf("X: %4.2f Y: %4.2f Z: %4.2f \n", mag->x, mag->y, mag->z);
    return 0;
}

void LIS2MDL_Reboot_Mag(LIS2MDL_MAG* mag) {
    LIS2MDL_WriteRegister(LIS2MDL_CFG_REG_A_ADDR, 1 << 6);
    mag->temp = 0;
    mag->x = 0;
    mag->y = 0;
    mag->z = 0;
}
