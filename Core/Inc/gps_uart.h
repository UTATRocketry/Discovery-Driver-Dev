/*
 * gps_uart.h
 *
 *  Created on: Mar 3, 2026
 *      Author: prith
 *  Status:
 *
 *  Purpose:
 *  - using DMA, take data from uart, put into ring buffer as defined in ring_buffer.h
 *  - provide a clean interface for higher layers (gps_parser)
 *
 * 	Notes:
 * 	- This module does NOT parse NMEA (or UBX binary)
 * 	- DMA circular buffer alone can work, but a ring buffer adds some extra saftey apperently. also convient if ever
 * 	  switching from dma to inturrupts
 * 	- using UART5 RX with circular DMA
 */

#ifndef INC_GPS_UART_H_
#define INC_GPS_UART_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ring_buffer.h"
#include "stm32l4xx_hal.h" /*Needed for UART*/

/* -----------------------
 *  Structs
 * ----------------------- */
typedef struct {
    UART_HandleTypeDef* huart;  // UART used for GPS (&huart5 rn)
    RingBuffer* rb;             // ring buffer as main storage (must be already initalized)

    uint8_t* dmaBuffer;             // DMA circular RX buffer storage
    size_t dmaBufferLength;         // length of dmaBuf in bytes
    volatile size_t dmaLastIndex;   // last processed index into dmaBuf [0..dmaLen-1]
    volatile uint32_t rbDropBytes;  // check for overflow and how many bytes were lost
} GpsUart;

/* -----------------------
 *  Functions
 * ----------------------- */

// Initalize UART, ring buffer, DMA RX buffer
bool gpsUartInit(GpsUart* gpsUart, UART_HandleTypeDef* uartAddressPin, RingBuffer* rb, uint8_t* dmaBuffer,
                 size_t dmaBufferLength);

// Start UART Receive-to-IDLE with DMA into dmaBuffer (circular). (called once after init)
HAL_StatusTypeDef gpsUartStartRx(GpsUart* gpsUart);

// Put new bytes from the DMA buffer into the ring buffer. HAL_UARTEx_RxEventCallback calls this
void gpsUartOnRxEvent(GpsUart* gpsUart, uint16_t size);

// a wrapper around the ring buffer function rbRead(). returns number of bytes actually read
size_t gpsUartRead(GpsUart* gpsUart, uint8_t* out, size_t maxLength);

#endif /* INC_GPS_UART_H_ */
