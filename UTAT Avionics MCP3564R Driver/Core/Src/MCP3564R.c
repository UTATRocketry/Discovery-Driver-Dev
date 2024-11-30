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
	MCP3564_hspi = hspi1;
	//connects hspi to ADC/check that its connected
	//set CS low
	//send write command
	//write to config register to enable active mode
	//recieve status from STATUS byte
	//return 0 if successful

}

// Checks if the MCP3564 is connected to the SPI bus, returns 0 if successful, 1 if failed
int MCP3564_CheckConnection(){
	//Checks if connected by verifying the mode, accessed through register 0x1
	//am I able to read something from this?
	//TxData: CMD byte → CMD[7:6] = 01 (address of device)
	//		         → CMD[5:2] = 0x1
	//		         → CMD[1:0] = 01 (static, for now)
	HAL_StatusTypeDef status;

	//01 = device address, 0001 = CONFIG0, 01 = static read
	uint8_t TxData = 0b01000101; // is this how you do this in C?
	uint8_t RxData[8]; // create space for status byte
	uint16_t Size = 1; // Size: 1 byte
	uint32_t Timeout = 100; // this is arbitrary, pls help

	//Pin C4 is our manual chip select line for the MCP3564R
	//Set ~CS low to begin reading and writing to chip
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);

	status = HAL_SPI_TransmitReceive(MCP3564_hspi, &TxData, &RxData, Size, Timeout);

	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);

	*dev_addr = ((int32_t)RxData[4] << 4 | (int32_t)RxData[5] << 4); // is the index the bit or the byte?
	// check that STAT[5:4]=DEV_ADDR[1:0] == 01
	if (status != HAL_OK) { // function didn't work???
		return 1;
	}
	else if (dev_addr == 01) {
		return 0; // device connected
	}
	return 1;
}

// Reads voltage calculated from MCP3564, returns 0 if successful, 1 if failed
int MCP3564_ReadChannel(int32_t *channelReading){

	HAL_StatusTypeDef status;
	int8_t data[3];
	int8_t command = 0b01000001;

	//CS low
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);
	//returns 0 if no problem
	//01 = device address, 0000 = ADCDATA, 01 = static read
	status = HAL_SPI_Transmit (MCP3564_hspi, &command, 1, 1000);
	if(status == HAL_ERROR){
		return status;
	}
	status = HAL_SPI_Receive (MCP3564_hspi, &data, 3, 1000);

	//CS high
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);

	//stitches the 3 bytes of data together
	//todo: verify that these bits are being stitched together correctly
	*channelReading = ((int32_t)data[0] << 16 | (int32_t)data[1] << 8 | (int32_t)data[2]);

	return status;
}

