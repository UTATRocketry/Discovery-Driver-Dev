/*
 * gps_parser.c
 *
 *  Created on: Mar 3, 2026
 *      Author: prith
 *
 * Parses NMEA sentences from the GPS receiver.
 *
 * Only GGA is parsed. All other sentences are disabled on the GPS module
 * via UBX CFG-VALSET in gps_config.c, so they should never appear here.
 * If they do, they are counted in ignored_sentences inside GpsStats.
 *
 * GGA gives us everything we need:
 *   field 2-3:  latitude and hemisphere (N/S)
 *   field 4-5:  longitude and hemisphere (E/W)
 *   field 6:    fix quality (0 = no fix, 1 = GPS, 2 = DGPS)
 *   field 7:    number of satellites in use
 *   field 9:    altitude in metres above sea level
 */

#include "gps_parser.h"

#include <stdlib.h>
#include <string.h>

#include "constants.h"

/* -----------------------
 * Assembler state: builds one complete NMEA sentence from the raw byte stream.
 * Sentences start with '$' and end with '\n'. Bytes outside a sentence are
 * discarded. If a sentence exceeds NMEA_MAX_LEN, it is abandoned and the
 * assembler waits for the next '$'.
 * ----------------------- */
typedef struct {
    char sentence[NMEA_MAX_LEN];
    uint16_t length;
    bool in_sentence;
} GpsNmeaAssembler;

static GpsNmeaAssembler assembler;

/* -----------------------
 * Internal Helpers
 * ----------------------- */

/* NMEA encodes lat/lon as ddmm.mmmm (degrees and decimal minutes).
 * This converts to decimal degrees, which is the standard format. */
static double degree_minutes_to_decimal(double degree_minutes, char hemisphere) {
    int degrees = (int)(degree_minutes / 100.0);
    double minutes = degree_minutes - ((double)degrees * 100.0);
    double decimal = degrees + (minutes / 60.0);
    if (hemisphere == 'S' || hemisphere == 'W')
        decimal = -decimal;
    return decimal;
}

/* Converts one ASCII hex character to its integer value (0-15).
 * Returns 0xFF if the character is not valid hex. */
static uint8_t hex_to_nibble(char c) {
    if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
    if (c >= 'A' && c <= 'F') return (uint8_t)(10 + (c - 'A'));
    if (c >= 'a' && c <= 'f') return (uint8_t)(10 + (c - 'a'));
    return 0xFF;
}

/* Validates the NMEA XOR checksum. Every NMEA sentence ends with *XX where
 * XX is the XOR of all bytes between '$' and '*'. Returns false if the
 * sentence is malformed or the checksum does not match. */
static bool nmea_checksum_valid(const char* sentence) {
    if (!sentence || sentence[0] != '$')
        return false;

    const char* star = strchr(sentence, '*');
    if (!star || !star[1] || !star[2])
        return false;

    uint8_t calc = 0;
    for (const char* p = sentence + 1; p < star; p++)
        calc ^= (uint8_t)(*p);

    uint8_t high = hex_to_nibble(star[1]);
    uint8_t low = hex_to_nibble(star[2]);
    if (high == 0xFF || low == 0xFF)
        return false;

    return (calc == (uint8_t)((high << 4) | low));
}

/* -----------------------
 * GGA Parser
 *
 * Example sentence:
 *   $GNGGA,123519.00,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
 *
 * Field index (0-based after the sentence ID):
 *   0:  UTC time (hhmmss.ss) -- not used
 *   1:  not used (field count starts after the sentence ID token)
 *
 * Actual field layout when tokenized by comma:
 *   token 0:  sentence ID ($GNGGA)
 *   token 1:  UTC time
 *   token 2:  latitude (ddmm.mmmm)
 *   token 3:  N/S hemisphere
 *   token 4:  longitude (dddmm.mmmm)
 *   token 5:  E/W hemisphere
 *   token 6:  fix quality (0=invalid, 1=GPS, 2=DGPS, 4=RTK fixed, 5=RTK float)
 *   token 7:  satellites in use
 *   token 8:  HDOP -- not used
 *   token 9:  altitude (metres above sea level)
 * ----------------------- */
