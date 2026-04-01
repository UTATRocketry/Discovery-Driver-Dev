/*
 * ring_buffer.h
 *
 *  Created on: Mar 3, 2026
 *      Author: prith
 *
 *	Status: done
 *
 *  Notes:
 *  	- for DMA, no need for seperate ring buffer if DMA operating in circular mode?
 *  	- for inturrupts, ring buffer needed
 */

#ifndef INC_RING_BUFFER_H_
#define INC_RING_BUFFER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* -----------------------
 *  Structs
 * ----------------------- */
typedef struct {
    uint8_t* buffer;
    size_t bufferSize;                // capacity of buffer in bytes (for embedded systems, apperantly making this a power of 2 is
                                      // more efficent)
    size_t mask;                      // mask = size - 1 (used later for the wrap around calculatoins). Storing it in the struct is faster
    volatile size_t head;             // write to head
    volatile size_t tail;             // read from tail
    volatile uint32_t overflowCount;  // counts bytes dropped due to full buffer
    bool bufferInit;                  // verify that the buffer has been initalized
} RingBuffer;

/* -----------------------
 *  Functions
 * ----------------------- */
// Initialize ring buffer with provided storageAddress + size
bool rbInit(RingBuffer* rb, uint8_t* storageAddress, size_t size);

// returns amount of bits available in the buffer
size_t rbAvailable(const RingBuffer* rb);

// returns amount of bytes unread sitting in the buffer
size_t rbRead(RingBuffer* rb, uint8_t* outData, size_t maxLenth);

// Helper: push many bytes (stops when full). Returns number pushed.
size_t rbWrite(RingBuffer* rb, const uint8_t* inData, size_t len);

// clear everything in case of error
void rbReset(RingBuffer* rb);

/*More functions to possibly implement if theres a need(?):
// Peek next byte without removing. Returns false if empty.
bool rbPeek(const RingBuffer *rb, uint8_t *out);

void rbReset(RingBuffer *rb);

void rbDebug(RingBuffer* cb);

// push one byte, return false if full
bool rbPush(RingBuffer *rb, uint8_t b);

//Pop one byte. Returns false if empty.
bool rbPop(RingBuffer *rb, uint8_t *out);

*/

#endif /* INC_RING_BUFFER_H_ */
