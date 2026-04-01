/*
 * NEOM9N.c
 *
 *  Created on: Mar 18, 2025
 *      Author: William Gomez
 *      Modified by: Aisha Aftab
 *  (Implementation with interupts and not polling)
 */
#include "NEOM9N.h"

/* -------------------------------------------------------------------------
   helper functions mostly built by AI
   ------------------------------------------------------------------------- */

float convert_nmea_to_decimal(float nmea, char hemisphere) {
    int degrees = (int)(nmea / 100);
    float minutes = nmea - (degrees * 100);
    float decimal = degrees + (minutes / 60.0f);
    if (hemisphere == 'S' || hemisphere == 'W') decimal = -decimal;
    return decimal;
}

void calc_checksum(uint8_t* msg, uint16_t len, uint8_t* ck_a, uint8_t* ck_b) {
    *ck_a = 0;
    *ck_b = 0;
    for (uint16_t i = 0; i < len; i++) {
        *ck_a += msg[i];
        *ck_b += *ck_a;
    }
}

/* -------------------------------------------------------------------------
   UART handle used by this driver
   ------------------------------------------------------------------------- */
UART_HandleTypeDef* uartAddress;

/* -------------------------------------------------------------------------
   Interrupt-based RX (Receive-to-IDLE) buffers/flags
   ------------------------------------------------------------------------- */

/* Exposed flags (declared extern in .h) */
volatile uint8_t NEOM9N_rxReady = 0;
volatile uint16_t NEOM9N_rxSize = 0;

/* HAL writes new bytes here each time an idle event occurs */
static uint8_t NEOM9N_itChunk[256];

/* Internal accumulation buffer that main can read from */
#define NEOM9N_ACCUM_SIZE 2048
static uint8_t NEOM9N_accum[NEOM9N_ACCUM_SIZE];
static volatile uint16_t NEOM9N_accumLen = 0;

/* - DEBUG BUFFER - */
unsigned char rx_buffDEBUG[1000];  // read buffer from NEO-M9N
unsigned char tx_buffDEBUG[1000];  // write buffer to serial output

/* -------------------------------------------------------------------------
   Driver Functions
   ------------------------------------------------------------------------- */

int NEOM9N_init(UART_HandleTypeDef* uartAddressPin) {
    uartAddress = uartAddressPin;
    int status = 0;

    status = NEOM9N_CheckConnection();
    if (status == HAL_ERROR) {
        return status;  // if GPS does not respond, return 1
    }
    // todo: continue to configure settings for GPS once connected/wait for cold start procedure to finish

    /*
      NOTE:
      The UBX configuration code below was in the original file.
      It is left here to preserve intent, but keep in mind:
      - UBX is binary (strlen is not safe)
      - It should be sent on the GPS UART (uartAddress), not necessarily hlpuart1
      If you do not need config, you can ignore this part.
    */

    return 0;
}

/*
 * Starts UART reception using interrupt method (Receive-to-IDLE).
 * Call once after NEOM9N_init() in main.c.
 */
void NEOM9N_StartRxIT(void) {
    NEOM9N_accumLen = 0;
    NEOM9N_rxReady = 0;
    NEOM9N_rxSize = 0;

    // Start receiving into the chunk buffer
    HAL_UARTEx_ReceiveToIdle_IT(uartAddress, NEOM9N_itChunk, sizeof(NEOM9N_itChunk));
}

/*
 * Called from HAL_UARTEx_RxEventCallback in main.c.
 * This stores incoming bytes into NEOM9N_accum.
 */
void NEOM9N_OnRxEventIT(uint16_t size) {
    if (size == 0) {
        // Restart reception anyway
        HAL_UARTEx_ReceiveToIdle_IT(uartAddress, NEOM9N_itChunk, sizeof(NEOM9N_itChunk));
        return;
    }

    // Cap copy size so we don't overflow
    uint16_t space = 0;
    if (NEOM9N_accumLen < NEOM9N_ACCUM_SIZE) {
        space = (uint16_t)(NEOM9N_ACCUM_SIZE - NEOM9N_accumLen);
    }

    uint16_t toCopy = (size <= space) ? size : space;

    if (toCopy > 0) {
        memcpy((void*)&NEOM9N_accum[NEOM9N_accumLen], NEOM9N_itChunk, toCopy);
        NEOM9N_accumLen += toCopy;
    }

    // Signal main loop that new bytes are available
    NEOM9N_rxSize = size;
    NEOM9N_rxReady = 1;

    // Restart reception immediately
    HAL_UARTEx_ReceiveToIdle_IT(uartAddress, NEOM9N_itChunk, sizeof(NEOM9N_itChunk));
}

