/*
 * gps.c
 *
 *  Created on: Mar 11, 2026
 *      Author: prith
 */

#include "gps_interface.h"

#include "gps_parser.h"
#include "gps_uart.h"

static GpsFix fix;

void gpsInit(void) {
    gpsParserInit(&fix);
}

void gpsProcess(void) {
    uint8_t buf[128];

    size_t len = gpsUartRead(buf, sizeof(buf));

    if (len > 0) {
        gpsParserFeed(&fix, buf, len, HAL_GetTick());
    }
}

bool gpsGetFix(GpsFix* out) {
    if (!out) return false;
    *out = fix;
    return fix.valid;
}

bool gpsHasFix(void) {
    return fix.valid;
}
