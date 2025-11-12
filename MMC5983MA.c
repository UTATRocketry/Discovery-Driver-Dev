/**
 * @file mag.c
 * @brief Implementation of magnetometer driver
 *
 * This file contains the implementation of the magnetometer driver
 * using the MMC5983MA sensor. It includes functions for initializing,
 * configuring, and reading data from the magnetometer.
 *
 * @authors Eric Chiang
 * @date 2025-10-18
 * @version 0.1
 * @bug No known bugs.
 */
#include "MMC5983MA.h"
#include "sensors_defs.h"

/*
 * [0] = Bandwidth
 * [1] = Continous Mode Measurement Freq
 * [2] = Set Count
 */
uint8_t MMC5983MA_SETTINGS[3];

// Calibration Values
float hard_iron[3] = {0};
float soft_iron[3][3] = {
		{1,0,0},
		{0,1,0},
		{0,0,1}
};

uint8_t MMC5983MA_Init(void) {
    HAL_StatusTypeDef status = HAL_OK;
    uint8_t whoami = MMC5983MA_ReadRegister(MMC5983MA_WHO_AM_I);
    if (whoami != MMC5983MA_WHO_AM_I_VALUE) {
    	printf("Initialization failed, incorrect whoami value. Expected: %x, Received: %x", MMC5983MA_WHO_AM_I_VALUE, whoami);
    	return 1;
    }

    // Enable Auto_SR_en
    uint8_t reg_val = 1 << 1;
    MMC5983MA_WriteRegister(MMC5983MA_CTRL0, reg_val);

    // Set bandwidth of sensor to be 100hz (default) (measurement time = 8ms)
//    reg_val = 0b00000000;
//    MMC5983MA_WriteRegister(MMC5983MA_CTRL1, reg_val);
	MMC5983MA_SETTINGS[0] = MMC5983MA_BANDWIDTH_100HZ;

    // Set measurement mode to be continuous @ 100Hz.
    // Set auto SET operation of magnetometer/1000 measurement (realigns the mag??)
    reg_val =  0b11101101;
	MMC5983MA_WriteRegister(MMC5983MA_CTRL2, reg_val);
	MMC5983MA_SETTINGS[1] = MMC5983MA_MEASUREMENT_100HZ;
	MMC5983MA_SETTINGS[2] = MMC5983MA_SET_1000;

	// Nothing to modify on CTRL3 register

	return 0;
}

uint8_t MMC5983MA_ReadRegister(uint8_t addr) {
	HAL_StatusTypeDef status = HAL_OK;

	// MSB = 1 for read, 5 bit register addresses starts at Bit 2
    uint8_t txData = addr | 0x80;
    uint8_t rxData;

    // Set CS Pin Low
    status = HAL_SPI_GetState(&hspi1);
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_12);
    HAL_GPIO_WritePin(MMC5983MA_CS_PORT, MMC5983MA_CS_PIN, GPIO_PIN_RESET);
    status = HAL_SPI_Transmit(&hspi1, &txData, 1, SENSORS_SPI_TIMEOUT);
    if (status != HAL_OK) {
        printf("Error writing to MMC5983MA register 0x%02X: %d\n", addr, status);
        if (status == HAL_TIMEOUT) {
            printf("MMC5983MA SPI write timeout\n");
        } else if (status == HAL_ERROR) {
            printf("MMC5983MA SPI write error\n");
        } else if (status == HAL_BUSY) {
            printf("MMC5983MA SPI is busy\n");
        }
    }
    status = HAL_SPI_Receive(&hspi1, &rxData, 1, SENSORS_SPI_TIMEOUT);
    if (status != HAL_OK) {
        printf("Error reading from MMC5983MA register 0x%02X: %d\n", addr, status);
        if (status == HAL_TIMEOUT) {
            printf("MMC5983MA SPI read timeout\n");
        } else if (status == HAL_ERROR) {
            printf("MMC5983MA SPI read error\n");
        } else if (status == HAL_BUSY) {
            printf("MMC5983MA SPI is busy\n");
        }
    }

    // Set CS Pin High
    HAL_GPIO_WritePin(MMC5983MA_CS_PORT, MMC5983MA_CS_PIN, GPIO_PIN_SET);

    return rxData;
}

