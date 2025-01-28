/**
 ******************************************************************************
 * @file           : runcam_driver.c
 * @brief          : Source file for RunCam driver
 * 
 * @author         : Amelia Ellis
 * 
 * @details        : Implements functions for interacting with the RunCam
 *                   camera via UART, including commands for control and
 *                   configuration.
 ******************************************************************************
*/

#include "runcam_driver.h"

// Static UART handle for internal use
static UART_HandleTypeDef *runcam_huart;

// Global status variable
static runcam_status_t runcam_status = RUNCAM_OK;

// Known settings (for logging or debugging)
static const char *runcam_setting_names[NUM_RUNCAM_SETTINGS] = {
    "Charset",
    "Columns",
    "TV Mode",
    "SD Card Capacity",
    "Remaining Recording Time",
    "Resolution",
    "Camera Time"
};

/**
 * @brief  Get the current RunCam status
 * @return      runcam_status_t  RUNCAM_OK on success, otherwise error code.
 */
runcam_status_t runcam_get_status(void) {
    return runcam_status;
}


/**
 * @brief  Sends a command packet to the RunCam and receives a response.
 * 
 * @param[in]   packet       Pointer to the command packet.
 * @param[in]   packet_len   Length of the command packet.
 * @param[out]  response     Pointer to the response buffer.
 * @param[in]   response_len Expected length of the response buffer.
 * 
 * @return      runcam_status_t  RUNCAM_OK on success, otherwise error code.
 */
static runcam_status_t runcam_transact(uint8_t *packet, uint8_t packet_len, uint8_t *response, uint8_t response_len) {
    // Transmit command packet
    if (HAL_UART_Transmit(runcam_huart, packet, packet_len, RUNCAM_UART_TIMEOUT) != HAL_OK) {
    	return (runcam_status = RUNCAM_UART_TX_FAIL);
    }

    // Receive response packet
    if (HAL_UART_Receive(runcam_huart, response, response_len, RUNCAM_UART_TIMEOUT) == HAL_TIMEOUT) {
        return (runcam_status = RUNCAM_NO_RESPONSE);
    }
    if (HAL_UART_Receive(runcam_huart, response, response_len, RUNCAM_UART_TIMEOUT) == HAL_ERROR) {
        return (runcam_status = RUNCAM_INVALID_RESPONSE);
    }
    if (HAL_UART_Receive(runcam_huart, response, response_len, RUNCAM_UART_TIMEOUT) != HAL_OK) {
        return (runcam_status = RUNCAM_UNKNOWN_ERROR);
    }
    // Validate response header
    if (response[0] != RUNCAM_PACKET_HEADER) {
        return (runcam_status = RUNCAM_INVALID_RESPONSE);
    }

    return (runcam_status = RUNCAM_OK);
}


/**
 * @brief  Returns a string representation of the RunCam status code.
 * 
 * @param[in]   status  Status code to convert to a string.
 * 
 * @return      const char*  Pointer to the status string.
 */
const char* runcam_status_to_string(runcam_status_t status) {
    switch (status) {
        case RUNCAM_OK:
            return "Success";
        case RUNCAM_UART_TX_FAIL:
            return "RunCam UART Transmit Failed";
        case RUNCAM_NO_RESPONSE:
            return "RunCam UART Receive Failed, No Response from Device";
        case RUNCAM_INVALID_RESPONSE:
            return "RunCam UART Receive Failed, Invalid Response from Device";
        case RUNCAM_UNKNOWN_ERROR:
            return "Unknown Error";
        default:
            return "Invalid Status Code";
    }
}

/**
 * @brief  Initializes the RunCam driver with a UART handle.
 * 
 * @param[in]   huart  Pointer to the UART handle for communication.
 * @return      runcam_status_t  RUNCAM_OK on success, otherwise error code.
 */
runcam_status_t runcam_init(UART_HandleTypeDef *huart) {
    runcam_huart = huart;

    uint8_t packet[3] = { RUNCAM_PACKET_HEADER, RUNCAM_CMD_GET_DEVICE_INFO, 0 };
    packet[2] = runcam_calculate_crc(packet, 2);

    uint8_t response[5] = {0};
    return runcam_transact(packet, sizeof(packet), response, sizeof(response));
}

