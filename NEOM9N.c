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
//extern UART_HandleTypeDef huart5;

/* - DEBUG BUFFER - */
unsigned char rx_buffDEBUG[1000]; //read buffer from NEO-M9N
unsigned char tx_buffDEBUG[1000]; //write buffer to serial output


int NEOM9N_init(UART_HandleTypeDef* uartAddressPin){
	uartAddress = uartAddressPin;
	int status = 0;


	status = NEOM9N_CheckConnection();
	if (status == HAL_ERROR) {
		return status; // if GPS does not respond, return 1
	}
	//todo: continue to configure settings for GPS once connected/wait for cold start procedure to finish

	//0x209100bb
	//0xb5620608060064000100010000
	//0x30210001

	uint8_t ubx_msg[] = {
	        0xB5, 0x62,       // Sync chars
	        0x06, 0x08,       // Class, ID (CFG-RATE)
	        0x06, 0x00,       // Length (6 bytes)
	        0x64, 0x00,       // measRate = 100 ms (0x0064)
	        0x01, 0x00,       // navRate = 1
	        0x01, 0x00,       // timeRef = 1 (GPS time)
	        0x00, 0x00        // Placeholder for checksum
	    };

	    // Calculate checksum over bytes 2..11 (class to last payload byte)
	    uint8_t ck_a, ck_b;
	    calc_checksum(&ubx_msg[2], 10, &ck_a, &ck_b);
	    ubx_msg[12] = ck_a;
	    ubx_msg[13] = ck_b;

	    HAL_UART_Transmit(&hlpuart1, ubx_msg, strlen((uint8_t*)ubx_msg), 2000);


	    uint8_t ubx_cfg_msg[] = {
	        0xB5, 0x62,       // UBX sync chars
	        0x06, 0x01,       // Class = CFG, ID = MSG
	        0x08, 0x00,       // Length = 8
	        0xF0, 0x00,       // Payload: NMEA GGA (class, id)
	        0x00,             // Rate for I2C (set to 0 if not used)
	        0x02,             // Rate for UART1 (1 = enabled)
	        0x00,             // Rate for UART2 (not present on M9N)
	        0x00,             // Rate for USB
	        0x00,             // Rate for SPI
	        0x00,             // Reserved
	        0x00, 0x00        // Checksum (to be filled in)
	    };
	    uint8_t ck_c, ck_d;
	    	    calc_checksum(&ubx_cfg_msg[2], 10, &ck_c, &ck_d);
	    	    ubx_msg[12] = ck_c;
	    	    ubx_msg[13] = ck_d;

	    	    HAL_UART_Transmit(&hlpuart1, ubx_cfg_msg, strlen((uint8_t*)ubx_cfg_msg), 2000);

	    return 0;
}

int NEOM9N_CheckConnection(){
	unsigned char rx_buff[10000] = {0};
	int status = 0;
	status = NEOM9N_getData(rx_buff);

	//debug TO BE REMOVED LATER
	//HAL_UART_Transmit(&hlpuart1, rx_buff, strlen((char*)rx_buff), 2000);
	return status;

}

//reads from readable buffer, checks if interrupt happened while it was reading (if so, swap pointers)
int NEOM9N_getData(unsigned char *GPSData){

	unsigned char buffer[10000] = {0};
	int status = 0;
	status = HAL_UART_Receive(uartAddress, buffer, 1, 2000);
	if(status == 3){
		return HAL_ERROR;
	}
	memset(GPSData, 0, 10000);

	strcat(GPSData, buffer);

	while(status != 3){
	status = HAL_UART_Receive(uartAddress, buffer, 1, 1);
	if(status != 3){
		strcat(GPSData, buffer);
		}
	}

	return HAL_OK;
}




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






