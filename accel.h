/**
 * @file accel.h
 * @brief Header file for accelerometer sensor functions
 * 
 * This file contains the declarations for functions to initialize,
 * configure, and read data from the accelerometer sensor.
 * 
 * @authors Amelia Ellis
 * @date 2025-04-10
 * @version 0.1
 * @bug No known bugs.
 */
#ifndef ACCEL_H
#define ACCEL_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "stm32h7xx_hal.h"
#include "sensors_defs.h"



#define DEVID_REG         0x00
#define POWER_CTL         0x2D
#define BW_RATE           0x2C
#define DATA_FORMAT       0x31
#define DATAX0            0x32

#define ADXL375_DEVICE_ID 0xE5

#define ADXL375_CS_PORT   GPIOG
#define ADXL375_CS_PIN    GPIO_PIN_13
#define ADXL375_CS_LOW()  HAL_GPIO_WritePin(ADXL375_CS_PORT, ADXL375_CS_PIN, GPIO_PIN_RESET)
#define ADXL375_CS_HIGH() HAL_GPIO_WritePin(ADXL375_CS_PORT, ADXL375_CS_PIN, GPIO_PIN_SET)

// Sensor settings
#define ADXL375_SETTINGS_SIZE 8
extern uint8_t ADXL375_SETTINGS[ADXL375_SETTINGS_SIZE];


uint8_t Accel_Init();

uint8_t Accel_Read(vector_t *pData);

uint8_t Accel_Test();
#endif // ACCEL_H