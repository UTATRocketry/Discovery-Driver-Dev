#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "stm32l4xx_hal.h"

/* ----------------------------------------------
 * Return Codes
 * ---------------------------------------------- */
typedef enum {
    GPS_CFG_OK = 0,
    GPS_CFG_ERR_UART = 1,     // HAL_UART_Transmit failed (DMA running? wrong baud?)
    GPS_CFG_ERR_NAK = 2,      // GPS rejected the config
    GPS_CFG_ERR_TIMEOUT = 3,  // No ACK received within timeout - GPS not responding
} GPS_CfgStatus;

/* ----------------------------------------------
 * Functions
 * ---------------------------------------------- */
// wait for ack from gps after sending a command
bool ubx_wait_ack(UART_HandleTypeDef* huart, uint8_t cls, uint8_t id,
                  uint32_t timeout_ms);

/* -----------------
 * Configure the GPS module.
 *
 * Set reinit_baud to false if the GPS is already running at GPS_TARGET_BAUD.
 * If both reinit_baud and reinit_nmea_rate are false, nothing happens
 * If both are true, only baud rate is changed
 ------------------- */
GPS_CfgStatus gps_configure(UART_HandleTypeDef* huart, bool reinit_baud,
                            bool reinit_nmea_rate);
