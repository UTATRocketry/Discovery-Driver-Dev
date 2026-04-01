/*
 * gps_parser.h
 *
 *  Created on: Mar 3, 2026
 *      Author: prith
 *
 *  Status: Done, needs to be tested
 *
 *  Purpose:
 *  	- turn uart bytes into full nmea sentences
        - validatae nmea checksum to ensure data hasn't been correupted
        - parse data and put into gpsFix struct(holds lat, lon, speed, alt, satallites).
          Can add more to this if need be
    Notes:
        - this file is seperated from interrupts/DMA and UART
        - only thing that needs to be changed if switching from NEMA --> UBX or vise versa

 */

#ifndef INC_GPS_PARSER_H_
#define INC_GPS_PARSER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gps_interface.h"  //contains ParserStats struct

#define NMEA_MAX_LEN 256  // using 256 bytes as upper bound for max characters in gga/rmc nmea string

/*-----------------------
 *  Structs
-----------------------*/
typedef struct {
    bool valid;               // true if current GPS fix is valid
    double lat;               // lattitude
    double lon;               // longitude
    float alt;                // altitude
    float speed_mps;          // meters per second. (converted from RMC knots)
    uint8_t satellites_used;  // from data sent by GGA
    size_t last_update_ms;    // can measure navigation rate / fix rate (hz) (default is 1Hz)
} GpsFix;

/*-----------------------
 * Function Declarations
-----------------------*/
// init/reset gps parser data
void gps_parser_init(GpsFix* fix, ParserStats* debugger);

// init/reset gps parser data
void gps_parser_init(GpsFix* fix, ParserStats* debugger);

// feed raw bytes; internally extracts full NMEA sentences and updates fix
void parse_sentence(ParserStats* debugger, GpsFix* fix, char* sentence, size_t message_timestamp);

#endif /* INC_GPS_PARSER_H_ */
