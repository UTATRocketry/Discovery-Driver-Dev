/*
 * constants.h
 *
 *  Created on: Apr 3, 2026
 *      Author: prith
 *
 *  Storing constants used across all files
 */

#ifndef INC_CONSTANTS_H_
#define INC_CONSTANTS_H_

#define GPS_RB_LEN 256
#define DMA_LEN 128
#define NMEA_MAX_LEN 128      // using 128 bytes as upper bound for max characters in gga nmea string (actual value is about 76 bytes)
#define GPS_PROCESS_CHUNK 64  // used in gps_process()

/* ----------------------------------------------
 * Target gps configuration —   EDIT TO MODIFY GPS
 * ---------------------------------------------- */
// these changes will effect file gps_config
/* CFG-VALSET layer bitmask (pg. 89, payload byte 1) */
#define UBX_LAYER_RAM 0x01u    // DO NOT TOUCH THESE VALUES
#define UBX_LAYER_FLASH 0x04u  // DO NOT TOUCH THESE VALUES

// VALUES THAT CAN BE CHANGED //
#define GPS_TARGET_BAUD 115200u
#define GPS_FIX_RATE_MS 200u  // measurement period -> 200 ms = 5 Hz

/* Save mode:
 *   UBX_LAYER_RAM                         = temporary until reset/power loss
 *   UBX_LAYER_RAM | UBX_LAYER_FLASH       = apply now and save permanently
 */
#define UBX_LAYERS (UBX_LAYER_RAM | UBX_LAYER_FLASH)

#endif /* INC_CONSTANTS_H_ */