/**
 * @brief  Retrieves device information from the RunCam.
 * 
 * @param[out]  response  Buffer to store the response from the camera.
 * @return      runcam_status_t  RUNCAM_OK on success, otherwise error code.
 */
runcam_status_t runcam_get_device_info(uint8_t *response) {
    uint8_t packet[3] = { RUNCAM_PACKET_HEADER, RUNCAM_CMD_GET_DEVICE_INFO, 0 };
    packet[2] = runcam_calculate_crc(packet, 2);

    return runcam_transact(packet, sizeof(packet), response, 5);
}

/**
 * @brief  Retrieves a single setting from the RunCam.
 * 
 * @param[in]   setting_id   ID of the setting to retrieve.
 * @param[out]  response     Buffer to store the setting response.
 * @param[in]   chunk_index  Chunk index for paginated settings (if applicable).
 * 
 * @return      runcam_status_t  RUNCAM_OK on success, otherwise error code.
 */
runcam_status_t runcam_get_setting(uint8_t setting_id, uint8_t *response, uint8_t chunk_index) {
    uint8_t packet[5] = { RUNCAM_PACKET_HEADER, RUNCAM_CMD_GET_SETTINGS, setting_id, chunk_index, 0 };
    packet[4] = runcam_calculate_crc(packet, 4);

    return runcam_transact(packet, sizeof(packet), response, 10);
}

/**
 * @brief  Retrieves all settings from the RunCam.
 * 
 * @param[out]  settings  Array of RunCam_Setting structures to store the results.
 * 
 * @return      runcam_status_t  RUNCAM_OK on success, otherwise error code.
 */
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

/**
 * @brief  Writes a new value to a specified setting on the RunCam.
 * 
 * @param[in]   setting_id    ID of the setting to write.
 * @param[in]   value         Pointer to the new value.
 * @param[in]   value_length  Length of the value.
 * 
 * @return      runcam_status_t  RUNCAM_OK on success, otherwise error code.
 */
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

/**
 * @brief  Sends a command to the RunCam.
 * 
 * @param[in]   command  Command ID (e.g., camera control, 5-key simulation).
 * @param[in]   action   Action ID for the specified command.
 * 
 * @return      runcam_status_t  RUNCAM_OK on success, otherwise error code.
 */
runcam_status_t runcam_send_command(uint8_t command, uint8_t action) {
    uint8_t packet[4] = { RUNCAM_PACKET_HEADER, command, action, 0 };
    packet[3] = runcam_calculate_crc(packet, 3);

    uint8_t response[4];
    return runcam_transact(packet, sizeof(packet), response, sizeof(response));
}

/**
 * @brief  Calculates the CRC-8 checksum for a data packet.
 * 
 * @param[in]   data    Pointer to the data buffer.
 * @param[in]   length  Length of the data buffer.
 * @return      uint8_t  Calculated CRC-8 value.
 */
uint8_t runcam_calculate_crc(uint8_t *data, uint8_t length) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ RUNCAM_CRC8POLY;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

/**
 * @brief  Starts video recording on the RunCam.
 */
void runcam_start_recording(void) {
    runcam_send_command(RUNCAM_CMD_CAMERA_CONTROL, RUNCAM_ACTION_START_RECORD);
}

/**
 * @brief  Stops video recording on the RunCam.
 */
void runcam_stop_recording(void) {
    runcam_send_command(RUNCAM_CMD_CAMERA_CONTROL, RUNCAM_ACTION_STOP_RECORD);
}

/**
 * @brief  Changes the mode of the RunCam.
 */
void runcam_change_mode(void) {
    runcam_send_command(RUNCAM_CMD_CAMERA_CONTROL, RUNCAM_ACTION_MODE);
}

/**
 * @brief  Toggles the power state of the RunCam.
 */
void runcam_power_toggle(void) {
    runcam_send_command(RUNCAM_CMD_CAMERA_CONTROL, RUNCAM_ACTION_POWER);
}

