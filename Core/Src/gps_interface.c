/*
 * gps_interface.c
 *
 *  Created on: Mar 11, 2026
 *      Author: prith
 */

#include "gps_interface.h"
#include "constants.h"

/* -----------------------
 *  Internal module state
 * ----------------------- */
static GpsUartHandler g_uart;
static GpsFix g_fix;
static GpsStats g_stats;
static bool g_initialized = false;

/* -----------------------
 * Functions
 * ----------------------- */
// initalize buffers and all other stuff needed. Performs total initalization
void gps_init(UART_HandleTypeDef *huart, RingBuffer *rb, uint8_t *rb_buf,
		uint8_t *dma_buf) {
	if (!huart || !rb || !rb_buf || !dma_buf) {
		g_initialized = false;
		return;
	}

	rb_init(rb, rb_buf, GPS_RB_LEN);

	gps_parser_init(&g_fix, &g_stats);

	if (!gps_uart_init(&g_uart, huart, rb, dma_buf, DMA_LEN)) {
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

// callback for uart. Just takes wtv is currently in the DMA and puts it into the main storage (i.e. main ring buffer)
void gps_on_rx_event(uint16_t new_pos) {
	if (!g_initialized)
		return;

	gps_uart_on_rx_event(&g_stats, &g_uart, new_pos);
}

// parse data from the large ring buffer
void gps_process(void) {
	if (!g_initialized)
		return;

	uint8_t temp_buf[GPS_PROCESS_CHUNK];  // transport this much data at once
	size_t bytes_read = 0;

	// read until buffer empty
	while (1) {
		bytes_read = gps_uart_read(&g_uart, temp_buf, sizeof(temp_buf));
		if (bytes_read == 0)
			break;

		gps_parser_feed(&g_stats, &g_fix, temp_buf, bytes_read, HAL_GetTick());
	}
}

// check if the gps has a valid fix
bool gps_has_fix(void) {
	if (!g_initialized)
		return false;

	return g_fix.valid;
}

// getter for latest fix
bool get_fix(GpsFix *out) {
	if (!g_initialized || !out)
		return false;

	*out = g_fix;
	return g_fix.valid;
}

// getter for stats
void get_stats(GpsStats *stats) {
	if (!g_initialized || !stats)
		return;

	*stats = g_stats;
}

size_t get_dropped_bytes() {
	if (!g_initialized)
		return 0;

	return gps_uart_get_and_clear_dropped_bytes(&g_uart);
}

size_t get_dma_overrun_count() {
	return g_uart.dma_overrun_count;
}
