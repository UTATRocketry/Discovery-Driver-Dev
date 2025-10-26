/*
 * NEOM9N.c
 *
 *  Created on: Mar 18, 2025
 *      Author: William Gomez
 */
 #include "NEOM9N.h"

 //helper functions mostly built by AI
 
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
 
UART_HandleTypeDef* uartAddress;

extern UART_HandleTypeDef hlpuart1;

/* - DOUBLE BUFFER SYSTEM FOR INTERRUPT-BASED RECEPTION - */
#define GPS_BUFFER_SIZE 2048

static uint8_t buffer1[GPS_BUFFER_SIZE];
static uint8_t buffer2[GPS_BUFFER_SIZE];
static uint8_t* volatile write_buffer = buffer1;  // Interrupt writes here
static uint8_t* volatile read_buffer = buffer2;   // Main loop reads here
static volatile uint16_t write_size = 0;
static volatile uint8_t data_ready = 0;


 
 
/* AI-Modified: Cursor AI (Claude Sonnet 4.5) - October 2025
 * Fixed UART handle bugs, checksum calculations, and added interrupt initialization
 * Bug fixes and interrupt setup by AI */
int NEOM9N_init(UART_HandleTypeDef* uartAddressPin){
    if (uartAddressPin == NULL) {
        return HAL_ERROR;
    }
    
    uartAddress = uartAddressPin;
    int status = 0;
    
    // Initialize buffers
    memset(buffer1, 0, GPS_BUFFER_SIZE);
    memset(buffer2, 0, GPS_BUFFER_SIZE);
    data_ready = 0;
    write_size = 0;

    // Configure GPS: Set update rate to 100ms (10Hz)
    uint8_t ubx_cfg_rate[] = {
        0xB5, 0x62,       // Sync chars
        0x06, 0x08,       // Class, ID (CFG-RATE)
        0x06, 0x00,       // Length (6 bytes)
        0x64, 0x00,       // measRate = 100 ms (0x0064)
        0x01, 0x00,       // navRate = 1
        0x01, 0x00,       // timeRef = 1 (GPS time)
        0x00, 0x00        // Placeholder for checksum
    };

    // Calculate checksum (class + id + length + payload = 2+2+6 = 10 bytes)
    uint8_t ck_a, ck_b;
    calc_checksum(&ubx_cfg_rate[2], 10, &ck_a, &ck_b);
    ubx_cfg_rate[12] = ck_a;
    ubx_cfg_rate[13] = ck_b;

    // Send to GPS (not debug UART!)
    HAL_UART_Transmit(uartAddress, ubx_cfg_rate, 14, 2000);
    HAL_Delay(100); // Wait for GPS to process

    // Configure GPS: Enable NMEA GGA messages
    uint8_t ubx_cfg_msg[] = {
        0xB5, 0x62,       // UBX sync chars
        0x06, 0x01,       // Class = CFG, ID = MSG
        0x08, 0x00,       // Length = 8
        0xF0, 0x00,       // Payload: NMEA GGA (class, id)
        0x00,             // Rate for I2C
        0x01,             // Rate for UART1 (enabled)
        0x00,             // Rate for UART2
        0x00,             // Rate for USB
        0x00,             // Rate for SPI
        0x00,             // Reserved
        0x00, 0x00        // Checksum (to be filled in)
    };
    
    // Calculate checksum (class + id + length + payload = 2+2+8 = 12 bytes)
    uint8_t ck_c, ck_d;
    calc_checksum(&ubx_cfg_msg[2], 12, &ck_c, &ck_d);
    ubx_cfg_msg[14] = ck_c;
    ubx_cfg_msg[15] = ck_d;

    // Send to GPS
    HAL_UART_Transmit(uartAddress, ubx_cfg_msg, 16, 2000);
    HAL_Delay(100);

    // Start interrupt-based reception
    HAL_UARTEx_ReceiveToIdle_IT(uartAddress, write_buffer, GPS_BUFFER_SIZE);

    return HAL_OK;
}

