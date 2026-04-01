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
static inline uint32_t getBufferOverflow(const GpsUart* gpsUart) {
    return (gpsUart && gpsUart->rb) ? gpsUart->rb->overflowCount : 0;
}

// calculate distance between two indices in a circular buffer
static size_t modDistance(size_t a, size_t b, size_t m) {
    if (a >= b) return a - b;
    return (m - b) + a;
}

// Harvest bytes from dmaBuffer[oldPos..newPos) into ring buffer, with wrap handling
static void dataIntoRing(GpsUart* gpsUart, size_t newPos) {
    // All AI generated, not modified
    if (!gpsUart || !gpsUart->rb || !gpsUart->dmaBuffer || gpsUart->dmaBufferLength == 0)
        return;

    size_t len = gpsUart->dmaBufferLength;
    size_t oldPos = gpsUart->dmaLastIndex;

    // Clamp defensively
    if (newPos >= len) newPos %= len;
    if (oldPos >= len) oldPos %= len;

    // No new data
    if (newPos == oldPos)
        return;

    size_t attempted;
    size_t written;

    if (newPos > oldPos) {
        // Contiguous region [oldPos, newPos)
        attempted = newPos - oldPos;
        written = rbWrite(gpsUart->rb, &gpsUart->dmaBuffer[oldPos], attempted);

        if (written < attempted) {
            gpsUart->rbDropBytes += (attempted - written);
        }
    } else {
        // Wrapped: [oldPos, len)
        attempted = len - oldPos;
        written = rbWrite(gpsUart->rb, &gpsUart->dmaBuffer[oldPos], attempted);

        if (written < attempted) {
            gpsUart->rbDropBytes += (attempted - written);
        }

        // Then [0, newPos)
        if (newPos > 0) {
            attempted = newPos;
            written = rbWrite(gpsUart->rb, &gpsUart->dmaBuffer[0], attempted);

            if (written < attempted) {
                gpsUart->rbDropBytes += (attempted - written);
            }
        }
    }

    gpsUart->dmaLastIndex = newPos;
}

// Determine current DMA write position in circular buffer using DMA counter
static size_t getDmaWritePosition(const GpsUart* gpsUart, uint16_t sizeHint) {
    // completly ai written code, unmodfied
    // purpose is to figure out the correct write position cause HAL_UARTEx_RxEventCallback(..., Size) from HAL driver
    // might not give the right awnser for circular buffers apperently
    if (!gpsUart || !gpsUart->huart || !gpsUart->dmaBufferLength) return 0;

    // If HAL linked the RX DMA handle, we can compute the current write index:
    // pos = dmaLen - NDTR
    if (gpsUart->huart->hdmarx != NULL) {
        uint32_t remaining = __HAL_DMA_GET_COUNTER(gpsUart->huart->hdmarx);
        size_t pos = gpsUart->dmaBufferLength - (size_t)remaining;

        if (pos >= gpsUart->dmaBufferLength)
            pos %= gpsUart->dmaBufferLength;
        return pos;
    }

    // Fallback: HAL's sizeHint is often "bytes in buffer" for RxEventCallback.
    size_t pos = (size_t)sizeHint;
    if (pos >= gpsUart->dmaBufferLength) pos %= gpsUart->dmaBufferLength;
    return pos;
}

/* -----------------------
 *  Core functions
 * ----------------------- */
// initalize gpsUart struct (must do before everything else)
bool gpsUartInit(GpsUart* gpsUart, UART_HandleTypeDef* uartAddressPin, RingBuffer* rb, uint8_t* dmaBuffer,
                 size_t dmaBufferLength) {
    if (!gpsUart || !uartAddressPin || !rb || !dmaBuffer) return false;
    if (dmaBufferLength < 2) return false;  // ring buffers have to be > 2

    gpsUart->huart = uartAddressPin;
    // ensure that the ring buffer has been initlalized before putting it into the gpsUart
    if (rb->bufferInit)
        gpsUart->rb = rb;
    else
        return false;

    gpsUart->dmaBuffer = dmaBuffer;
    gpsUart->dmaBufferLength = dmaBufferLength;
    gpsUart->dmaLastIndex = 0;
    gpsUart->dmaOverrunCount = 0;

    return true;
}

// run once to start dma data transfer
HAL_StatusTypeDef gpsUartStartRx(GpsUart* gpsUart) {
    // if not initalized or soemthing else is wrong with anything in the sturct
    if (!gpsUart || !gpsUart->huart || !gpsUart->dmaBuffer || gpsUart->dmaBufferLength == 0)
        return HAL_ERROR;

    // clear
    gpsUart->dmaLastIndex = 0;
    gpsUart->dmaOverrunCount = 0;
    rbReset(gpsUart->rb);

    // Start Receive-to-IDLE with DMA into the circular buffer
    HAL_StatusTypeDef st = HAL_UARTEx_ReceiveToIdle_DMA(gpsUart->huart, gpsUart->dmaBuffer,
                                                        gpsUart->dmaBufferLength);
    if (st != HAL_OK) return st;

    // Disable half-transfer interrupts to reduce IRQ load.
    // IDLE events will still happen and RxEventCallback will still get called to take stuff from dmaBuffer and put into
    // custom ring buffer.
    // however, we are not disabling the transfer complete inturrupt from the DMA
    if (gpsUart->huart->hdmarx != NULL)
        __HAL_DMA_DISABLE_IT(gpsUart->huart->hdmarx, DMA_IT_HT);

    return HAL_OK;
}

// if data recieved, put into the ring buffer
void gpsUartOnRxEvent(GpsUart* gpsUart, uint16_t size) {
    if (!gpsUart || !gpsUart->huart) return;
    size_t newPos = getDmaWritePosition(gpsUart, size);  // figure out where the DMA is currently wirting
    dataIntoRing(gpsUart, newPos);                       // put new data into the ring buffer.
}

// a wrapper around the ring buffer function rbRead(). returns number of bytes actually read
size_t gpsUartRead(GpsUart* gpsUart, uint8_t* out, size_t maxLength) {
    if (!gpsUart || !gpsUart->rb || !out || maxLength == 0) return 0;
    return rbRead(gpsUart->rb, out, maxLength);
}