static void parse_gga(GpsStats* debugger, GpsFix* fix, char* gga_string,
                      size_t message_timestamp) {
    int field = 0;
    char* token = strtok(gga_string, ",");

    char lat_str[20] = {0};
    char lon_str[20] = {0};
    char alt_str[20] = {0};
    char sats_str[8] = {0};
    char fix_quality = '0';
    char lat_hem = '?';
    char lon_hem = '?';

    while (token) {
        switch (field) {
            case 2:
                strncpy(lat_str, token, sizeof(lat_str) - 1);
                break;
            case 3:
                lat_hem = token[0];
                break;
            case 4:
                strncpy(lon_str, token, sizeof(lon_str) - 1);
                break;
            case 5:
                lon_hem = token[0];
                break;
            case 6:
                fix_quality = token[0];
                break;
            case 7:
                strncpy(sats_str, token, sizeof(sats_str) - 1);
                break;
            case 9:
                strncpy(alt_str, token, sizeof(alt_str) - 1);
                break;
            default:
                break;
        }
        token = strtok(NULL, ",");
        field++;
    }

    /* fix quality 0 means the receiver has no valid position solution */
    if (fix_quality == '0' || fix_quality == '\0') {
        fix->valid = false;
        return;
    }

    if (lat_str[0] && lon_str[0]) {
        fix->lat = degree_minutes_to_decimal(atof(lat_str), lat_hem);
        fix->lon = degree_minutes_to_decimal(atof(lon_str), lon_hem);
        fix->valid = true;
    }

    if (sats_str[0]) {
        int sats = atoi(sats_str);
        if (sats < 0) sats = 0;
        if (sats > 255) sats = 255;
        fix->satellites_used = (uint8_t)sats;
    }

    if (alt_str[0])
        fix->alt = (float)atof(alt_str);

    fix->last_update_ms = message_timestamp;
    debugger->parsed_gga_count++;
}

/* -----------------------
 * Sentence dispatcher
 *
 * Only GGA is expected here since all other sentences are disabled on the
 * GPS module. Anything else increments ignored_sentences, which shows up
 * in the stats print in main and tells you the GPS config did not apply.
 * ----------------------- */
static void parse_sentence(GpsStats* debugger, GpsFix* fix, char* sentence,
                           size_t message_timestamp) {
    if (strncmp(sentence + 3, "GGA", 3) == 0)
        parse_gga(debugger, fix, sentence, message_timestamp);
    else
        debugger->ignored_sentences++;
}

/* -----------------------
 * Public API
 * ----------------------- */

void gps_parser_init(GpsFix* fix, GpsStats* debugger) {
    if (!fix || !debugger)
        return;

    memset(&assembler, 0, sizeof(assembler));
    memset(fix, 0, sizeof(*fix));
    memset(debugger, 0, sizeof(*debugger));

    fix->valid = false;
}

/* gps_parser_feed -- called from gps_process() with chunks read from the ring
 * buffer. Assembles raw bytes into complete NMEA sentences one character at a
 * time, validates the checksum, then dispatches to parse_gga.
 *
 * message_timestamp should be HAL_GetTick() at the time of the call. It is
 * stored in fix->last_update_ms so the caller can measure fix age. */
void gps_parser_feed(GpsStats* debugger, GpsFix* fix, const uint8_t* data,
                     size_t length, size_t message_timestamp) {
    if (!debugger || !fix || !data || length == 0)
        return;

    for (size_t i = 0; i < length; i++) {
        char c = (char)data[i];

        if (c == '$') {
            assembler.in_sentence = true;
            assembler.length = 0;
        }

        if (!assembler.in_sentence)
            continue;

        if (assembler.length < (NMEA_MAX_LEN - 1)) {
            assembler.sentence[assembler.length++] = c;
            assembler.sentence[assembler.length] = '\0';
        } else {
            /* sentence exceeded buffer -- abandon it and wait for the next '$' */
            assembler.in_sentence = false;
            assembler.length = 0;
            debugger->line_overflow_drops++;
            continue;
        }

        if (c == '\n') {
            debugger->sentences_seen++;

            if (assembler.length > debugger->max_sentence_length_seen)
                debugger->max_sentence_length_seen = assembler.length;

            assembler.in_sentence = false;

            if (nmea_checksum_valid(assembler.sentence)) {
                debugger->valid_sentences++;
                parse_sentence(debugger, fix, assembler.sentence, message_timestamp);
            } else {
                debugger->checksum_failure++;
            }

            assembler.length = 0;
        }
    }
}