void MMC5983MA_ReadRegisters(uint8_t addr, uint8_t *buffer, uint8_t length) {
	HAL_StatusTypeDef status = HAL_OK;

	// MSB = 1 for read, 5 bit register addresses starts at Bit 2
	uint8_t txData = addr | 0x80;

	// Set CS to Low
	HAL_GPIO_WritePin(MMC5983MA_CS_PORT, MMC5983MA_CS_PIN, GPIO_PIN_RESET);
    status = HAL_SPI_Transmit(&hspi1, &txData, 1, SENSORS_SPI_TIMEOUT);
    if (status != HAL_OK) {
        printf("Error writing to MMC5983MA register 0x%02X: %d\n", addr, status);
        if (status == HAL_TIMEOUT) {
            printf("MMC5983MA SPI write timeout\n");
        } else if (status == HAL_ERROR) {
            printf("MMC5983MA SPI write error\n");
        } else if (status == HAL_BUSY) {
            printf("MMC5983MA SPI is busy\n");
        }
    }

    status = HAL_SPI_Receive(&hspi1, buffer, length, SENSORS_SPI_TIMEOUT);
    if (status != HAL_OK) {
        printf("Error reading from MMC5983MA register 0x%02X: %d\n", addr, status);
        if (status == HAL_TIMEOUT) {
            printf("MMC5983MA SPI read timeout\n");
        } else if (status == HAL_ERROR) {
            printf("MMC5983MA SPI read error\n");
        } else if (status == HAL_BUSY) {
            printf("MMC5983MA SPI is busy\n");
        }
    }

    // Set CS Pin High
    HAL_GPIO_WritePin(MMC5983MA_CS_PORT, MMC5983MA_CS_PIN, GPIO_PIN_SET);
}

void MMC5983MA_WriteRegister(uint8_t addr, uint8_t value) {
	HAL_StatusTypeDef status = HAL_OK;
	uint8_t buffer[2] = { addr, value };

	// Set CS to Low
	HAL_GPIO_WritePin(MMC5983MA_CS_PORT, MMC5983MA_CS_PIN, GPIO_PIN_RESET);
	status = HAL_SPI_Transmit(&hspi1, buffer, 2, SENSORS_SPI_TIMEOUT);
    if (status != HAL_OK) {
        printf("Error writing to MMC5983MA register 0x%02X: %d\n", addr, status);
        if (status == HAL_TIMEOUT) {
            printf("MMC5983MA SPI write timeout\n");
        } else if (status == HAL_ERROR) {
            printf("MMC5983MA SPI write error\n");
        } else if (status == HAL_BUSY) {
            printf("MMC5983MA SPI is busy\n");
        }
    }

	// Set CS to High
	HAL_GPIO_WritePin(MMC5983MA_CS_PORT, MMC5983MA_CS_PIN, GPIO_PIN_SET);
}

void MMC5983MA_ReadMagneticField16(vector_t* mag_data) {
    uint8_t buffer[6];
    MMC5983MA_ReadRegisters(MMC5983MA_XOUT_H, buffer, 6);

	uint16_t x_raw = (uint16_t)(buffer[0] << 8 | buffer[1]);
	uint16_t y_raw = (uint16_t)(buffer[2] << 8 | buffer[3]);
	uint16_t z_raw = (uint16_t)(buffer[4] << 8 | buffer[5]);

	mag_data->v[0] = MMC5983MA_16Bits_to_mGauss(x_raw);
	mag_data->v[1] = MMC5983MA_16Bits_to_mGauss(y_raw);
	mag_data->v[2] = MMC5983MA_16Bits_to_mGauss(z_raw);
}

void MMC5983MA_ReadMagneticField18(vector_t* mag_data) {
    uint8_t buffer[7];
    MMC5983MA_ReadRegisters(MMC5983MA_XOUT_H, buffer, 7);

    // buffer[6] contains two additional bits of x, y, z mag readings
	uint32_t x_raw = (uint32_t)(buffer[0] << 10 | buffer[1] << 2 | buffer[6] >> 6);
	uint32_t y_raw = (uint32_t)(buffer[2] << 10 | buffer[3] << 2 | (buffer[6] & 0x30) >> 4);
	uint32_t z_raw = (uint32_t)(buffer[4] << 10 | buffer[5] << 2 | (buffer[6] & 0x0C) >> 2);

	mag_data->v[0] = MMC5983MA_18Bits_to_mGauss(x_raw);
	mag_data->v[1] = MMC5983MA_18Bits_to_mGauss(y_raw);
	mag_data->v[2] = MMC5983MA_18Bits_to_mGauss(z_raw);
}