/*
 * Non-blocking "getData" for interrupt method:
 * Copies the accumulated bytes into GPSData (null-terminated).
 * Returns HAL_OK if new data was copied, HAL_BUSY if nothing new yet.
 */
HAL_StatusTypeDef NEOM9N_getDataIT(unsigned char* GPSData, uint16_t maxLen) {
    if (!GPSData || maxLen < 2) return HAL_ERROR;
    if (NEOM9N_rxReady == 0) return HAL_BUSY;

    // Copy out as much as fits
    uint16_t copyLen = NEOM9N_accumLen;
    if (copyLen > (uint16_t)(maxLen - 1)) {
        copyLen = (uint16_t)(maxLen - 1);
    }

    memcpy(GPSData, (const void*)NEOM9N_accum, copyLen);
    GPSData[copyLen] = '\0';

    // Reset accumulation after handing it out (simple approach)
    NEOM9N_accumLen = 0;
    NEOM9N_rxReady = 0;

    return HAL_OK;
}

int NEOM9N_CheckConnection() {
    unsigned char rx_buff[256] = {0};
    int status = 0;

    /*
      For interrupt method, connection checking is usually:
      - start RX
      - wait until some bytes arrive (rxReady)
      Here, we keep the old shape but do a simple check:
    */

    NEOM9N_StartRxIT();

    // Wait briefly for any data to arrive
    uint32_t start = HAL_GetTick();
    while ((HAL_GetTick() - start) < 2000) {
        if (NEOM9N_rxReady) {
            NEOM9N_getDataIT(rx_buff, sizeof(rx_buff));
            // If we got any '$' or 'G', it's likely alive
            if (strchr((char*)rx_buff, '$') != NULL) {
                return HAL_OK;
            }
        }
    }

    return HAL_ERROR;
}

/* -------------------------------------------------------------------------
   (Polling version) - Existing function preserved
   NOTE: This blocks and is not recommended for continuous GPS streams.
   ------------------------------------------------------------------------- */
int NEOM9N_getData(unsigned char* GPSData) {
    unsigned char b = 0;
    uint32_t start = HAL_GetTick();
    uint16_t i = 0;

    if (!GPSData) return HAL_ERROR;
    memset(GPSData, 0, 10000);

    // Read until newline or timeout
    while ((HAL_GetTick() - start) < 2000) {
        if (HAL_UART_Receive(uartAddress, &b, 1, 10) == HAL_OK) {
            if (i < 9999) {
                GPSData[i++] = b;
                GPSData[i] = '\0';
            }
            if (b == '\n') {
                return HAL_OK;
            }
        }
    }

    return HAL_ERROR;
}

/* -------------------------------------------------------------------------
   Parsing functions (unchanged)
   ------------------------------------------------------------------------- */

// used Perplexity AI to generate this function:
// https://www.perplexity.ai/search/include-stdio-h-include-string-LhU2sFfoT8CujSTzgv7LbA

int NEOM9N_getPosition(float* latitude, char* latitudeHemisphere,
                       float* longitude, char* longitudeHemisphere,
                       unsigned char* GPSData, int size) {
    // Function will search through buffer until it finds GNMRC, after which it
    // will count 3 commas and record latitude, 1 comma then  N/S, 1 comma then
    // longitude, 1 comma then W/E

    char* start = strstr((const char*)GPSData, "$GNRMC,");
    if (!start) return 1;  // Not found

    // Tokenize the GNRMC sentence
    // $GNRMC,time,A,lat,NS,lon,EW,...
    char sentence[200];
    int i = 0;
    while (start[i] != '\n' && start[i] != '\r' && start[i] != '\0' && i < 199) {
        sentence[i] = start[i];
        i++;
    }
    sentence[i] = '\0';

    // Now parse the fields
    char* token;
    int field = 0;
    char latStr[20] = {0}, lonStr[20] = {0};
    token = strtok(sentence, ",");
    while (token != NULL) {
        if (field == 3) strncpy(latStr, token, sizeof(latStr) - 1);
        if (field == 4 && latitudeHemisphere) *latitudeHemisphere = token[0];
        if (field == 5) strncpy(lonStr, token, sizeof(lonStr) - 1);
        if (field == 6 && longitudeHemisphere) *longitudeHemisphere = token[0];
        token = strtok(NULL, ",");
        field++;
    }
    if (latStr[0] && lonStr[0] && field > 10) {
        float nmeaLat = atof(latStr);
        float nmeaLon = atof(lonStr);
        if (latitude)
            *latitude = convert_nmea_to_decimal(nmeaLat, *latitudeHemisphere);
        if (longitude)
            *longitude = convert_nmea_to_decimal(nmeaLon, *longitudeHemisphere);
        return 0;
    } else {
        *latitude = 0;
        *longitude = 0;
        *latitudeHemisphere = 0;
        *longitudeHemisphere = 0;
        return 2;
    }
}

