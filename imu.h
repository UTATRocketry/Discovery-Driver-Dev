/**
 * @file imu.h
 * @brief Header file for IMU driver
 * 
 * This file contains the definitions and function prototypes for
 * initializing and reading data from the IMU sensor. It includes
 * the necessary structures, function prototypes, and constants
 * required for the IMU driver.
 * 
 * @authors Amelia Ellis
 * @date 2025-04-11
 * @version 0.1
 * @bug No known bugs.
 */
#ifndef IMU_H
#define IMU_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "stm32h7xx_hal.h"
#include "sensors_defs.h"


#define LSM6DSOX_WHO_AM_I_ID           0x6C // Device ID for LSM6DSOX

// ----- Register Map Macros -----
#define LSM6DSOX_WHO_AM_I_ADDR         0x0F // Device ID register
#define LSM6DSOX_FUNC_CFG_ACCESS       0x1 // Enable embedded functions register
#define LSM6DSOX_PIN_CTRL_ADDR         0x2 // Pin control register

#define LSM6DSOX_INT1_CTRL_ADDR        0x0D // Interrupt enable for data ready
#define LSM6DSOX_CTRL1_XL_ADDR         0x10 // Main accelerometer config register
#define LSM6DSOX_CTRL2_G_ADDR          0x11 // Main gyro config register
#define LSM6DSOX_CTRL3_C_ADDR          0x12 // Main configuration register
#define LSM6DSOX_CTRL8_XL_ADDR         0x17 // High and low pass for accel
#define LSM6DSOX_CTRL10_C_ADDR         0x19 // Main configuration register
#define LSM6DSOX_STATUS_REG_ADDR       0x1E // Status register
#define LSM6DSOX_OUTX_L_G_ADDR         0x22 // First gyro data register
#define LSM6DSOX_OUTX_L_XL_ADDR         0x28 // First accel data register
#define LSM6DSOX_OUT_TEMP_L_ADDR       0x20 // First data register (temperature low)
#define LSM6DSOX_STEPCOUNTER_ADDR      0x4B // 16-bit step counter
#define LSM6DSOX_TAP_CFG_ADDR          0x58 // Tap/pedometer configuration
#define LSM6DSOX_WAKEUP_THS_ADDR       0x5B // Single and double-tap function threshold register
#define LSM6DSOX_WAKEUP_DUR_ADDR       0x5C // Free-fall, wakeup, timestamp and sleep mode duration


/** The accelerometer data rate */
typedef enum data_rate {
    LSM6DS_RATE_SHUTDOWN,
    LSM6DS_RATE_12_5_HZ,
    LSM6DS_RATE_26_HZ,
    LSM6DS_RATE_52_HZ,
    LSM6DS_RATE_104_HZ,
    LSM6DS_RATE_208_HZ,
    LSM6DS_RATE_416_HZ,
    LSM6DS_RATE_833_HZ,
    LSM6DS_RATE_1_66K_HZ,
    LSM6DS_RATE_3_33K_HZ,
    LSM6DS_RATE_6_66K_HZ,
  } lsm6ds_data_rate_t;
  
  /** The accelerometer data range */
  typedef enum accel_range {
    LSM6DS_ACCEL_RANGE_2_G,
    LSM6DS_ACCEL_RANGE_16_G,
    LSM6DS_ACCEL_RANGE_4_G,
    LSM6DS_ACCEL_RANGE_8_G
  } lsm6ds_accel_range_t;
  
  /** The gyro data range */
  typedef enum gyro_range {
    LSM6DS_GYRO_RANGE_125_DPS = 0b0010,
    LSM6DS_GYRO_RANGE_250_DPS = 0b0000,
    LSM6DS_GYRO_RANGE_500_DPS = 0b0100,
    LSM6DS_GYRO_RANGE_1000_DPS = 0b1000,
    LSM6DS_GYRO_RANGE_2000_DPS = 0b1100,
    ISM330DHCX_GYRO_RANGE_4000_DPS = 0b0001
  } lsm6ds_gyro_range_t;
  
  /** The high pass filter bandwidth */
  typedef enum hpf_range {
    LSM6DS_HPF_ODR_DIV_50 = 0,
    LSM6DS_HPF_ODR_DIV_100 = 1,
    LSM6DS_HPF_ODR_DIV_9 = 2,
    LSM6DS_HPF_ODR_DIV_400 = 3,
  } lsm6ds_hp_filter_t;

// Pin Definitions
#define LSM6DSOX_CS_PORT GPIOG
#define LSM6DSOX_CS_PIN GPIO_PIN_10
#define LSM6DSOX_CS_LOW()   HAL_GPIO_WritePin(LSM6DSOX_CS_PORT, LSM6DSOX_CS_PIN, GPIO_PIN_RESET)
#define LSM6DSOX_CS_HIGH()  HAL_GPIO_WritePin(LSM6DSOX_CS_PORT, LSM6DSOX_CS_PIN, GPIO_PIN_SET)

uint8_t IMU_Init(Sensor_t *self);
uint8_t IMU_Read(Sensor_t *self, vector_t *pGyroData, vector_t *pAccelData);


#endif // IMU_H