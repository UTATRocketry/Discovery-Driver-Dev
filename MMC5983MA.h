/**
 * @file mag.h
 * @brief Header file for magnetometer driver
 *
 * This file contains the definitions and function prototypes for
 * initializing and reading data from the magnetometer. It includes
 * the necessary structures, function prototypes, and constants
 * required for the magnetometer driver.
 *
 * @authors Eric Chiang
 * @date 2025-10-18
 * @version 0.1
 * @bug No known bugs.
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef MMC5983MA_DRIVER_H_
#define MMC5983MA_DRIVER_H_

// Includes
#include "stm32g0xx_hal.h"
#include "sensors_defs.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// SPI Handle
extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart2;

// Pin Definitions
#define MMC5983MA_CS_PORT GPIOA
#define MMC5983MA_CS_PIN GPIO_PIN_0

// Register Addresses
#define MMC5983MA_WHO_AM_I      0x2F
#define MMC5983MA_XOUT_H     	0x00
#define MMC5983MA_XOUT_L     	0x01
#define MMC5983MA_YOUT_H     	0x02
#define MMC5983MA_YOUT_L     	0x03
#define MMC5983MA_ZOUT_H     	0x04
#define MMC5983MA_ZOUT_L     	0x05
#define MMC5983MA_XYZOUT     	0x06 // If in 18 bit mode, this register contains the last two bits of each orientation
#define MMC5983MA_TOUT       	0x07
#define MMC5983MA_STATUS     	0x08
#define MMC5983MA_CTRL0     	0x09
#define MMC5983MA_CTRL1     	0x0A
#define MMC5983MA_CTRL2     	0x0B
#define MMC5983MA_CTRL3     	0x0C

// WHO_AM_I Expected Value
#define MMC5983MA_WHO_AM_I_VALUE 0x30

// Magnetometer Settings
typedef enum {
    MMC5983MA_BANDWIDTH_100HZ = 0b00,
    MMC5983MA_BANDWIDTH_200HZ = 0b01,
    MMC5983MA_BANDWIDTH_400HZ = 0b10,
    MMC5983MA_BANDWIDTH_800HZ = 0b11
} mmc5983ma_bandwith_t;

typedef enum {
    MMC5983MA_MEASUREMENT_OFF = 0b000,
    MMC5983MA_MEASUREMENT_1HZ = 0b001,
    MMC5983MA_MEASUREMENT_10HZ = 0b010,
    MMC5983MA_MEASUREMENT_20HZ = 0b011,
    MMC5983MA_MEASUREMENT_50HZ = 0b100,
    MMC5983MA_MEASUREMENT_100HZ = 0b101,
    MMC5983MA_MEASUREMENT_200HZ = 0b110,
    MMC5983MA_MEASUREMENT_1000HZ = 0b111
} mmc5983ma_measurement_freq_t;

typedef enum {
    MMC5983MA_SET_1 = 0b000,
    MMC5983MA_SET_25 = 0b001,
    MMC5983MA_SET_75 = 0b010,
    MMC5983MA_SET_100 = 0b011,
    MMC5983MA_SET_250 = 0b100,
    MMC5983MA_SET_500 = 0b101,
    MMC5983MA_SET_1000 = 0b110,
    MMC5983MA_SET_2000 = 0b111,
} mmc5983ma_set_count_t;

// Function Prototypes
/**
 * @brief Write a single byte to a MMC5983MA register.
 * @param addr: Register address to write.
 * @param value: Value to write.
 */
void MMC5983MA_WriteRegister(uint8_t addr, uint8_t value);

/**
 * @brief Read a single byte from a MMC5983MA register.
 * @param addr: Register addres to read.
 * @return The byte value of the register
 */
uint8_t MMC5983MA_ReadRegister(uint8_t addr);

/**
 * @brief Read a single byte from a MMC5983MA register.
 * @param addr: Register addres to read.
 * @param buffer: The buffer to write returned values to.
 * @param length: The length of the input buffer.
 */
void MMC5983MA_ReadRegisters(uint8_t addr, uint8_t *buffer, uint8_t length);

/**
 * @brief Initializes the magnetometer.
 *
 * This function configures the SPI communication and sets up the GPIO pins
 * for the magnetometer. It also performs a self-test to ensure the sensor is
 * functioning correctly.
 *
 * @return Status of the initialization (0 for success, 1 for failure).
 */
uint8_t MMC5983MA_Init(void);

/**
 * @brief Read the output of the magnetometer and stores it into the output data container.
 * 	Resolution of the output is 16 bits/0.25 mG
 * @param mag_data: The container for the data
 */
void MMC5983MA_ReadMagneticField16(vector_t* mag_data);

/**
 * @brief Read the output of the magnetometer and stores it into the output data container
 * Resolution of the output is 18 bits/0.0625 mG
 * @param mag_data: The container for the data
 */
void MMC5983MA_ReadMagneticField18(vector_t* mag_data);

/**
 * @brief Change the functionality of the magnetometer
 * @param ctrl_reg: One of four CTRL register addreses for the MMC5983MA
 * @param val: The byte to be writtern to the CTRL registers
 */

float MMC5983MA_16Bits_to_mGauss(uint16_t mag_val);

/**
 * @brief Convert the 18 bit output of the magnetometer to mGauss
 * @param ctrl_reg: One of the X, Y, Z magnetometer outputs
 * @return The value of the magnetometer output in mGauss
 */
float MMC5983MA_18Bits_to_mGauss(uint32_t mag_val);


/**
 * @brief Get the die temperature of the MMC5983MA
 * @return The temperature in celsius
 */
float MMC5983MA_Get_Temp(void);

/**
 * @brief Software Reset of the magnetometer. Clears all registers and self-tests on reset.
 */
void MMC5983MA_SW_Reset(void);

/**
 * @brief Transmits out the x, y, z values of the magnetometer to externally calibrate with MotionCal
 * @param mag_data: The output data container for the magnetometer in milligauss
 */
void MMC5983MA_Calibrate_MotionCal(vector_t* mag_data);

/**
 * @brief Applies calibration values to the magnetometer data
 * @param mag_data: The output data container for the magnetometer in milligauss
 */
void MMC5983MA_Calibrate_Data(vector_t* mag_data);

#endif // MMC5983MA_DRIVER_H_
