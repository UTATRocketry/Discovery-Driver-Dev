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
//connects hspi to ADC/check that its connected
    HAL_StatusTypeDef status;

}

// Checks if the MCP3564 is connected to the SPI bus, returns 0 if successful, 1 if failed
int MCP3564_CheckConnection(){
	//Checks if connected by verifying the mode, accessed through register 0x1
	//am I able to read something from this?
	//TxData: CMD byte → CMD[7:6] = 01 (address of device)
	//		         → CMD[5:2] = 0x1
	//		         → CMD[1:0] = 01 (static, for now)
	HAL_StatusTypeDef status;

	unit8_t TxData = 0b01000101; // is this how you do this in C?
	uint8_t RxData[8]; // create space for status byte
	uint16_t Size = 8; // Size: 8 bits (is this measured in bits or bytes?)
	unint32_t Timeout = 100; // this is arbitrary, pls help
	status = HAL_SPI_TransmitReceive(hspi, &TxData, &RxData, Size, Timeout);

	*dev_addr = ((int32_t)RxData[4] << 4 | (int32_t)RxData[5] << 4); // is the index the bit or the byte?
	// check that STAT[5:4]=DEV_ADDR[1:0] == 01
	if (status != HAL_OK) { // function didn't work???
		return 0;
	}
	elif (dev_addr == 01) {
		return 1; // device connected
	}
	return 0;
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

