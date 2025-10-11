/**
 * @file mag.h
 * @brief Header file for magnetometer driver
 * 
 * This file contains the definitions and function prototypes for
 * initializing and reading data from the magnetometer. It includes
 * the necessary structures, function prototypes, and constants
 * required for the magnetometer driver.
 * 
 * @authors Amelia Ellis, Eric Chiang
 * @date 2025-04-11
 * @version 0.1
 * @bug No known bugs.
 */
#ifndef MAG_H
#define MAG_H

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include "stm32h7xx_hal.h"

// Register Addresses
#define LIS2MDL_WHO_AM_I_ADDR 0x4F
#define LIS2MDL_CFG_REG_A_ADDR 0x60
#define LIS2MDL_CFG_REG_B_ADDR 0x61
#define LIS2MDL_CFG_REG_C_ADDR 0x62
#define LIS2MDL_INT_CTRL_ADDR 0x63
#define LIS2MDL_INT_SRC_ADDR 0x64
#define LIS2MDL_THIS_L_ADDR 0x65
#define LIS2MDL_THIS_H_ADDR 0x66
#define LIS2MDL_STATUS_REG_ADDR 0x67
#define LIS2MDL_OUT_X_L_ADDR 0x68
#define LIS2MDL_OUT_X_H_ADDR 0x69
#define LIS2MDL_OUT_Y_L_ADDR 0x6A
#define LIS2MDL_OUT_Y_H_ADDR 0x6B
#define LIS2MDL_OUT_Z_L_ADDR 0x6C
#define LIS2MDL_OUT_Z_H_ADDR 0x6D
#define LIS2MDL_OUT_TEMP_L_ADDR 0x6E 
#define LIS2MDL_OUT_TEMP_H_ADDR 0X6F

// WHO_AM_I expected value
#define LIS2MDL_WHO_AM_I_VALUE 0x40

// SPI Chip Select Pin Configurations
#define LIS2MDL_CS_PORT GPIOG
#define LIS2MDL_CS_PIN GPIO_PIN_12

// Constants to convert to gauss
#define MAG_BIT_TO_MILLIGAUSS 1.5 // 1.5 milligauss per bit
#define MAG_BIT_TO_CELSIUS 0.125 // 0.125 C per bit

// TODO: find out where these setting are defined
/*
0 - Gauss Range
1 - ODR
2 - Performance Mode
3 - Continous/Single
Not sure why there is 8 settings but whatevs
*/
extern SPI_HandleTypeDef hspi1;

// SPI Communication Macros
#define LIS2MDL_CS_LOW()   HAL_GPIO_WritePin(LIS2MDL_CS_PORT, LIS2MDL_CS_PIN, GPIO_PIN_RESET)
#define LIS2MDL_CS_HIGH()  HAL_GPIO_WritePin(LIS2MDL_CS_PORT, LIS2MDL_CS_PIN, GPIO_PIN_SET)

// Magnetometer Struct

typedef struct {
  float x;
  float y;
  float z;
  float temp;
  lis2mdl_datamode_t data_mode;
  lis2mdl_dataRate_t odr;
  lis2mdl_performancemode_t performance_mode;
} LIS2MDL_MAG;

/** The magnetometer data rate for single mode measurement*/
typedef enum {
    LIS2MDL_DATARATE_10_HZ = 0b00, ///<  10 Hz
    LIS2MDL_DATARATE_20_HZ = 0b01,  ///<  20 Hz
    LIS2MDL_DATARATE_50_HZ = 0b10,   ///<  50 Hz
    LIS2MDL_DATARATE_100_HZ = 0b11,     ///<  100 Hz
} lis2mdl_dataRate_t;

/** The magnetometer performance mode */
typedef enum {
    LIS2MDL_PERFORMANCEMODE_LOWPOWER = 0b0,  ///< Low Power Mode
    LIS2MDL_PERFORMANCEMODE_HIGH_RES = 0b1,    ///< High Resolution Mode
} lis2mdl_performancemode_t;

  /** The magnetometer data mode */
  typedef enum {
	LIS2MDL_OPERATIONMODE_CONTINUOUS = 0b00, ///< Continuous conversion mode
	LIS2MDL_OPERATIONMODE_SINGLE = 0b01,     ///< Single-shot conversion
	LIS2MDL_OPERATIONMODE_POWERDOWN = 0b11,  ///< Idle mode
  } lis2mdl_datamode_t;

/**
 * @brief Write a single byte to a LIS2MDL register.
 * @param reg: Register address.
 * @param value: Value to write.
 */
void LIS2MDL_WriteRegister(uint8_t addr, uint8_t value);
/**
 * @brief Read a single byte from a LIS2MDL register.
 * @param reg: Register address.
 * @return The value read.
 */
uint8_t LIS2MDL_ReadRegister(uint8_t addr);

/**
 * @brief Read multiple bytes starting from a LIS2MDL register.
 * @param reg: Starting register address.
 * @param buffer: Buffer to store the data.
 * @param length: Number of bytes to read.
 */
void LIS2MDL_ReadRegisters(uint8_t addr, uint8_t *buffer, uint8_t len);

/**
 * @brief Initializes the magnetometer.
 * 
 * This function configures the SPI communication and sets up the GPIO pins
 * for the magnetometer. It also performs a self-test to ensure the sensor is
 * functioning correctly.
 * 
 * @return uint8_t Status of the initialization (0 for success, 1 for failure).
 */
uint8_t LIS2MDL_Init(LIS2MDL_MAG* mag);

/**
 * @brief This function configures the mode, data rate, and range of the magnetometer.
 * 
 * @param mode The mode to set (0 for continuous, 1 for single).
 * @param dataRate The data rate to set (in Hz).
 * @param range The range to set (in Gauss).
 * @return uint8_t Status of the configuration (0 for success, 1 for failure).
 */

uint8_t LIS2MDL_GetConfig(LIS2MDL_MAG* mag);

/**
 * @brief Reads data from the magnetometer.
 * 
 * This function retrieves the magnetic field data from the magnetometer.
 * 
 * @param pData Pointer to store the magnetic field data (x, y, z components).
 * @return uint8_t Status of the read operation (0 for success, 1 for failure).
 */
uint8_t LIS2MDL_Read_Mag(LIS2MDL_MAG* mag);

/**
 * @brief Reboots the magnetometer. 
 * 
 * Magnetometer memory is reset. The user config registers are retained.
 *
 * @return None
 */
void LIS2MDL_Reboot_Mag(LIS2MDL_MAG* mag);

uint8_t Mag_Test();

#endif // MAG_H