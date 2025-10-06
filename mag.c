/**
 * @file mag.c
 * @brief Implementation of magnetometer driver
 * 
 * This file contains the implementation of the magnetometer driver
 * using the LIS3MDL sensor. It includes functions for initializing,
 * configuring, and reading data from the magnetometer.
 * 
 * @authors Amelia Ellis
 * @date 2025-04-11
 * @version 0.1
 * @bug No known bugs.
 */
#include "mag.h"

//GPIO_TypeDef LIS3MDL_CS_PORT =  GPIOG;
//uint16_t LIS3MDL_CS_PIN = GPIO_PIN_12;
/**
 * @brief Write a single byte to a LIS3MDL register.
 * @param reg: Register address.
 * @param value: Value to write.
 */
void LIS3MDL_WriteRegister(uint8_t reg, uint8_t value) {
    HAL_StatusTypeDef status;
    uint8_t txData[2] = {reg & 0x7F, value};  // MSB 0 for write
    LIS3MDL_CS_LOW();
    status = HAL_SPI_Transmit(&hspi1, txData, sizeof(txData), SENSORS_SPI_TIMEOUT);
    if (status != HAL_OK) {
        printf("Error writing to LIS3MDL register 0x%02X: %d\n", reg, status);
        if (status == HAL_TIMEOUT) {
            printf("LIS3MDL SPI write timeout\n");
        } else if (status == HAL_ERROR) {
            printf("LIS3MDL SPI write error\n");
        } else if (status == HAL_BUSY) {
            printf("LIS3MDL SPI is busy\n");
        }
    }
    LIS3MDL_CS_HIGH();
}

/**
 * @brief Read a single byte from a LIS3MDL register.
 * @param reg: Register address.
 * @return The value read.
 */
uint8_t LIS3MDL_ReadRegister(uint8_t reg) {
    HAL_StatusTypeDef status;
    uint8_t txData = reg | 0x80;  // MSB 1 for read
    uint8_t rxData;
    LIS3MDL_CS_LOW();
    status = HAL_SPI_Transmit(&hspi1, &txData, 1, SENSORS_SPI_TIMEOUT);
    if (status != HAL_OK) {
        printf("Error writing to LIS3MDL register 0x%02X: %d\n", reg, status);
        if (status == HAL_TIMEOUT) {
            printf("LIS3MDL SPI write timeout\n");
        } else if (status == HAL_ERROR) {
            printf("LIS3MDL SPI write error\n");
        } else if (status == HAL_BUSY) {
            printf("LIS3MDL SPI is busy\n");
        }
    }
    status = HAL_SPI_Receive(&hspi1, &rxData, 1, SENSORS_SPI_TIMEOUT);
    if (status != HAL_OK) {
        printf("Error reading from LIS3MDL register 0x%02X: %d\n", reg, status);
        if (status == HAL_TIMEOUT) {
            printf("LIS3MDL SPI read timeout\n");
        } else if (status == HAL_ERROR) {
            printf("LIS3MDL SPI read error\n");
        } else if (status == HAL_BUSY) {
            printf("LIS3MDL SPI is busy\n");
        }
    }
    LIS3MDL_CS_HIGH();
    return rxData;
}

/**
 * @brief Read multiple bytes starting from a LIS3MDL register.
 * @param reg: Starting register address.
 * @param buffer: Buffer to store the data.
 * @param len: Number of bytes to read.
 */
void LIS3MDL_ReadRegisters(uint8_t reg, uint8_t *buffer, uint8_t len) {
    HAL_StatusTypeDef status;
    uint8_t txData = reg | 0x80;  // MSB 1 for read
    LIS3MDL_CS_LOW();
    status = HAL_SPI_Transmit(&hspi1, &txData, 1, SENSORS_SPI_TIMEOUT);
    if (status != HAL_OK) {
        printf("Error writing to LIS3MDL register 0x%02X: %d\n", reg, status);
        if (status == HAL_TIMEOUT) {
            printf("LIS3MDL SPI write timeout\n");
        } else if (status == HAL_ERROR) {
            printf("LIS3MDL SPI write error\n");
        } else if (status == HAL_BUSY) {
            printf("LIS3MDL SPI is busy\n");
        }
    }
    status = HAL_SPI_Receive(&hspi1, buffer, len, SENSORS_SPI_TIMEOUT);
    if (status != HAL_OK) {
        printf("Error reading from LIS3MDL register 0x%02X: %d\n", reg, status);
        if (status == HAL_TIMEOUT) {
            printf("LIS3MDL SPI read timeout\n");
        } else if (status == HAL_ERROR) {
            printf("LIS3MDL SPI read error\n");
        } else if (status == HAL_BUSY) {
            printf("LIS3MDL SPI is busy\n");
        }
    }
    LIS3MDL_CS_HIGH();
}

