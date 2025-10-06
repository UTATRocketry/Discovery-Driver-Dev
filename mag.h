/**
 * @file mag.h
 * @brief Header file for magnetometer driver
 * 
 * This file contains the definitions and function prototypes for
 * initializing and reading data from the magnetometer. It includes
 * the necessary structures, function prototypes, and constants
 * required for the magnetometer driver.
 * 
 * @authors Amelia Ellis
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
#include "sensors_defs.h"



// Register Addresses
#define LIS3MDL_WHO_AM_I_ADDR      0x0F
#define LIS3MDL_CTRL_REG1_ADDR     0x20
#define LIS3MDL_CTRL_REG2_ADDR     0x21
#define LIS3MDL_CTRL_REG3_ADDR     0x22
#define LIS3MDL_CTRL_REG4_ADDR     0x23
#define LIS3MDL_OUT_X_L_ADDR       0x28
#define LIS3MDL_OUT_X_H_ADDR       0x29
#define LIS3MDL_OUT_Y_L_ADDR       0x2A
#define LIS3MDL_OUT_Y_H_ADDR       0x2B
#define LIS3MDL_OUT_Z_L_ADDR       0x2C
#define LIS3MDL_OUT_Z_H_ADDR       0x2D

// WHO_AM_I Expected Value
#define LIS3MDL_WHO_AM_I_VALUE 0x3D



// Pin Definitions
#define LIS3MDL_CS_PORT GPIOG
#define LIS3MDL_CS_PIN GPIO_PIN_12
//extern GPIO_TypeDef LIS3MDL_CS_PORT;
//extern uint16_t LIS3MDL_CS_PIN;


extern uint8_t LIS3MDL_SETTINGS[8];

// SPI Communication Macros
#define LIS3MDL_CS_LOW()   HAL_GPIO_WritePin(LIS3MDL_CS_PORT, LIS3MDL_CS_PIN, GPIO_PIN_RESET)
#define LIS3MDL_CS_HIGH()  HAL_GPIO_WritePin(LIS3MDL_CS_PORT, LIS3MDL_CS_PIN, GPIO_PIN_SET)

/** The magnetometer ranges */
typedef enum {
    LIS3MDL_RANGE_4_GAUSS = 0b00,  ///< +/- 4g (default value)
    LIS3MDL_RANGE_8_GAUSS = 0b01,  ///< +/- 8g
    LIS3MDL_RANGE_12_GAUSS = 0b10, ///< +/- 12g
    LIS3MDL_RANGE_16_GAUSS = 0b11, ///< +/- 16g
  } lis3mdl_range_t;
  
  /** The magnetometer data rate, includes FAST_ODR bit */
  typedef enum {
    LIS3MDL_DATARATE_0_625_HZ = 0b0000, ///<  0.625 Hz
    LIS3MDL_DATARATE_1_25_HZ = 0b0010,  ///<  1.25 Hz
    LIS3MDL_DATARATE_2_5_HZ = 0b0100,   ///<  2.5 Hz
    LIS3MDL_DATARATE_5_HZ = 0b0110,     ///<  5 Hz
    LIS3MDL_DATARATE_10_HZ = 0b1000,    ///<  10 Hz
    LIS3MDL_DATARATE_20_HZ = 0b1010,    ///<  20 Hz
    LIS3MDL_DATARATE_40_HZ = 0b1100,    ///<  40 Hz
    LIS3MDL_DATARATE_80_HZ = 0b1110,    ///<  80 Hz
    LIS3MDL_DATARATE_155_HZ = 0b0001,   ///<  155 Hz (FAST_ODR + UHP)
    LIS3MDL_DATARATE_300_HZ = 0b0011,   ///<  300 Hz (FAST_ODR + HP)
    LIS3MDL_DATARATE_560_HZ = 0b0101,   ///<  560 Hz (FAST_ODR + MP)
    LIS3MDL_DATARATE_1000_HZ = 0b0111,  ///<  1000 Hz (FAST_ODR + LP)
  } lis3mdl_dataRate_t;
  
  /** The magnetometer performance mode */
  typedef enum {
	LIS3MDL_PERFORMANCEMODE_LOWPOWER = 0b00,  ///< Low power mode
	LIS3MDL_PERFORMANCEMODE_MEDIUM = 0b01,    ///< Medium performance mode
	LIS3MDL_PERFORMANCEMODE_HIGH = 0b10,      ///< High performance mode
	LIS3MDL_PERFORMANCEMODE_ULTRAHIGH = 0b11, ///< Ultra-high performance mode
  } lis3mdl_performancemode_t;
  
  /** The magnetometer operation mode */
  typedef enum {
	LIS3MDL_OPERATIONMODE_CONTINUOUS = 0b00, ///< Continuous conversion mode
	LIS3MDL_OPERATIONMODE_SINGLE = 0b01,     ///< Single-shot conversion
	LIS3MDL_OPERATIONMODE_POWERDOWN = 0b11,  ///< Powered-down
  } lis3mdl_operationmode_t;



/**
 * @brief Write a single byte to a LIS3MDL register.
 * @param reg: Register address.
 * @param value: Value to write.
 */
void LIS3MDL_WriteRegister(uint8_t reg, uint8_t value);
/**
 * @brief Read a single byte from a LIS3MDL register.
 * @param reg: Register address.
 * @return The value read.
 */
uint8_t LIS3MDL_ReadRegister(uint8_t reg);

/**
 * @brief Read multiple bytes starting from a LIS3MDL register.
 * @param reg: Starting register address.
 * @param buffer: Buffer to store the data.
 * @param length: Number of bytes to read.
 */
void LIS3MDL_ReadRegisters(uint8_t reg, uint8_t *buffer, uint8_t len);

/**
 * @brief Initializes the magnetometer.
 * 
 * This function configures the SPI communication and sets up the GPIO pins
 * for the magnetometer. It also performs a self-test to ensure the sensor is
 * functioning correctly.
 * 
 * @return uint8_t Status of the initialization (0 for success, 1 for failure).
 */
uint8_t Mag_Init();

/**
 * @brief This function configures the mode, data rate, and range of the magnetometer.
 * 
 * @param mode The mode to set (0 for continuous, 1 for single).
 * @param dataRate The data rate to set (in Hz).
 * @param range The range to set (in Gauss).
 * @return uint8_t Status of the configuration (0 for success, 1 for failure).
 */
uint8_t Mag_SetConfig(uint8_t *settings);

/**
 * @brief Gets the current configuration of the magnetometer.
 * 
 * This function retrieves the current mode, data rate, and range of the magnetometer.
 * 
 * @param mode Pointer to store the current mode.
 * @param dataRate Pointer to store the current data rate.
 * @param range Pointer to store the current range.
 * @return uint8_t Status of the configuration retrieval (0 for success, 1 for failure).
 */
uint8_t Mag_GetConfig();

/**
 * @brief Reads data from the magnetometer.
 * 
 * This function retrieves the magnetic field data from the magnetometer.
 * 
 * @param pData Pointer to store the magnetic field data (x, y, z components).
 * @return uint8_t Status of the read operation (0 for success, 1 for failure).
 */
uint8_t Mag_Read(vector_t *pData);

uint8_t Mag_Test();

#endif // MAG_H
