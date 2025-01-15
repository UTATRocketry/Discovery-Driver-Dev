/*
 * barometer.c
 *
 *  Created on: Jan 14, 2025
 *      Author: UTAT
 */

#include "barometer.h"

void init_barometer_protocol(void) {
	// Pull PS low to select the SPI protocol
	// GPIO_PIN_RESET (low)
	HAL_GPIO_WritePin(PS_GPIO_Port, PS_Pin, GPIO_PIN_RESET);
 }


void reset_barometer(SPI_HandleTypeDef *hspi) {
	// function runs after the system turns on
	// loads the calibration coefficients

	uint8_t resetCommand = RESET_COMMAND;

    // Pull CSB low to enable communication (suggested)
	HAL_GPIO_WritePin(CSB_GPIO_Port, CSB_Pin, GPIO_PIN_RESET);

    // Send the reset command
	if (HAL_SPI_Transmit(hspi, &resetCommand, 1, HAL_MAX_DELAY) == HAL_OK) {
        // 2.8 ms reload
		HAL_Delay(3);
    } else {
    	printf("Failed to send reset command\n");
    }

	// Pull CSB high to enable communication
    HAL_GPIO_WritePin(CSB_GPIO_Port, CSB_Pin, GPIO_PIN_SET);
}


uint32_t read_uncompensated_pressure(SPI_HandleTypeDef *hspi) {
	// Pull CSB low, open connection
	HAL_GPIO_WritePin(CSB_GPIO_Port, CSB_Pin, GPIO_PIN_RESET);

	// Measure the current D1 value
	uint8_t d1Command = D1_COMMAND;
	HAL_SPI_Transmit(hspi, &d1Command, 1, HAL_MAX_DELAY);

	// Pull CSB high (suggested, ensure accuracy), close connection
	HAL_GPIO_WritePin(CSB_GPIO_Port, CSB_Pin, GPIO_PIN_SET);

	// Barometer converting
	HAL_Delay(9.50);

	// Open connection
	HAL_GPIO_WritePin(CSB_GPIO_Port, CSB_Pin, GPIO_PIN_RESET);

	uint8_t adcCommand = ADC_READ_COMMAND;
	HAL_SPI_Transmit(hspi, &adcCommand, 1, HAL_MAX_DELAY);

	uint8_t returnData[3] = {0};
	HAL_SPI_Receive(hspi, returnData, 3, HAL_MAX_DELAY);

	// Close connection
	HAL_GPIO_WritePin(CSB_GPIO_Port, CSB_Pin, GPIO_PIN_SET);

	uint32_t pressure = 0;
	// Result in 24 bits
	pressure = ((uint32_t)rxData[0] << 16) | ((uint32_t)rxData[1] << 8) | (uint32_t)rxData[2];

	return pressure;
}


uint32_t read_uncompensated_temp(SPI_HandleTypeDef *hspi) {
	// Pull CSB low, open connection
	HAL_GPIO_WritePin(CSB_GPIO_Port, CSB_Pin, GPIO_PIN_RESET);

	// Measure the current D1 value
	uint8_t d1Command = D2_COMMAND;
	HAL_SPI_Transmit(hspi, &d1Command, 1, HAL_MAX_DELAY);

	// Pull CSB high (suggested, ensure accuracy), close connection
	HAL_GPIO_WritePin(CSB_GPIO_Port, CSB_Pin, GPIO_PIN_SET);

	// Barometer converting
	HAL_Delay(9.50);

	// Open connection
	HAL_GPIO_WritePin(CSB_GPIO_Port, CSB_Pin, GPIO_PIN_RESET);

	uint8_t adcCommand = ADC_READ_COMMAND;
	HAL_SPI_Transmit(hspi, &adcCommand, 1, HAL_MAX_DELAY);

	uint8_t returnData[3] = {0};
	HAL_SPI_Receive(hspi, returnData, 3, HAL_MAX_DELAY);

	// Close connection
	HAL_GPIO_WritePin(CSB_GPIO_Port, CSB_Pin, GPIO_PIN_SET);

	uint32_t pressure = 0;
	// Result in 24 bits
	pressure = ((uint32_t)rxData[0] << 16) | ((uint32_t)rxData[1] << 8) | (uint32_t)rxData[2];

	return pressure;
}


uint16 read_PROM(SPI_HandleTypeDef *hspi, uint8_t address) {
	// Pull CSB low, open connection
	HAL_GPIO_WritePin(CSB_GPIO_Port, CSB_Pin, GPIO_PIN_RESET);

	// Measure the current D1 value
	uint8_t Command = address;
	HAL_SPI_Transmit(hspi, &Command, 1, HAL_MAX_DELAY);

    uint8_t returnData[2] = {0};
	HAL_SPI_Receive(hspi, returnData, 2, HAL_MAX_DELAY);

	// Close connection
	HAL_GPIO_WritePin(CSB_GPIO_Port, CSB_Pin, GPIO_PIN_SET);

	uint16_t prom = 0;
	// Result in 16 bits
	prom = ((uint16_t)rxData[0] << 8) | rxData[1];

	return prom;
}

