/*
 * gps.c
 *
 *  Created on: Mar 11, 2026
 *      Author: prith
 */

#include "gps_interface.h"

/* -----------------------
 *  Internal module state
 * ----------------------- */
static GpsUartHandler g_uart;
static GpsFix g_fix;
static GpsStats g_stats;
static bool g_initialized = false;

/* -----------------------
 * Helper functions
 * ----------------------- */

/* -----------------------
 *  Visible Functions
 * ----------------------- */
// initalize buffers and all other stuff needed
void gps_init(UART_HandleTypeDef* huart, RingBuffer* rb, uint8_t* dma_buf) {
    if (!huart || !rb || !dma_buf) {
        g_initialized = false;
        return;
    }

    gps_parser_init(&g_fix, &g_stats);

    if (!gps_uart_init(&g_uart, huart, rb, dma_buf)) {
        g_initialized = false;
        return;
    }

    g_initialized = true;
}

// start UART DMA reception
bool gps_start(void) {
    if (!g_initialized)
        return false;

    return (gps_uart_start_rx(&g_uart) == HAL_OK);
}

// callback
void gps_on_rx_event() {
    if (!g_initialized)
        return;

    gps_uart_on_rx_event(&g_stats, &g_uart);
}

// parse data from the large ring buffer
void gps_process(void) {
    if (!g_initialized)
        return;

    uint8_t temp_buf[64];
    size_t bytes_read = gps_uart_read(&g_uart, temp_buf, sizeof(temp_buf));

    if (bytes_read == 0)
        return;

    gps_parser_feed(&g_stats, &g_fix, temp_buf, bytes_read, HAL_GetTick());

    // keep mirrored overflow info in stats
    g_stats.ring_buffer_overflows = g_uart.rb_drop_bytes;
}

// check if the gps has a valid fix
bool gps_has_fix(void) {
    if (!g_initialized)
        return false;

    return g_fix.valid;
}

// getter for latest fix
bool gps_get_fix(GpsFix* out) {
    if (!g_initialized || !out)
        return false;

    *out = g_fix;
    return g_fix.valid;
}

// getter for stats
void gps_get_stats(GpsStats* stats) {
    if (!g_initialized || !stats)
        return;

    *stats = g_stats;
}
