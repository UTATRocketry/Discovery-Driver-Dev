/*
 * NEOM9N.c
 *
 *  Created on: Mar 18, 2025
 *      Author: William Gomez
 *      Modified by: Pierce Luu
 */
#include "NEOM9N.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

float convert_nmea_to_decimal(float nmea, char hemisphere) {
    int degrees = (int)(nmea / 100);
    float minutes = nmea - (degrees * 100);
    float decimal = degrees + (minutes / 60.0f);
    if (hemisphere == 'S' || hemisphere == 'W') decimal = -decimal;
    return decimal;
}

void calc_checksum(uint8_t *msg, uint16_t len, uint8_t *ck_a, uint8_t *ck_b) {
    *ck_a = 0;
    *ck_b = 0;
    for (uint16_t i = 0; i < len; i++) {
        *ck_a += msg[i];
        *ck_b += *ck_a;
    }
}

/* AI-Generated: Cursor AI (Claude Sonnet 4.5) - Dec 2025 */
static uint8_t calc_nmea_checksum(const char *sentence, int len) {
    uint8_t checksum = 0;
    for (int i = 1; i < len; i++) {
        if (sentence[i] == '*' || sentence[i] == '\r' || sentence[i] == '\n') {
            break;
        }
        checksum ^= (uint8_t)sentence[i];
    }
    return checksum;
}

static int validate_nmea_checksum(const char *sentence) {
    const char *checksum_start = strchr(sentence, '*');
    if (checksum_start == NULL) {
        return 0;
    }
    char checksum_str[3] = {0};
    checksum_str[0] = checksum_start[1];
    checksum_str[1] = checksum_start[2];
    
    uint8_t received_checksum = (uint8_t)strtol(checksum_str, NULL, 16);
    int sentence_len = checksum_start - sentence;
    uint8_t calculated_checksum = calc_nmea_checksum(sentence, sentence_len);
    
    return (received_checksum == calculated_checksum);
}

UART_HandleTypeDef* uartAddress;

#define GPS_BUFFER_SIZE 2048

static uint8_t buffer1[GPS_BUFFER_SIZE];
static uint8_t buffer2[GPS_BUFFER_SIZE];
static uint8_t* volatile write_buffer = buffer1;
static uint8_t* volatile read_buffer = buffer2;
static volatile uint16_t write_size = 0;
static volatile uint8_t interrupt_happened = 0;
static volatile uint8_t buffer_locked = 0;
static volatile uint32_t interrupt_count = 0;
static volatile uint32_t total_bytes_received = 0;

