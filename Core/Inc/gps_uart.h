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

#include "constants.h"
#include "gps_parser.h"  //contains ParserStats struct
#include "ring_buffer.h"
#include "stm32l4xx_hal.h"  // needed for UART

/* -----------------------
 * Structs
 * ----------------------- */
typedef struct {
    UART_HandleTypeDef* huart;       // UART used for GPS (&huart5 rn)
    RingBuffer* rb;                  // ring buffer as main storage (must be already initalized)
    uint8_t* dma_buffer;             // DMA circular RX buffer storage
    size_t dma_buffer_length;        // length of dmaBuf in bytes
    volatile size_t dma_last_index;  // last processed index into dmaBuf [0..dmaLen-1]
    size_t dma_overrun_count;        // tracks how many times new_pos == old_pos in the DMA buffer
    volatile size_t rb_drop_bytes;   // check for overflow and how many bytes were lost within dma buffer or main ring buffer
} GpsUartHandler;

/* -----------------------
 * Functions
 * ----------------------- */
// Initalize UART, ring buffer, DMA RX buffer
bool gps_uart_init(GpsUartHandler* gps_uart,
                   UART_HandleTypeDef* uart_address_pin, RingBuffer* rb,
                   uint8_t* dma_buffer, size_t dma_len);

// Start UART Receive-to-IDLE with DMA into dmaBuffer (circular). (called once after init)
HAL_StatusTypeDef gps_uart_start_rx(GpsUartHandler* gps_uart);

// Put new bytes from the DMA buffer into the ring buffer. HAL_UARTEx_RxEventCallback calls this
void gps_uart_on_rx_event(GpsStats* debugger, GpsUartHandler* gps_uart,
                          uint16_t new_pos);

// a wrapper around the ring buffer function rbRead(). returns number of bytes actually read
size_t gps_uart_read(GpsUartHandler* gps__uart, uint8_t* out, size_t max_length);

// get the lost bytes, then clear the counter (so 32 bit counter won't eventually overflow if gps runs for a long time)
size_t gps_uart_get_and_clear_dropped_bytes(GpsUartHandler* gps_uart);

#endif
