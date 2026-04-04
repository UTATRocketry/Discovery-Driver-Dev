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
	size_t last_update_ms; // can measure navigation rate / fix rate (hz) (default is 1Hz)
} GpsFix;

typedef struct {
	size_t total_bytes_received;
	size_t sentences_seen;
	size_t checksum_failure;
	size_t valid_sentences;
	size_t line_overflow_drops;    // sentence too long / buffer overflow
	size_t ring_buffer_overflows;  // FIFO full, data lost
	size_t parsed_gga_count;
	size_t parsed_rmc_count;
	size_t ignored_sentences;
	size_t max_sentence_length_seen;
} GpsStats;  // for the sake of debugging, can be removed later if needed

/*-----------------------
 * Function Declarations
 -----------------------*/
// init/reset gps parser data
void gps_parser_init(GpsFix *fix, GpsStats *debugger);

// parse data
void gps_parser_feed(GpsStats *debugger, GpsFix *fix, const uint8_t *data,
		size_t length, size_t message_timestamp);

#endif /* INC_GPS_PARSER_H_ */