int NEOM9N_init(UART_HandleTypeDef* uartAddressPin) {
    if (uartAddressPin == NULL) {
        return HAL_ERROR;
    }
    
    uartAddress = uartAddressPin;
    
    // Initialize double buffer system
    memset(buffer1, 0, GPS_BUFFER_SIZE);
    memset(buffer2, 0, GPS_BUFFER_SIZE);
    interrupt_happened = 0;
    buffer_locked = 0;
    write_size = 0;
    write_buffer = buffer1;
    read_buffer = buffer2;
    interrupt_count = 0;
    total_bytes_received = 0;

    // Configure GPS: Set update rate to 100ms (10Hz)
    uint8_t ubx_cfg_rate[] = {
        0xB5, 0x62,
        0x06, 0x08,
        0x06, 0x00,
        0x64, 0x00,
        0x01, 0x00,
        0x01, 0x00,
        0x00, 0x00
    };

    uint8_t ck_a, ck_b;
    calc_checksum(&ubx_cfg_rate[2], 10, &ck_a, &ck_b);
    ubx_cfg_rate[12] = ck_a;
    ubx_cfg_rate[13] = ck_b;

    if (HAL_UART_Transmit(uartAddress, ubx_cfg_rate, 14, 2000) != HAL_OK) {
        return HAL_ERROR;
    }
    HAL_Delay(100);

    // Configure GPS: Enable NMEA GGA messages (for altitude)
    uint8_t ubx_cfg_msg_gga[] = {
        0xB5, 0x62,
        0x06, 0x01,
        0x08, 0x00,
        0xF0, 0x00,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00, 0x00
    };
    
    calc_checksum(&ubx_cfg_msg_gga[2], 12, &ck_a, &ck_b);
    ubx_cfg_msg_gga[14] = ck_a;
    ubx_cfg_msg_gga[15] = ck_b;

    if (HAL_UART_Transmit(uartAddress, ubx_cfg_msg_gga, 16, 2000) != HAL_OK) {
        return HAL_ERROR;
    }
    HAL_Delay(100);

    // Configure GPS: Enable NMEA RMC messages (for position, time, speed)
    uint8_t ubx_cfg_msg_rmc[] = {
        0xB5, 0x62,
        0x06, 0x01,
        0x08, 0x00,
        0xF0, 0x04,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00, 0x00
    };
    
    calc_checksum(&ubx_cfg_msg_rmc[2], 12, &ck_a, &ck_b);
    ubx_cfg_msg_rmc[14] = ck_a;
    ubx_cfg_msg_rmc[15] = ck_b;

    if (HAL_UART_Transmit(uartAddress, ubx_cfg_msg_rmc, 16, 2000) != HAL_OK) {
        return HAL_ERROR;
    }
    HAL_Delay(100);

    // Start interrupt-based reception
    if (HAL_UARTEx_ReceiveToIdle_IT(uartAddress, write_buffer, GPS_BUFFER_SIZE) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

int NEOM9N_getData(unsigned char *GPSData) {
    if (GPSData == NULL) {
        return HAL_ERROR;
    }

    if (interrupt_happened == 0) {
        return HAL_ERROR;
    }

    // CRITICAL SECTION: Atomic buffer swap
    __disable_irq();
    
    if (interrupt_happened == 0) {
        __enable_irq();
        return HAL_ERROR;
    }

    buffer_locked = 1;

    uint8_t* temp = read_buffer;
    read_buffer = write_buffer;
    write_buffer = temp;

    uint16_t size = write_size;
    write_size = 0;
    interrupt_happened = 0;
    buffer_locked = 0;

    __enable_irq();
    // END CRITICAL SECTION

    if (size == 0 || size >= GPS_BUFFER_SIZE) {
        HAL_UARTEx_ReceiveToIdle_IT(uartAddress, write_buffer, GPS_BUFFER_SIZE);
        return HAL_ERROR;
    }

    memcpy(GPSData, read_buffer, size);
    GPSData[size] = '\0';

    if (HAL_UARTEx_ReceiveToIdle_IT(uartAddress, write_buffer, GPS_BUFFER_SIZE) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

int NEOM9N_isDataReady(void) {
    return interrupt_happened;
}

void NEOM9N_getDiagnostics(uint32_t* interrupt_count_out, uint32_t* total_bytes_out) {
    if (interrupt_count_out) *interrupt_count_out = interrupt_count;
    if (total_bytes_out) *total_bytes_out = total_bytes_received;
}

// INTERRUPT HANDLER: Called when UART receives data (IDLE line detected)
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    if (huart->Instance == uartAddress->Instance) {
        if (Size == 0 || Size > GPS_BUFFER_SIZE) {
            HAL_UARTEx_ReceiveToIdle_IT(uartAddress, write_buffer, GPS_BUFFER_SIZE);
            return;
        }
        
        interrupt_count++;
        total_bytes_received += Size;
        
        if (buffer_locked == 0) {
            write_size = Size;
            interrupt_happened = 1;
        }
    }
}

// INTERRUPT HANDLER: Called when UART receive buffer is full
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == uartAddress->Instance) {
        interrupt_count++;
        total_bytes_received += GPS_BUFFER_SIZE;
        
        if (buffer_locked == 0) {
            write_size = GPS_BUFFER_SIZE;
            interrupt_happened = 1;
        }
    }
}


/* AI-Generated: Cursor AI (Claude Sonnet 4.5) - Jan 2026 */

int NEOM9N_getPosition(float* latitude, char* latitudeHemisphere,
                       float* longitude, char* longitudeHemisphere,
                       unsigned char* GPSData, int size) {
    if (GPSData == NULL) {
        return 1;
    }

    // PARSE START: Search for RMC sentence (contains position data)
    const char* start = strstr((const char*)GPSData, "$GNRMC,");
    if (!start) {
        start = strstr((const char*)GPSData, "$GPRMC,");
        if (!start) {
            return 1;  // Not found
        }
    }

    // Extract sentence (copy to avoid modifying original)
    char sentence[200];
    int i = 0;
    while (start[i] != '\n' && start[i] != '\r' && start[i] != '\0' && i < 199) {
        sentence[i] = start[i];
        i++;
    }
    sentence[i] = '\0';

    
    // FIELD PARSING START: Tokenize comma-separated fields
    char sentence_copy[200];
    strncpy(sentence_copy, sentence, sizeof(sentence_copy) - 1);
    sentence_copy[sizeof(sentence_copy) - 1] = '\0';

    char* token;
    int field = 0;
    char latStr[20] = {0}, lonStr[20] = {0};
    char status = 'V';
    char latHem = 'N', lonHem = 'E';

    token = strtok(sentence_copy, ",");
    while (token != NULL) {
        // RMC fields: 0=$GNRMC, 1=time, 2=status, 3=lat, 4=NS, 5=lon, 6=EW, 7=speed, 8=course, 9=date
        if (field == 2) {
            status = token[0];  // A=valid fix, V=invalid
        } else if (field == 3) {
            strncpy(latStr, token, sizeof(latStr) - 1);  // Latitude in NMEA format
        } else if (field == 4) {
            if (token[0] != '\0') {
                latHem = token[0];  // N or S
            }
        } else if (field == 5) {
            strncpy(lonStr, token, sizeof(lonStr) - 1);  // Longitude in NMEA format
        } else if (field == 6) {
            if (token[0] != '\0') {
                lonHem = token[0];  // E or W
            }
        }
        token = strtok(NULL, ",");
        field++;
    }

    // Validate GPS fix status
    if (status != 'A') {
        return 1;  // No valid fix
    }

    // Validate position data exists
    if (latStr[0] == '\0' || lonStr[0] == '\0' || field < 10) {
        if (latitude) *latitude = 0;
        if (longitude) *longitude = 0;
        if (latitudeHemisphere) *latitudeHemisphere = '\0';
        if (longitudeHemisphere) *longitudeHemisphere = '\0';
        return 2;
    }

    // Convert NMEA format (ddmm.mmmmm) to decimal degrees
    float nmeaLat = atof(latStr);
    float nmeaLon = atof(lonStr);

    if (latitude) {
        *latitude = convert_nmea_to_decimal(nmeaLat, latHem);
    }
    if (longitude) {
        *longitude = convert_nmea_to_decimal(nmeaLon, lonHem);
    }
    if (latitudeHemisphere) {
        *latitudeHemisphere = latHem;
    }
    if (longitudeHemisphere) {
        *longitudeHemisphere = lonHem;
    }

    return 0;
}

int NEOM9N_getTime(float* time, unsigned char* GPSData, int size) {
    if (GPSData == NULL || time == NULL) {
        return -1;
    }

    // PARSE START: Search for RMC sentence (contains time data)
    const char* start = strstr((const char*)GPSData, "$GNRMC,");
    if (!start) {
        start = strstr((const char*)GPSData, "$GPRMC,");
        if (!start) {
            return -1;  // Not found
        }
    }

    // Extract time field (first field after $GNRMC,)
    char timeStr[20] = {0};
    int i = 0, j = 0;
    while (start[i] && start[i] != ',') i++;  // Skip "$GNRMC,"
    if (start[i] == ',') i++;
    while (start[i] && start[i] != ',' && j < 19) {  // Copy time until next comma
        timeStr[j++] = start[i++];
    }
    timeStr[j] = '\0';

    if (timeStr[0] == '\0') {
        return -2;
    }

    *time = atof(timeStr);
    return 0;
}

int NEOM9N_getSpeed(float* speed, unsigned char* GPSData) {
    if (GPSData == NULL || speed == NULL) {
        return -1;
    }

    // PARSE START: Search for RMC sentence (contains speed data)
    const char* start = strstr((const char*)GPSData, "$GNRMC,");
    if (!start) {
        start = strstr((const char*)GPSData, "$GPRMC,");
        if (!start) {
            return -1;  // Not found
        }
    }

    // Extract sentence (copy to avoid modifying original)
    char sentence[200];
    int i = 0;
    while (start[i] != '\n' && start[i] != '\r' && start[i] != '\0' && i < 199) {
        sentence[i] = start[i];
        i++;
    }
    sentence[i] = '\0';

    // FIELD PARSING START: Tokenize to extract speed field
    char sentence_copy[200];
    strncpy(sentence_copy, sentence, sizeof(sentence_copy) - 1);
    sentence_copy[sizeof(sentence_copy) - 1] = '\0';

    char* token;
    int field = 0;
    char speedStr[20] = {0};

    token = strtok(sentence_copy, ",");
    while (token != NULL) {
        if (field == 7) {  // Speed is field 7 in RMC sentence
            strncpy(speedStr, token, sizeof(speedStr) - 1);
            break;
        }
        token = strtok(NULL, ",");
        field++;
    }

    if (speedStr[0] == '\0') {
        return -2;
    }

    *speed = atof(speedStr);
    return 0;
}

int NEOM9N_getAltitude(float* altitude, unsigned char* GPSData) {
    if (GPSData == NULL || altitude == NULL) {
        return -1;
    }

    // PARSE START: Search for GGA sentence (contains altitude data)
    const char* start = strstr((const char*)GPSData, "$GNGGA,");
    if (!start) {
        start = strstr((const char*)GPSData, "$GPGGA,");
        if (!start) {
            return -1;  // Not found
        }
    }

    // Extract sentence (copy to avoid modifying original)
    char sentence[200];
    int i = 0;
    while (start[i] != '\n' && start[i] != '\r' && start[i] != '\0' && i < 199) {
        sentence[i] = start[i];
        i++;
    }
    sentence[i] = '\0';


    
    // FIELD PARSING START: Tokenize to extract altitude field
    char sentence_copy[200];
    strncpy(sentence_copy, sentence, sizeof(sentence_copy) - 1);
    sentence_copy[sizeof(sentence_copy) - 1] = '\0';

    char* token;
    int field = 0;
    char altStr[20] = {0};

    token = strtok(sentence_copy, ",");
    while (token != NULL) {
        if (field == 9) {  // Altitude is field 9 in GGA sentence
            strncpy(altStr, token, sizeof(altStr) - 1);
            break;
        }
        token = strtok(NULL, ",");
        field++;
    }

    if (altStr[0] == '\0') {
        return -2;
    }

    *altitude = atof(altStr);
    return 0;
}