/* OLD POLLING-BASED VERSION (REPLACED - KEPT FOR REFERENCE)
 * - Blocked main loop for seconds
 * - CPU wasted 100% while waiting

 * ============================================================================
 *
 * int NEOM9N_getData(unsigned char *GPSData){
 *     unsigned char buffer[10000] = {0};
 *     int status = 0;
 *     
 *     // BLOCKING: Wait up to 2 seconds for first byte
 *     status = HAL_UART_Receive(uartAddress, buffer, 1, 2000);
 *     if(status == 3){
 *         return HAL_ERROR;
 *     }
 *     memset(GPSData, 0, 10000);
 *     strcat(GPSData, buffer);
 * 
 *     // BLOCKING LOOP: Read bytes one-by-one with 1ms timeout each
 *     while(status != 3){
 *         status = HAL_UART_Receive(uartAddress, buffer, 1, 1);
 *         if(status != 3){
 *             strcat(GPSData, buffer);
 *         }
 *     }
 *     return HAL_OK;
 * }
 * ============================================================================ */

/* AI-Generated: Cursor AI (Claude Sonnet 4.5) - October 2025
 * Non-blocking double-buffer implementation */
// Gets GPS data from buffer (interrupt-based, non-blocking)
// Returns HAL_OK if new data available, HAL_ERROR if no new data
int NEOM9N_getData(unsigned char *GPSData){
 
    // Check if new data is ready
    if (data_ready == 0) {
        return HAL_ERROR;  // No new data yet
    }
    
    // CRITICAL SECTION: Swap buffers atomically
    __disable_irq();  // Disable interrupts briefly (~50 nanoseconds)
    
    // Swap the buffer pointers
    uint8_t* temp = read_buffer;
    read_buffer = write_buffer;
    write_buffer = temp;
    
    // Get size and clear flag
    uint16_t size = write_size;
    data_ready = 0;
    
    __enable_irq();  // Re-enable interrupts
    // END CRITICAL SECTION
    
    // Now copy data (interrupt can fire safely, writes to OTHER buffer)
    if (size > 0 && size < GPS_BUFFER_SIZE) {
        memcpy(GPSData, read_buffer, size);
        GPSData[size] = '\0';  // Null terminate
    } else {
        return HAL_ERROR;
    }
    
    return HAL_OK;
}

// Checks if new GPS data is ready (non-blocking check)
int NEOM9N_isDataReady(void){
    return data_ready;
}

/* ============================================================================
 * INTERRUPT CALLBACK FUNCTIONS
 * AI-Generated: Cursor AI (Claude Sonnet 4.5) - October 2025
 * These are called automatically by HAL when UART events occur
 * Implements interrupt-driven GPS data reception using UART IDLE detection
 * ============================================================================ */

// Called when UART receives data up to buffer size OR IDLE line detected
// This is the MAIN interrupt handler for GPS data reception
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    // Check if this interrupt is from the GPS UART
    if (huart->Instance == UART5) {
        // Data is complete in write_buffer, save size and set flag
        write_size = Size;
        data_ready = 1;
        
        // Restart reception for next GPS burst (uses SAME write_buffer)
        // Buffer swap happens in getData(), not here!
        HAL_UARTEx_ReceiveToIdle_IT(uartAddress, write_buffer, GPS_BUFFER_SIZE);
    }
}

// Fallback: Called when UART receive completes (buffer full)
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    // Check if this interrupt is from the GPS UART
    if (huart->Instance == UART5) {
        // Buffer is full, set flag
        write_size = GPS_BUFFER_SIZE;
        data_ready = 1;
        
        // Restart reception
        HAL_UARTEx_ReceiveToIdle_IT(uartAddress, write_buffer, GPS_BUFFER_SIZE);
    }
}

/* ============================================================================
 * NMEA PARSING FUNCTIONS
 * These parse specific data from GPS NMEA sentences
 * ============================================================================ */

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
   }else {
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
 
   // Parse speed
   char* token;
   int field = 0;
   char altStr[20] = {0};
   token = strtok(sentence, ",");
   while (token != NULL) {
     field++;
     if (field == 10) strncpy(altStr, token, sizeof(altStr) - 1);
     token = strtok(NULL, ",");
   }
   // could run into issue here if called and speedStr is not 0
   if (altStr[0]) {
     float altiturd = atof(altStr);
     *altitude = altiturd;
     return 0;
   }
   return -2;
 }
 
 
 