uint8_t Mag_Init() {
	printf("Initializing Magnetometer...\n");
    uint8_t whoAmI = LIS3MDL_ReadRegister(LIS3MDL_WHO_AM_I_ADDR);
    if (whoAmI != LIS3MDL_WHO_AM_I_VALUE) {
        printf("Initialization error: LIS3MDL not found\n");
		printf("Initialization Failed: Magnetometer not responding\n");
        return 1;  // Device not found
    }

    LIS3MDL_SETTINGS[0] = LIS3MDL_RANGE_12_GAUSS;
    LIS3MDL_SETTINGS[1] = LIS3MDL_DATARATE_10_HZ;
    LIS3MDL_SETTINGS[2] = LIS3MDL_PERFORMANCEMODE_HIGH;
    LIS3MDL_SETTINGS[3] = LIS3MDL_OPERATIONMODE_CONTINUOUS;

    // Configure CTRL_REG1: Enable temperature sensor, set high-performance mode, ODR=10Hz
    LIS3MDL_WriteRegister(LIS3MDL_CTRL_REG1_ADDR, 0x70);

    // Configure CTRL_REG2: Set full scale to ±12 gauss
    LIS3MDL_WriteRegister(LIS3MDL_CTRL_REG2_ADDR, 0x20);

    // Configure CTRL_REG3: Set continuous-conversion mode
    LIS3MDL_WriteRegister(LIS3MDL_CTRL_REG3_ADDR, 0x00);

    Mag_GetConfig();

    return 0;  // Initialization successful
}

uint8_t Mag_Read(vector_t *pData) {
    uint8_t buffer[6];
    LIS3MDL_ReadRegisters(LIS3MDL_OUT_X_L_ADDR, buffer, 6);
    // Convert raw data to gauss
	int16_t x_raw = (int16_t)(buffer[1] << 8 | buffer[0]);
	int16_t y_raw = (int16_t)(buffer[3] << 8 | buffer[2]);
	int16_t z_raw = (int16_t)(buffer[5] << 8 | buffer[4]);

	float scale = 1.0f; // Scale factor based on range
	switch (LIS3MDL_SETTINGS[0]) {
		case LIS3MDL_RANGE_4_GAUSS: scale = 6842.0f; break;
		case LIS3MDL_RANGE_8_GAUSS: scale = 3421.0f; break;
		case LIS3MDL_RANGE_12_GAUSS: scale = 2281.0f; break;
		case LIS3MDL_RANGE_16_GAUSS: scale = 1711.0f; break;
	}

	pData->x = x_raw / scale;
	pData->y = y_raw / scale;
	pData->z = z_raw / scale;

	return 0; // Success
}

uint8_t Mag_SetConfig(uint8_t *settings) {
    uint8_t ctrl_reg1 = (settings[1] << 1); // Data rate bits
    uint8_t ctrl_reg2 = (settings[0] << 5); // Range bits
    // uint8_t ctrl_reg3 = (settings[3] << 0); // Operation mode bits
    uint8_t ctrl_reg4 = (settings[2] << 2); // Performance mode bits

    // Write configuration to registers
    LIS3MDL_WriteRegister(LIS3MDL_CTRL_REG1_ADDR, ctrl_reg1);
    LIS3MDL_WriteRegister(LIS3MDL_CTRL_REG2_ADDR, ctrl_reg2);
    // LIS3MDL_WriteRegister(self, LIS3MDL_CTRL_REG3_ADDR, ctrl_reg3);
    LIS3MDL_WriteRegister(LIS3MDL_CTRL_REG4_ADDR, ctrl_reg4);
    // Read back the configuration to verify
    if (Mag_GetConfig(LIS3MDL_SETTINGS) != 0 ||
    	LIS3MDL_SETTINGS[0] != settings[0] ||
		LIS3MDL_SETTINGS[1] != settings[1] ||
		LIS3MDL_SETTINGS[2] != settings[2]) {
        return 1; // Configuration verification failed
    }

    return 0; // Success
}

uint8_t Mag_GetConfig() {
    uint8_t ctrl_reg1, ctrl_reg2, ctrl_reg4;
    // Read configuration registers
    ctrl_reg1 = LIS3MDL_ReadRegister(LIS3MDL_CTRL_REG1_ADDR);
    ctrl_reg2 = LIS3MDL_ReadRegister(LIS3MDL_CTRL_REG2_ADDR);
    ctrl_reg4 = LIS3MDL_ReadRegister(LIS3MDL_CTRL_REG4_ADDR);
    // Check if the registers are valid
    if (ctrl_reg1 == 0xFF || ctrl_reg2 == 0xFF || ctrl_reg4 == 0xFF) {
        return 1; // Error reading registers
    }
    
    // Extract configuration values
    LIS3MDL_SETTINGS[0] = (ctrl_reg2 >> 5) & 0x03; // Range bits
    LIS3MDL_SETTINGS[1] = (ctrl_reg1 >> 1) & 0x0F; // Data rate bits
    LIS3MDL_SETTINGS[2] = (ctrl_reg4 >> 5) & 0x03; // Performance Mode bits
    return 0; // Success
}

uint8_t Mag_Test() {
	printf("Starting LIS3MDL Driver Test\n");
    // Initialize LIS3MDL
    if (Mag_Init() != 0) {
        // Handle error: LIS3MDL not found
        printf("Initialization error: LIS3MDL not found\n");
        return 1;
    }
    printf("LIS3MDL Initialized Successfully \n");
    DelayMs(1000);
    vector_t magData;
    if (Mag_Read(&magData) != 0) {
        printf("Read Error: LIS3MDL Read Failed ");
        return 1;
    }
    printf("X: %4.2f Y: %4.2f Z: %4.2f \n", magData.x, magData.y, magData.z);
    return 0;
}
