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

#define GPS_RB_LEN 2048
#define DMA_LEN 256
#define NMEA_MAX_LEN 256  // using 256 bytes as upper bound for max characters in gga/rmc nmea string
#define GPS_PROCESS_CHUNK 64 // used in gps_procesS()

#endif /* INC_CONSTANTS_H_ */
