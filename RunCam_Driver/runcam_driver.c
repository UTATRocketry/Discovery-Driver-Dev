/**
 ******************************************************************************
 * @file    runcam_driver.c
 * @author  Amelia Ellis
 * @brief   RunCam  driver source file
 ******************************************************************************
*/

#include "runcam_driver.h"

static UART_HandleTypeDef *runcam_huart;

// Known settings based on protocol documentation
const char *setting_names[NUM_RUNCAM_SETTINGS] = {
    "Charset",
    "Columns",
    "TV Mode",
    "SD Card Capacity",
    "Remaining Recording Time",
    "Resolution",
    "Camera Time"
};

// Global status variable
runcam_status_t runcam_status = RUNCAM_OK;

// Generic function to send and receive RunCam data
static runcam_status_t runcam_transact(uint8_t *packet, uint8_t packet_len, uint8_t *response, uint8_t response_len) {
    // Transmit command packet
    if (HAL_UART_Transmit(runcam_huart, packet, packet_len, RUNCAM_UART_TIMEOUT) != HAL_OK) {
    	return (runcam_status = RUNCAM_UART_TX_FAIL);
    }

    // Receive response packet
    if (HAL_UART_Receive(runcam_huart, response, response_len, RUNCAM_UART_TIMEOUT) != HAL_OK) {
    	return (runcam_status = RUNCAM_UART_RX_FAIL);
    }

    // Validate response header
    if (response[0] != RUNCAM_PACKET_HEADER) {
        return (runcam_status = RUNCAM_INVALID_RESPONSE);
    }

    return (runcam_status = RUNCAM_OK);
}

// Function to return error messages as strings
const char* runcam_status_to_string(runcam_status_t status) {
    switch (status) {
        case RUNCAM_OK:
            return "Success";
        case RUNCAM_UART_TX_FAIL:
            return "RunCam UART Transmit Failed";
        case RUNCAM_UART_RX_FAIL:
            return "RunCam UART Receive Failed, No Response from Device";
        case RUNCAM_INVALID_RESPONSE:
            return "RunCam UART Receive Failed, Invalid Response from Device";
        default:
            return "Unknown Error";
    }
}

runcam_status_t runcam_init(UART_HandleTypeDef *huart) {
    runcam_huart = huart;

    uint8_t packet[3] = { RUNCAM_PACKET_HEADER, RUNCAM_CMD_GET_DEVICE_INFO, 0 };
    packet[2] = runcam_calculate_crc(packet, 2);

    uint8_t response[5] = {0};
    return runcam_transact(packet, sizeof(packet), response, sizeof(response));
}

runcam_status_t runcam_get_device_info(uint8_t *response) {
    uint8_t packet[3] = { RUNCAM_PACKET_HEADER, RUNCAM_CMD_GET_DEVICE_INFO, 0 };
    packet[2] = runcam_calculate_crc(packet, 2);

    return runcam_transact(packet, sizeof(packet), response, 5);
}

runcam_status_t runcam_get_setting(uint8_t setting_id, uint8_t *response, uint8_t chunk_index) {
    uint8_t packet[5] = { RUNCAM_PACKET_HEADER, RUNCAM_CMD_GET_SETTINGS, setting_id, chunk_index, 0 };
    packet[4] = runcam_calculate_crc(packet, 4);

    return runcam_transact(packet, sizeof(packet), response, 10);
}

runcam_status_t runcam_get_all_settings(RunCam_Setting *settings) {
    for (uint8_t i = 0; i < NUM_RUNCAM_SETTINGS; i++) {
        uint8_t packet[5] = { RUNCAM_PACKET_HEADER, RUNCAM_CMD_GET_SETTINGS, i, 0, 0 };
        packet[4] = runcam_calculate_crc(packet, 4);

        uint8_t response[30] = {0};  // Max expected response length

        runcam_status = runcam_transact(packet, sizeof(packet), response, sizeof(response));
		if (runcam_status != RUNCAM_OK) {
			return runcam_status;  // Return the first error encountered
		}

        // Extract the setting value from the response, assuming the value starts at index 4
        settings[i].setting_id = i;
        strncpy(settings[i].value, (char*)&response[4], sizeof(settings[i].value) - 1);
        settings[i].value[sizeof(settings[i].value) - 1] = '\0';  // Ensure null termination
    }

    return RUNCAM_OK;
}

runcam_status_t runcam_write_setting(uint8_t setting_id, uint8_t *value, uint8_t value_length) {
    uint8_t packet[5 + value_length];
    packet[0] = RUNCAM_PACKET_HEADER;
    packet[1] = RUNCAM_CMD_WRITE_SETTING;
    packet[2] = setting_id;

    memcpy(&packet[3], value, value_length);
    packet[3 + value_length] = runcam_calculate_crc(packet, 3 + value_length);

    uint8_t response[4];
    return runcam_transact(packet, sizeof(packet), response, sizeof(response));
}

runcam_status_t runcam_send_command(uint8_t command, uint8_t action) {
    uint8_t packet[4] = { RUNCAM_PACKET_HEADER, command, action, 0 };
    packet[3] = runcam_calculate_crc(packet, 3);

    uint8_t response[4];
    return runcam_transact(packet, sizeof(packet), response, sizeof(response));
}

uint8_t crc8(uint8_t crc, uint8_t data) {
    crc ^= data;
    for (int i = 0; i < 8; i++) {
        if (crc & 0x80) {
            crc = (crc << 1) ^ RUNCAM_CRC8POLY;
        } else {
            crc <<= 1;
        }
    }
    return crc;
}

uint8_t runcam_calculate_crc(uint8_t *data, uint8_t length) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < length; i++) {
        crc = crc8(crc, data[i]);
    }
    return crc;
}

// Control the camera to start recording video
void runcam_start_recording(void) {
    runcam_send_command(RUNCAM_CMD_CAMERA_CONTROL, RUNCAM_ACTION_START_RECORD);
}

// Control the camera to stop recording video
void runcam_stop_recording(void) {
    runcam_send_command(RUNCAM_CMD_CAMERA_CONTROL, RUNCAM_ACTION_STOP_RECORD);
}

// Switch the device operating mode
void runcam_change_mode(void) {
    runcam_send_command(RUNCAM_CMD_CAMERA_CONTROL, RUNCAM_ACTION_MODE);
}

// Toggle the power button
void runcam_power_toggle(void) {
    runcam_send_command(RUNCAM_CMD_CAMERA_CONTROL, RUNCAM_ACTION_POWER);
}