float MMC5983MA_Get_Temp(void) {
	HAL_StatusTypeDef status = HAL_OK;
	uint8_t raw_temp = MMC5983MA_ReadRegister(MMC5983MA_TOUT);
	return (float)raw_temp * 0.8 - 75.0;
}

void MMC5983MA_SW_Reset(void) {
	MMC5983MA_WriteRegister(MMC5983MA_CTRL1, 0x80); // SW Reset is the MSB
	// TODO: Maybe use a timer instead so we don't block
	HAL_Delay(10); // 10ms power-up time
}

float MMC5983MA_16Bits_to_mGauss(uint16_t mag_val) {
	float mag_val_mgauss = (float)mag_val - 65536.0;
	mag_val_mgauss /= 65536.0;
	mag_val_mgauss *= 8; // Get full range of +-8 Gauss
	return mag_val_mgauss * 1000; // Get milligauss
}

float MMC5983MA_18Bits_to_mGauss(uint32_t mag_val) {
	float mag_val_mgauss = (float)mag_val - 131072.0;
	mag_val_mgauss /= 131072.0;
	mag_val_mgauss *= 8; // Get full range of +-8 Gauss
	return mag_val_mgauss * 1000; // Get milligauss
}

/*
 * More in depth explanation of this function:
 * We are using MotionCal (https://github.com/PaulStoffregen/MotionCal) to calibrate the magnetometer.
 * MotionCal reads in sensor data over UART in a very specific format, and then automatically performs
 * calibration and outputs calibration values for the magnetometer.
 * Required output format for MotionCal: "Raw:{accX},{accY},{accZ},{gyrX},{gyrY},{gyrZ},{magX},{magY},{magZ}\r\n"
 *
 * Either way, these calibrations need to happen after the magnetometer has been installed onto the rocket system for accurate
 * bias values.
 */
void MMC5983MA_Calibrate_MotionCal(vector_t* mag_data) {
	char raw_data[] = "Raw:0,0,0,0,0,0,"; // Currently only require calibration for mag values
	char new_line[] = "\r\n";
	char comma = ',';

	// Inputted mag values must be in units of milligauss
	char x_buffer[8] = "";
	char y_buffer[8] = "";
	char z_buffer[8] = "";
	sprintf(x_buffer, "%d", (int16_t)mag_data->v[0]);
	sprintf(y_buffer, "%d", (int16_t)mag_data->v[1]);
	sprintf(z_buffer, "%d", (int16_t)mag_data->v[2]);

	HAL_UART_Transmit(&huart2, (uint8_t*)raw_data, sizeof(raw_data)-1, HAL_UART_TIMEOUT_VALUE);
	HAL_UART_Transmit(&huart2, (uint8_t*)x_buffer, strlen(x_buffer), HAL_UART_TIMEOUT_VALUE);
	HAL_UART_Transmit(&huart2, (uint8_t*)&comma, 1, HAL_UART_TIMEOUT_VALUE);
	HAL_UART_Transmit(&huart2, (uint8_t*)y_buffer, strlen(y_buffer), HAL_UART_TIMEOUT_VALUE);
	HAL_UART_Transmit(&huart2, (uint8_t*)&comma, 1, HAL_UART_TIMEOUT_VALUE);
	HAL_UART_Transmit(&huart2, (uint8_t*)z_buffer, strlen(z_buffer), HAL_UART_TIMEOUT_VALUE);
	HAL_UART_Transmit(&huart2, (uint8_t*)new_line, sizeof(new_line), HAL_UART_TIMEOUT_VALUE);
}



// Hard iron offsets are biases in the magnetometer that are created by magnetic
	// fields emitted by permanent magnets (motors, inductors, etc)
// Soft iron offsets are biases caused by materials near the magnetometer that
	// warp/disturb the surrounding magnetic fields
void MMC5983MA_Calibrate_Data(vector_t* mag_data) {
	float mag_temp[3] = {0};
	for (uint8_t i = 0; i < 3; i++) {
		mag_temp[i] = mag_data->v[i] - hard_iron[i];
	}

	for (uint8_t i = 0; i < 3; i++) {
		mag_data->v[i] =
				soft_iron[i][0] * mag_temp[0] +
				soft_iron[i][1] * mag_temp[1] +
				soft_iron[i][2] * mag_temp[2];
	}
}
