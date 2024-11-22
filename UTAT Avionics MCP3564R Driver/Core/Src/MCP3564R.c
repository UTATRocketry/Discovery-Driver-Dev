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

    HAL_StatusTypeDef status;

// this is a comment
}

// Checks if the MCP3564 is connected to the SPI bus, returns 0 if successful, 1 if failed
int MCP3564_CheckConnection(){
	//Checks if connected by verifying the mode, accessed through register 0x1

}

// Reads voltage calculated from MCP3564, returns 0 if successful, 1 if failed
int MCP3564_ReadChannel(int32_t *channelReading){


	HAL_StatusTypeDef status;
	int8_t data[3];

	//returns 0 if no problem
	status = HAL_SPI_Receive (MCP3564_hspi1, data, 3, 1000);

	//stitches the 3 bytes of data together
	//todo: verify that these bits are being stitched together correctly
	*channelReading = ((int32_t)data[0] << 16 | (int32_t)data[1] << 8 | (int32_t)data[2]);

	return status;
}

