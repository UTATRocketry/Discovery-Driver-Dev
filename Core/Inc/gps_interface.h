/*
 * gps_interface.h
 *
 *  Created on: Mar 11, 2026
 *      Author: prith
 */

#ifndef INC_GPS_INTERFACE_H_
#define INC_GPS_INTERFACE_H_

#pragma once
#include <stdbool.h>
#include <stdint.h>

/* -----------------------
 * Structs
 * ----------------------- */
typedef struct {
    double latDeg;
    double lonDeg;
    float speedMps;
    float altMeters;
    uint8_t satellitesUsed;
    bool valid;
    size_t lastUpdateMs;
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
} ParserStats;  // for the sake of debugging, can be removed later if needed

/* -----------------------
 * Functions
 * ----------------------- */
void gpsInit(void);
void gpsProcess(void);  // call periodically
bool gpsGetFix(GpsFix* out);
bool gpsHasFix(void);

#endif
