/**
 ******************************************************************************
 * @file    lis3mdl_driver.h
 * @author  Amelia Ellis
 * @brief   LIS3MDL header driver file
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef LIS3MDL_DRIVER_H
#define LIS3MDL_DRIVER_H

// Includes
#include "stm32h7xx_hal.h"
#include <stdint.h>

// SPI Handle
extern SPI_HandleTypeDef hspi1;

// Pin Definitions
#define LIS3MDL_CS_PORT GPIOA
#define LIS3MDL_CS_PIN GPIO_PIN_4

// Register Addresses
#define WHO_AM_I_ADDR      0x0F
#define CTRL_REG1_ADDR     0x20
#define CTRL_REG2_ADDR     0x21
#define CTRL_REG3_ADDR     0x22
#define OUT_X_L_ADDR       0x28
#define OUT_X_H_ADDR       0x29
#define OUT_Y_L_ADDR       0x2A
#define OUT_Y_H_ADDR       0x2B
#define OUT_Z_L_ADDR       0x2C
#define OUT_Z_H_ADDR       0x2D

// WHO_AM_I Expected Value
#define WHO_AM_I_VALUE     0x3D

// SPI Communication Macros
#define LIS3MDL_CS_LOW()   HAL_GPIO_WritePin(LIS3MDL_CS_PORT, LIS3MDL_CS_PIN, GPIO_PIN_RESET)
#define LIS3MDL_CS_HIGH()  HAL_GPIO_WritePin(LIS3MDL_CS_PORT, LIS3MDL_CS_PIN, GPIO_PIN_SET)

// Function Prototypes
void LIS3MDL_WriteRegister(uint8_t reg, uint8_t value);
uint8_t LIS3MDL_ReadRegister(uint8_t reg);
void LIS3MDL_ReadRegisters(uint8_t reg, uint8_t *buffer, uint8_t length);
uint8_t LIS3MDL_Init(void);
void LIS3MDL_ReadMagneticField(int16_t *x, int16_t *y, int16_t *z);

#endif // LIS3MDL_DRIVER_H
