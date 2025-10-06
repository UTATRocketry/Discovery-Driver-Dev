/**
 * @file sensors.h
 * @brief Header file for sensor initialization and data reading functions.
 * 
 * This file contains the definitions for initializing and reading data from
 * various sensors including IMU, Barometer, Magnetometer, Accelerometer,
 * and GPS. It also includes function prototypes for temperature sensors.
 * 
 * @authors Amelia Ellis
 * @date 2025-04-10
 * @version 0.1
 * @bug No known bugs.
 * 
 * @todo Remove fake_hal.h and replace with actual HAL header.
 * @todo Add error handling for sensor initialization and data reading.
 */
#ifndef SENSORS_H
#define SENSORS_H

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include "stm32h7xx_hal.h"


#define SENSORS_MAGFIELD_EARTH_MAX                                             \
  (60.0F) /**< Maximum magnetic field on Earth's surface */
#define SENSORS_MAGFIELD_EARTH_MIN                                             \
  (30.0F) /**< Minimum magnetic field on Earth's surface */
#define SENSORS_PRESSURE_SEALEVELHPA                                           \
  (1013.25F) /**< Average sea level pressure is 1013.25 hPa */
#define SENSORS_DPS_TO_RADS                                                    \
  (0.017453293F) /**< Degrees/s to rad/s multiplier */
#define SENSORS_RADS_TO_DPS                                                    \
  (57.29577793F) /**< Rad/s to degrees/s  multiplier */
#define SENSORS_GAUSS_TO_MICROTESLA                                            \
  (100) /**< Gauss to micro-Tesla multiplier */
#define SENSORS_MICROTESLA_TO_GAUSS                                            \
  (0.01F) /**< Micro-Tesla to Gauss multiplier */
#define SENSORS_MG_TO_MS2                                                     \
  (0.00980665F) /**< mg to m/s^2 multiplier */
#define SENSORS_MS2_TO_MG                                                      \
  (101.971621F) /**< m/s^2 to mg multiplier */

#define SENSORS_GRAVITY_STANDARD (9.80665F) /**< Standard gravity in m/s^2 */


#define SENSORS_SPI_TIMEOUT 2000 /**< SPI timeout in milliseconds */

// SPI Handle
extern SPI_HandleTypeDef hspi1;

typedef union vector3_u {
  float v[3];
  struct {
	 float x, y, z;
  };
} vector_t;


#endif // SENSORS_H
