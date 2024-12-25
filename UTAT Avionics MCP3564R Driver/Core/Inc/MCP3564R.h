/*
 * MCP3564R.h
 *
 *  Created on: Nov 4, 2024
 *      Author: williamgomez
 */

#ifndef INC_MCP3564R_H_
#define INC_MCP3564R_H_

#include "stm32l4xx_hal.h" /* Needed for SPI */

/*
---Resources Used---

1. Offical MCP3564R Datasheet from Manufacterer: https://ww1.microchip.com/downloads/aemDocuments/documents/APID/ProductDocuments/DataSheets/MCP3561_2_4R-Data-Sheet-DS200006391C.pdf
2. STM32L4 User Manual: https://www.st.com/resource/en/user_manual/um1884-description-of-stm32l4l4-hal-and-lowlayer-drivers-stmicroelectronics.pdf
*/

/* Registers Section */

//Not sure if we need all of these yet, so they're just all included as per the Notion info page

#define ADCDATA 0x0 /*Latest A/D conversion data refer to page 90 and 48*/
#define CONFIG0 0x1 /*ADC Operating mode, Master Clock mode and Input Bias Current Source mode*/
#define CONFIG1 0x2 /*SPS read/write*/
#define CONFIG2 0x3
#define CONFIG3 0x4

#define IRQ 0x5
#define MUX 0x6 /*Choose channel*/
#define SCAN 0x7
#define TIMER 0x8
#define OFFSETCAL 0x9
#define GAINCAL 0xA
#define LOCK 0xD
#define CRCCFG 0xF


extern SPI_HandleTypeDef* MCP3564_hspi1;

/* Driver Functions Section */

//Initializes MCP3564, returns 0 if successful, 1 if failed
int MCP3564_Init(SPI_HandleTypeDef* hspi1);
// Checks if the MCP3564 is connected to the SPI bus, returns 0 if successful, 1 if failed
int MCP3564_CheckConnection();

// Reads voltage calculated from MCP3564, returns voltage in V
int MCP3564_ReadChannel();

/*
// Other functions to be implemented later?

// Reads the current samples per second, returns the value of SPS
int MCP3564_ReadSPS();

// Writes a new VALID samples per second value to the MCP3564R, returns 0 if successful, 1 if failed
int MCP3564_SetSPS(int input);

// Configures device to draw minimal power, like sleep mode, returns 0 if successful, 1 if failed

// Reads the internal reference voltage of the MCP3564R, returns voltage in V

// Reads the analog and digital master clocks

// Writes the analog and digital master clocks


*/

#endif /* INC_MCP3564R_H_ */