// used Perplexity AI to generate this function:
// https://www.perplexity.ai/search/include-stdio-h-include-string-LhU2sFfoT8CujSTzgv7LbA

int NEOM9N_getTime(float* time, unsigned char* GPSData, int size) {
    // Function will search through buffer until it finds GNMRC, after which it
    // will start recording a float for the time and stop after it reaches a
    // comma.
    char* start = strstr((const char*)GPSData, "$GNRMC,");
    if (!start) return -1;  // Not found

    // The time is the first field after $GNRMC,
    char timeStr[20] = {0};
    int i = 0, j = 0;
    // Skip "$GNRMC,"
    while (start[i] && start[i] != ',') i++;
    if (start[i] == ',') i++;
    // Copy until next comma
    while (start[i] && start[i] != ',' && j < 19) {
        timeStr[j++] = start[i++];
    }
    timeStr[j] = '\0';
    if (time) *time = atof(timeStr);
    return 0;
}

int NEOM9N_getSpeed(float* speed, unsigned char* GPSData) {
    // Function will search through buffer until it finds GNMRC, after which it
    // will count 7 commas and parse the ground speed of the GPS
    // Returns 0 if successful, -1 if GNRMC is not found, -2 if GNRMC was found
    // but another issue occured

    char* start = strstr((const char*)GPSData, "$GNRMC,");
    if (!start) return -1;  // Not found

    // Tokenize the GNRMC sentence
    char sentence[200];
    int i = 0;
    while (start[i] != '\n' && start[i] != '\r' && start[i] != '\0' && i < 199) {
        sentence[i] = start[i];
        i++;
    }
    sentence[i] = '\0';

    // Parse speed
    char* token;
    int field = 0;
    char speedStr[20] = {0};
    token = strtok(sentence, ",");
    while (token != NULL) {
        if (field == 7) strncpy(speedStr, token, sizeof(speedStr) - 1);
        token = strtok(NULL, ",");
        field++;
    }
    // could run into issue here if called and speedStr is not 0
    if (speedStr[0]) {
        float iSpeedMyPants = atof(speedStr);
        *speed = iSpeedMyPants;
        return 0;
    }
    return -2;
}

int NEOM9N_getAltitude(float* altitude, unsigned char* GPSData) {
    // Function will search through buffer until it finds GNGGA, after which it
    // will count 7 commas and parse the ground speed of the GPS
    // Returns 0 if successful, -1 if GNRMC is not found, -2 if GNRMC was found
    // but another issue occured

    char* start = strstr((const char*)GPSData, "$GNGGA,");
    if (!start) return -1;  // Not found

    // Tokenize the GNGGA sentence
    char sentence[200];
    int i = 0;
    while (start[i] != '\n' && start[i] != '\r' && start[i] != '\0' && i < 199) {
        sentence[i] = start[i];
        i++;
    }
    sentence[i] = '\0';

    // Parse altitude
    char* token;
    int field = 0;
    char altStr[20] = {0};
    token = strtok(sentence, ",");
    while (token != NULL) {
        field++;
        if (field == 10) strncpy(altStr, token, sizeof(altStr) - 1);
        token = strtok(NULL, ",");
    }

    if (altStr[0]) {
        float altiturd = atof(altStr);
        *altitude = altiturd;
        return 0;
    }
    return -2;
}
