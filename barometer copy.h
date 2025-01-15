/*
 * barometer.h
 *
 *  Created on: Jan 14, 2025
 *      Author: UTAT
 */
#ifndef BAROMETER_H
#define BAROMETER_H

#include "main.h"

#define RESET_COMMAND 0x1E
#define D1_COMMAND 0x48
#define D2_COMMAND 0x58
#define ADC_READ_COMMAND 0x00

//function declaration
void init_barometer_protocol(void);
void reset_barometer(SPI_HandleTypeDef *hspi);
uint32_t read_uncompensated_pressure(SPI_HandleTypeDef *hspi);
uint32_t read_uncompensated_temp(SPI_HandleTypeDef *hspi);
uint16 read_PROM(SPI_HandleTypeDef *hspi, uint8_t address);


#endif // BAROMETER_H
