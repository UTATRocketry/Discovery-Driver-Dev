/**
 * @file accel.h
 * @brief Simple ADXL375 driver with Calibration and custom Print
 */

#ifndef ACCEL_H
#define ACCEL_H

#include "stm32g0xx_hal.h"
#include <stdint.h>

// --- Register Map ---
#define ADXL375_REG_DEVID       0x00
#define ADXL375_REG_BW_RATE     0x2C
#define ADXL375_REG_POWER_CTL   0x2D
#define ADXL375_REG_DATA_FORMAT 0x31
#define ADXL375_REG_DATAX0      0x32
#define ADXL375_REG_FIFO_CTL    0x38

// --- Constants ---
#define ADXL375_DEVICE_ID       0xE5
#define ADXL375_SPI_TIMEOUT     100 // ms

// --- CS Pin Macros ---
#define ADXL375_CS_PORT   GPIOA
#define ADXL375_CS_PIN    GPIO_PIN_4
#define ADXL375_CS_LOW()  HAL_GPIO_WritePin(ADXL375_CS_PORT, ADXL375_CS_PIN, GPIO_PIN_RESET)
#define ADXL375_CS_HIGH() HAL_GPIO_WritePin(ADXL375_CS_PORT, ADXL375_CS_PIN, GPIO_PIN_SET)

// --- Data Structure ---
typedef struct {
    float x;
    float y;
    float z;
} vector_t;

// --- Function Prototypes ---
uint8_t Accel_Init(void);
void    Accel_Calibrate(void);
uint8_t Accel_Read(vector_t *data);

#endif // ACCEL_H
