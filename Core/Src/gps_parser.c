/*
 * gps_parser.c
 *
 *  Created on: Mar 3, 2026
 *      Author: prith
 *
 *  Done, needs to be tested
 */
#include "gps_parser.h"

#include <stdlib.h>
#include <string.h>  // for memset

/* -----------------------
 *  Structs
 * ----------------------- */
typedef struct {
    char sentence[NMEA_MAX_LEN];  // holds one NMEA sentence
    uint16_t length;
    bool in_sentence;
} GpsNmeaAssembler;

static GpsNmeaAssembler assembler;

/* -----------------------
 *  Internal Helper Functions
 * ----------------------- */
// NMEA sends lat/lon in degrees + minutes (ddmm.mmmm). This converts to decimalDegrees
static double degree_minutes_to_decimal_degrees(double degree_minutes, char hemisphere) {
    int degrees = (int)(degree_minutes / 100.0);
    double minutes = degree_minutes - ((double)degrees * 100.0);
    double decimal = degrees + (minutes / 60.0);

    if (hemisphere == 'S' || hemisphere == 'W')
        decimal = -decimal;

    return decimal;
}

// turn ASCII hexadecimal character into its integer equivalent (0 to 15)
static uint8_t hex_to_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');  // subtract A(base16) add 10(base10)
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');  // incase ascii character is in lowercase
    return 0xFF;                                      // if error, return char larger than a nibble
}

// Validate NMEA checksum
static bool nmea_checksum_valid(const char* sentence) {
    // taken from: https://forum.arduino.cc/t/nmea-checksums-explained/1046083
    if (!sentence || sentence[0] != '$')
        return false;

    const char* star = strchr(sentence, '*');
    if (!star || !star[1] || !star[2])
        return false;

    uint8_t calc = 0;
    for (const char* p = sentence + 1; p < star; p++) {
        calc ^= (uint8_t)(*p);
    }

    uint8_t high = hex_to_nibble(star[1]);
    uint8_t low = hex_to_nibble(star[2]);
    if (high == 0xFF || low == 0xFF)
        return false;

    uint8_t expected = (high << 4) | low;
    return (calc == expected);
}

// parse rmc nmea string
static void parse_rmc(GpsStats* debugger, GpsFix* fix, char* rmc_string, size_t message_timestamp) {
    int field = 0;                          // represents each piece of the rmc string
    char* token = strtok(rmc_string, ",");  // create a pointer to first item in the rmc string

    // local buffers to hold info
    char lat_str[20] = {0};
    char lon_str[20] = {0};
    char speed_str[20] = {0};
    char lat_hem = 0;
    char lon_hem = 0;
    char status = 'V';  //  (A == data valid) (V == data not valid)

    while (token) {
        if (field == 2) status = token[0];  // if field == 2, at the 3rd part of the rmc string, this represents status
        if (field == 3) strncpy(lat_str, token, sizeof(lat_str) - 1);
        if (field == 4) lat_hem = token[0];
        if (field == 5) strncpy(lon_str, token, sizeof(lon_str) - 1);
        if (field == 6) lon_hem = token[0];
        if (field == 7) strncpy(speed_str, token, sizeof(speed_str) - 1);

        token = strtok(NULL, ",");  // move to the next token in sentence
        field++;
    }

    if (status != 'A') {  // 'A' indicates a valid fix
        fix->valid = false;
        return;
    }

    // if lat and lon are non empty, start storing stuff
    if (lat_str[0] && lon_str[0]) {
        double lat_nmea = atof(lat_str);  // convert string from nmea to float
        double lon_nmea = atof(lon_str);

        fix->lat = degree_minutes_to_decimal_degrees(lat_nmea, lat_hem);
        fix->lon = degree_minutes_to_decimal_degrees(lon_nmea, lon_hem);

        float speed_knots = (float)atof(speed_str);
        fix->speed_mps = speed_knots * 0.514444f;  // cause 1 knot = 0.514444 m/s

        fix->valid = true;
        fix->last_update_ms = message_timestamp;
        debugger->parsed_rmc_count++;  // successfully parsed
    }
}

