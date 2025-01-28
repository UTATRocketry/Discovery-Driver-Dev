/**
 ******************************************************************************
 * @file           : runcam_driver.h
 * @brief          : Header file for RunCam driver
 * 
 * @author         : Amelia Ellis
 * 
 * @details        : Provides function prototypes, macros, and structures for
 *                   interfacing with the RunCam camera over UART.
 ******************************************************************************
*/

#ifndef RUNCAM_DRIVER_H
#define RUNCAM_DRIVER_H

#include "stm32l4xx_hal.h"
#include <string.h>

// Command IDs
#define RUNCAM_CMD_GET_DEVICE_INFO   0x00
#define RUNCAM_CMD_CAMERA_CONTROL    0x01
#define RUNCAM_CMD_5KEY_SIMULATION   0x02
#define RUNCAM_CMD_GET_SETTINGS      0x10
#define RUNCAM_CMD_SETTING_DETAIL	 0x11
#define RUNCAM_CMD_WRITE_SETTING     0x13

// Action IDs for camera control
#define RUNCAM_ACTION_POWER         0x01
#define RUNCAM_ACTION_MODE          0x02
#define RUNCAM_ACTION_START_RECORD  0x03
#define RUNCAM_ACTION_STOP_RECORD   0x04

// Action IDs for 5 key simulation
// Setting IDs
#define RUNCAM_SETTING_CHARSET	        0
#define RUNCAM_SETTING_COLUMNS	        1
#define RUNCAM_SETTING_TV_MODE	        2 // TEXT_SELECTION
#define RUNCAM_SETTING_SDCARD_CAPACITY  3 // STRING
#define RUNCAM_SETTING_REMAINING_TIME   4 // STRING
#define RUNCAM_SETTING_RESOLUTION	    5 // TEXT_SELECTION
#define RUNCAM_SETTING_CAMERA_TIME	    6
#define NUM_RUNCAM_SETTINGS             7

// Miscellaneous constants
#define RUNCAM_CRC8POLY               0xD5
#define RUNCAM_PACKET_HEADER          0xCC
#define RUNCAM_INIT_TIMEOUT           500  // Timeout for initialization (ms)
#define RUNCAM_UART_TIMEOUT           1000 // UART timeout (ms)

// Status codes for driver functions
typedef enum {
    RUNCAM_OK = 0,
    RUNCAM_UART_TX_FAIL,
    RUNCAM_UART_RX_FAIL,
    RUNCAM_NO_RESPONSE,
    RUNCAM_INVALID_RESPONSE,
    RUNCAM_UNKNOWN_ERROR,
} runcam_status_t;

// Structure for storing a RunCam setting
typedef struct {
    uint8_t setting_id;          /**< Setting ID */
    char value[50];              /**< Value of the setting */
} runcam_setting_t;


// Function prototypes
runcam_status_t runcam_init(UART_HandleTypeDef *huart);
runcam_status_t runcam_get_device_info(uint8_t *response);
runcam_status_t runcam_get_setting(uint8_t setting_id, uint8_t *response, uint8_t chunk_index);
runcam_status_t runcam_get_all_settings(runcam_setting_t *settings);
runcam_status_t runcam_write_setting(uint8_t setting_id, uint8_t *value, uint8_t value_length);
runcam_status_t runcam_send_command(uint8_t command, uint8_t action);
uint8_t runcam_calculate_crc(uint8_t *data, uint8_t length);

const char* runcam_status_to_string(runcam_status_t status);


// Camera control functions
void runcam_start_recording(void);
void runcam_stop_recording(void);
void runcam_change_mode(void);
void runcam_power_toggle(void);

#endif /* RUNCAM_DRIVER_H */
