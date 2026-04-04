/*
 * gps_uart.c
 *
 *  Created on: Mar 4, 2026
 *      Author: prith
 *
 */

#include "gps_uart.h"

/* -----------------------
 *  Internal Helper Functions
 * ----------------------- */
// Take bytes from the DMA buffer and put into main storage, the ring buffer. Has wrap handling
static void data_into_ring(GpsStats* debugger, GpsUartHandler* gps_uart,
                           size_t new_pos) {
    // AI written
    if (!gps_uart || !gps_uart->rb || !gps_uart->dma_buffer || !debugger || gps_uart->dma_buffer_length == 0) {
        return;
    }

    size_t len = gps_uart->dma_buffer_length;
    size_t old_pos = gps_uart->dma_last_index;

    // Clamp defensively
    if (new_pos >= len) {
        new_pos %= len;
    }
    if (old_pos >= len) {
        old_pos %= len;
    }

    size_t attempted = 0;
    size_t written = 0;
    size_t total_written = 0;
    size_t total_attempted = 0;

    // Ambiguous case in circular DMA:
    // callback fired, but index did not change.
    // Safest interpretation here is a full-buffer lap / overrun.
    if (new_pos == old_pos) {
        gps_uart->dma_overrun_count++;

        // We know at least one full buffer worth of data was not safely distinguishable.
        // Count it as dropped.
        gps_uart->rb_drop_bytes += len;

        // Advance stays the same because DMA wrapped back to same index.
        gps_uart->dma_last_index = new_pos;
        return;
    }

    if (new_pos > old_pos) {
        // Contiguous region [old_pos, new_pos)
        attempted = new_pos - old_pos;
        written = rb_write(gps_uart->rb, &gps_uart->dma_buffer[old_pos],
                           attempted);

        total_attempted += attempted;
        total_written += written;

        if (written < attempted) {
            gps_uart->rb_drop_bytes += (attempted - written);
        }
    } else {
        // Wrapped region: [old_pos, len)
        attempted = len - old_pos;
        written = rb_write(gps_uart->rb, &gps_uart->dma_buffer[old_pos],
                           attempted);

        total_attempted += attempted;
        total_written += written;

        if (written < attempted) {
            gps_uart->rb_drop_bytes += (attempted - written);
        }

        // Then [0, new_pos)
        if (new_pos > 0) {
            attempted = new_pos;
            written = rb_write(gps_uart->rb, &gps_uart->dma_buffer[0],
                               attempted);

            total_attempted += attempted;
            total_written += written;

            if (written < attempted) {
                gps_uart->rb_drop_bytes += (attempted - written);
            }
        }
    }

    gps_uart->dma_last_index = new_pos;

    // If you want "bytes successfully buffered":
    debugger->total_bytes_received += total_written;
}

/* -----------------------
 *  Core functions
 * ----------------------- */
// initalize gpsUart struct (must do before everything else)
bool gps_uart_init(GpsUartHandler* gps_uart,
                   UART_HandleTypeDef* uart_address_pin, RingBuffer* rb,
                   uint8_t* dma_buffer, size_t dma_len) {
    // basic defensive checks
    if (!gps_uart || !uart_address_pin || !rb || !dma_buffer)
        return false;
    if (dma_len < 2)
        return false;

    gps_uart->huart = uart_address_pin;

    // ensure that the ring buffer has been initlalized before putting it into the gpsUart
    if (rb->buffer_init)
        gps_uart->rb = rb;
    else
        return false;

    gps_uart->dma_buffer = dma_buffer;
    gps_uart->dma_buffer_length = dma_len;
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
    HAL_StatusTypeDef st = HAL_UARTEx_ReceiveToIdle_DMA(gps_uart->huart,
                                                        gps_uart->dma_buffer, gps_uart->dma_buffer_length);

    if (st != HAL_OK)
        return st;  // return wtv error code

    // NOTE: Could safely do below code because IDLE inturrupts are significantly more often (~10x) than HT or FT
    // according to testing. See one of the checkouts on github with uart debugging implementation
    // // Disable half-transfer interrupts to reduce IRQ load.
    // // IDLE events will still happen and RxEventCallback will still get called to take stuff from dmaBuffer and put into
    // // custom ring buffer.
    // // however, we are not disabling the full transfer complete inturrupt from the DMA
    // if (gps_uart->huart->hdmarx != NULL)
    //     __HAL_DMA_DISABLE_IT(gps_uart->huart->hdmarx, DMA_IT_HT);

    return HAL_OK;
}

// if data recieved, take from dma buffer, put into the ring buffer
void gps_uart_on_rx_event(GpsStats* debugger, GpsUartHandler* gps_uart,
                          uint16_t new_pos) {
    if (!gps_uart || !gps_uart->huart)
        return;

    data_into_ring(debugger, gps_uart, new_pos);  // read from dma buffer into the ring buffer.
}

// a wrapper around the ring buffer function rbRead(). returns max_length bytes read
size_t gps_uart_read(GpsUartHandler* gps_uart, uint8_t* out, size_t max_length) {
    if (!gps_uart || !gps_uart->rb || !out || max_length == 0)
        return 0;

    return rb_read(gps_uart->rb, out, max_length);  // Read up to maxLength bytes into out. Returns number of bytes read
}

// get the lost bytes, then clear the counter (so 32 bit counter won't eventually overflow if gps runs for a long time)
size_t gps_uart_get_and_clear_dropped_bytes(GpsUartHandler* gps_uart) {
    // AI generated
    if (!gps_uart)  // basic defensive checks
        return 0;

    // Save current interrupt state and disable interrupts globally
    // This prevents the DMA callback from firing while we do math
    uint32_t primask_bit = __get_PRIMASK();
    __disable_irq();

    // Safely read and clear the counters
    size_t dropped = gps_uart->rb_drop_bytes;
    gps_uart->rb_drop_bytes = 0;

    // Restore interrupts to their exact previous state
    __set_PRIMASK(primask_bit);
    return dropped;
}
