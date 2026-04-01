/*
 * gps_uart.h
 *
 *  Created on: Mar 3, 2026
 *      Author: prith
 *
 *  Status: done, needs to be tested
 *
 *  Purpose:
 *  - using DMA, take data from uart, put into ring buffer as defined in ring_buffer.h
 *  - provide a clean interface for higher layers (gps_parser)
 *
 * 	Notes:
 * 	- using UART5 RX with circular DMA
 *  - This module does NOT parse NMEA (or UBX binary)
 * 	- DMA circular buffer alone can work, but a ring buffer adds some extra saftey apperently. also convient if ever
 * 	  switching from dma to inturrupts (but why would u)
 */

#ifndef INC_GPS_UART_H_
#define INC_GPS_UART_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gps_parser.h"  //contains ParserStats struct
#include "ring_buffer.h"
#include "stm32l4xx_hal.h"  // needed for UART

#define DMA_LEN 256  // using 256 bytes as upper bound for any nmea string (CHECK THIS CALCULATION)

/* -----------------------
 * Structs
 * ----------------------- */
typedef struct {
    UART_HandleTypeDef* huart;       // UART used for GPS (&huart5 rn)
    RingBuffer* rb;                  // ring buffer as main storage (must be already initalized)
    uint8_t* dma_buffer;             // DMA circular RX buffer storage
    size_t dma_buffer_length;        // length of dmaBuf in bytes
    volatile size_t dma_last_index;  // last processed index into dmaBuf [0..dmaLen-1]
    volatile size_t rb_drop_bytes;   // check for overflow and how many bytes were lost
} GpsUartHandler;

/* -----------------------
 * Functions
 * ----------------------- */
// Initalize UART, ring buffer, DMA RX buffer
bool gps_uart_init(GpsUartHandler* gps_uart, UART_HandleTypeDef* uart_address_pin, RingBuffer* rb,
                   uint8_t* dma_buffer);

// Start UART Receive-to-IDLE with DMA into dmaBuffer (circular). (called once after init)
HAL_StatusTypeDef gps_uart_start_rx(GpsUartHandler* gps_uart);

// Put new bytes from the DMA buffer into the ring buffer. HAL_UARTEx_RxEventCallback calls this
void gps_uart_on_rx_event(GpsStats* debugger, GpsUartHandler* gps_uart);

// a wrapper around the ring buffer function rbRead(). returns number of bytes actually read
size_t gps_uart_read(GpsUartHandler* gps__uart, uint8_t* out, size_t max_length);

#endif