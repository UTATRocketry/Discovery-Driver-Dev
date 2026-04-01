/*
 * gps_uart.c
 *
 *  Created on: Mar 4, 2026
 *      Author: prith
 *
 *  Mostly ai generated code
 */

#include "gps_uart.h"

#include <string.h>

/* -----------------------
 *  Internal Helper Functions
 * ----------------------- */
// return ring buffer overflow
// static inline size_t getBufferOverflow(const GpsUartHandler* gpsUart) {
//     return (gpsUart && gpsUart->rb) ? gpsUart->rb->overflowCount : 0;
// }

// calculate distance between two indices in a circular buffer
static size_t mod_distance(size_t a, size_t b, size_t m) {
    if (a >= b) return a - b;
    return (m - b) + a;
}

// Harvest bytes from dmaBuffer[oldPos..newPos) into ring buffer, with wrap handling
static void data_into_ring(ParserStats* debugger, GpsUartHandler* gps_uart, size_t new_pos) {
    // All AI generated, not modified
    if (!gps_uart || !gps_uart->rb || !gps_uart->dma_buffer || gps_uart->dma_buffer_length == 0)
        return;

    size_t len = gps_uart->dma_buffer_length;
    size_t old_pos = gps_uart->dma_last_index;

    // Clamp defensively
    if (new_pos >= len) new_pos %= len;
    if (old_pos >= len) old_pos %= len;

    // No new data
    if (new_pos == old_pos)
        return;

    size_t attempted;
    size_t written;

    if (new_pos > old_pos) {
        // Contiguous region [oldPos, newPos)
        attempted = new_pos - old_pos;
        written = rb_write(gps_uart->rb, &gps_uart->dma_buffer[old_pos], attempted);

        if (written < attempted) {
            gps_uart->rb_drop_bytes += (attempted - written);
        }
    } else {
        // Wrapped: [oldPos, len)
        attempted = len - old_pos;
        written = rb_write(gps_uart->rb, &gps_uart->dma_buffer[old_pos], attempted);

        if (written < attempted) {
            gps_uart->rb_drop_bytes += (attempted - written);
        }

        // Then [0, newPos)
        if (new_pos > 0) {
            attempted = new_pos;
            written = rb_write(gps_uart->rb, &gps_uart->dma_buffer[0], attempted);

            if (written < attempted) {
                gps_uart->rb_drop_bytes += (attempted - written);
            }
        }
    }

    gps_uart->dma_last_index = new_pos;

    debugger->total_bytes_received += attempted;  // increment
}

// Determine current DMA write position in circular buffer using DMA counter
static size_t get_dma_write_position(const GpsUartHandler* gps_uart) {
    // basic checks
    if (!gps_uart || !gps_uart->huart || !gps_uart->huart->hdmarx)
        return 0;

    size_t len = gps_uart->dma_buffer_length;

    // get the number of remaining data units in the current DMA Channel transfer (remianing)
    // subtract remaining from length to get current position
    // NDTR (Number of Data Register) starts with len and decrements as data transfers
    // so, NDTR <= len always. Wraps around from 0 to len in circular mode
    // use Len - NDTR  to calc current write position in DMA buffer
    size_t remaining = __HAL_DMA_GET_COUNTER(gps_uart->huart->hdmarx);
    size_t pos = len - (size_t)remaining;

    if (pos == len)
        pos = 0;  // cause circular buffer, wrap around

    return pos;
}

/* -----------------------
 *  Core functions
 * ----------------------- */
// initalize gpsUart struct (must do before everything else)
bool gps_uart_init(GpsUartHandler* gps_uart, UART_HandleTypeDef* uart_address_pin, RingBuffer* rb,
                   uint8_t* dma_buffer, size_t dma_buffer_length) {
    // basic defensive checks
    if (!gps_uart || !uart_address_pin || !rb || !dma_buffer) return false;
    if (dma_buffer_length < 2) return false;

    gps_uart->huart = uart_address_pin;

    // ensure that the ring buffer has been initlalized before putting it into the gpsUart
    if (rb->buffer_init)
        gps_uart->rb = rb;
    else
        return false;

    gps_uart->dma_buffer = dma_buffer;
    gps_uart->dma_buffer_length = dma_buffer_length;
    gps_uart->dma_last_index = 0;
    gps_uart->rb_drop_bytes = 0;

    return true;
}

// run once to start dma data transfer
HAL_StatusTypeDef gps_uart_start_rx(GpsUartHandler* gps_uart) {
    // basic chekcs (check if initalized)
    if (!gps_uart || !gps_uart->huart || !gps_uart->dma_buffer || gps_uart->dma_buffer_length == 0)
        return HAL_ERROR;

    // clear
    gps_uart->dma_last_index = 0;  // no need for smth like memset, dma will overwrite the garbage data anyway
    rb_reset(gps_uart->rb);

    // Start Receive-to-IDLE with DMA into the circular buffer
    HAL_StatusTypeDef st = HAL_UARTEx_ReceiveToIdle_DMA(gps_uart->huart, gps_uart->dma_buffer,
                                                        gps_uart->dma_buffer_length);

    if (st != HAL_OK)
        return st;  // return wtv error code

    // // Disable half-transfer interrupts to reduce IRQ load.
    // // IDLE events will still happen and RxEventCallback will still get called to take stuff from dmaBuffer and put into
    // // custom ring buffer.
    // // however, we are not disabling the transfer complete inturrupt from the DMA
    // if (gps_uart->huart->hdmarx != NULL)
    //     __HAL_DMA_DISABLE_IT(gps_uart->huart->hdmarx, DMA_IT_HT);

    return HAL_OK;
}

// if data recieved, take from dma buffer, put into the ring buffer
void gps_uart_on_rx_event(ParserStats* debugger, GpsUartHandler* gps_uart) {
    if (!gps_uart || !gps_uart->huart)
        return;

    size_t new_pos = get_dma_write_position(gps_uart);  // figure out where the DMA is currently wirting
    data_into_ring(debugger, gps_uart, new_pos);        // read from dma buffer into the ring buffer.
}

// a wrapper around the ring buffer function rbRead(). returns max_length bytse read
size_t gps_uart_read(GpsUartHandler* gps_uart, uint8_t* out, size_t max_length) {
    if (!gps_uart || !gps_uart->rb || !out || max_length == 0)
        return 0;

    return rb_read(gps_uart->rb, out, max_length);
}