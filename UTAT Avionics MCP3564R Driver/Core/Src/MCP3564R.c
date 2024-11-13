/*
 * MCP3564R.c
 *
 *  Created on: Nov 4, 2024
 *      Author: williamgomez
 */
#include "MCP3564R.h"


SPI_HandleTypeDef* MCP3564_hspi1;

//Initializes MCP3564 on a particular SPI bus, returns 0 if successful, 1 if failed
int MCP3564_Init(SPI_HandleTypeDef* hspi1){
	MCP3564_hspi1 = hspi1;

    uint8_t data;
    HAL_StatusTypeDef status;

}

// Checks if the MCP3564 is connected to the SPI bus, returns 0 if successful, 1 if failed
int MCP3564_CheckConnection(){
	//Checks if connected by verifying the mode, accessed through register 0x1

}

// Reads voltage calculated from MCP3564, returns voltage in V
float MCP3564_ReadVoltage(){

	uint8_t data[2];
	HAL_StatusTypeDef status;

	status = HAL_SPI_Receive (MCP3564_hspi1, data, 2, 1000);
	if (status != HAL_OK) return 0;

	int16_t raw_accel = (int16_t)(data[0] << 8 | data[1]);


}

