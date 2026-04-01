/*
 * gps.h
 *
 *  Created on: Mar 11, 2026
 *      Author: prith
 */

#ifndef INC_GPS_H_
#define INC_GPS_H_

#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    double latDeg;
    double lonDeg;
    float speedMps;
    float altMeters;
    uint8_t satellitesUsed;
    bool valid;
    uint32_t lastUpdateMs;
} GpsFix;

void gpsInit(void);
void gpsProcess(void);  // call periodically
bool gpsGetFix(GpsFix* out);
bool gpsHasFix(void);

#endif