// parse gga nmea string
static void parse_gga(GpsStats* debugger, GpsFix* fix, char* gga_string, size_t message_timestamp) {
    // exact same logic as parseRMC function but now with parameters applying to gga strings instead
    int field = 0;
    char* token = strtok(gga_string, ",");
    bool parsed_anything = false;  // just to check for a successful parse

    char sats_str[8] = {0};
    char alt_str[20] = {0};

    while (token) {
        if (field == 7) strncpy(sats_str, token, sizeof(sats_str) - 1);
        if (field == 9) strncpy(alt_str, token, sizeof(alt_str) - 1);

        token = strtok(NULL, ",");
        field++;
    }

    if (sats_str[0]) {
        int sats = atoi(sats_str);
        if (sats < 0) sats = 0;
        if (sats > 255) sats = 255;
        fix->satellites_used = (uint8_t)sats;
        parsed_anything = true;
    }

    if (alt_str[0]) {
        fix->alt = (float)atof(alt_str);
        parsed_anything = true;

        if (fix->valid) {
            fix->last_update_ms = message_timestamp;
        }
    }

    if (parsed_anything)
        debugger->parsed_gga_count++;
}

// figure out if rmc or gga and parse apporpriatly
static void parse_sentence(GpsStats* debugger, GpsFix* fix, char* sentence, size_t message_timestamp) {
    if (strstr(sentence, "RMC") == sentence + 3)
        parse_rmc(debugger, fix, sentence, message_timestamp);
    else if (strstr(sentence, "GGA") == sentence + 3)
        parse_gga(debugger, fix, sentence, message_timestamp);
    else
        debugger->ignored_sentences++;
}

/* -----------------------
 *  Visible functions
 * ----------------------- */
// init/reset
void gps_parser_init(GpsFix* fix, GpsStats* debugger) {
    if (!fix || !debugger)
        return;

    memset(&assembler, 0, sizeof(assembler));
    memset(fix, 0, sizeof(*fix));
    memset(debugger, 0, sizeof(*debugger));

    fix->valid = false;
}

// takes raw incoming UART bytes and tries to turn them into complete NMEA sentences, then updates fix
void gps_parser_feed(GpsStats* debugger, GpsFix* fix, const uint8_t* data, size_t length, size_t message_timestamp) {
    if (!data || length == 0)  // check if data was even given
        return;

    for (size_t i = 0; i < length; i++) {
        char c = (char)data[i];

        if (c == '$') {
            assembler.in_sentence = true;
            assembler.length = 0;
        }

        if (!assembler.in_sentence)  // skip those characters till be get to stuff we need in the nmea string
            continue;

        if (assembler.length < (NMEA_MAX_LEN - 1)) {
            assembler.sentence[assembler.length++] = c;
            assembler.sentence[assembler.length] = '\0';
        } else {  // TO PREVENT OVERFLOW: Abandon current string, wait for the next '$'
            assembler.in_sentence = false;
            assembler.length = 0;
            debugger->line_overflow_drops++;
            continue;
        }

        if (c == '\n') {                 // \r\n is the escape sequence for nmea strings
            debugger->sentences_seen++;  // new sentence seen!

            if (assembler.length > debugger->max_sentence_length_seen)  // update the longest sentence seen
                debugger->max_sentence_length_seen = assembler.length;

            assembler.in_sentence = false;  // end check

            if (nmea_checksum_valid(assembler.sentence)) {  // do the checksum and only proceed if validated
                debugger->valid_sentences++;
                parse_sentence(debugger, fix, assembler.sentence, message_timestamp);  // update the fix struct
            } else
                debugger->checksum_failure++;  // checksum failed, increment
            assembler.length = 0;              // reset assembler
        }
    }
}
