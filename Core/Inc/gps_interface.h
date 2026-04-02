/*
 * gps_interface.h
 *
 *  Created on: Mar 11, 2026
 *      Author: prith
 */

#ifndef INC_GPS_INTERFACE_H_
#define INC_GPS_INTERFACE_H_

#include <stdbool.h>
#include <stdint.h>

#include "gps_parser.h"
#include "gps_uart.h"

/* -----------------------
 * Functions
 * ----------------------- */
// initalize buffers and all other stuff needed
void gps_init(UART_HandleTypeDef* huart, RingBuffer* rb, uint8_t* rb_buf, uint8_t* dma_buf);

// start dma and uart
bool gps_start();

// callback helper
void gps_on_rx_event();

// parse data from the ring buffer
void gps_process();  // call periodically

// check if the gps has a valid fix
// (temporary implentation for debugging: and then turn on a light on the stm32)
bool gps_has_fix();

// get the most recent fix
// (temporary implentation for debugging: print stuff to console)
bool get_fix(GpsFix* copy);

// get stats for debugging purposes
void get_stats(GpsStats* copy);

#endif
