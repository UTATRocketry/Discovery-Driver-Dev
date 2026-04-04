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
    size_t buffer_size;              // capacity of buffer in bytes (for embedded systems, apperantly making this a power of 2 is
                                     // more efficent)
    size_t mask;                     // mask = size - 1 (used later for the wrap around calculatoins). Storing it in the struct is faster
    volatile size_t head;            // write to head
    volatile size_t tail;            // read from tail
    volatile size_t overflow_count;  // counts bytes dropped due to full buffer
    bool buffer_init;                // verify that the buffer has been initalized
} RingBuffer;

/* -----------------------
 *  Functions
 * ----------------------- */
// Initialize ring buffer with provided storageAddress + size
bool rb_init(RingBuffer* rb, uint8_t* storage_address, size_t size);

// returns amount of bytes available in the buffer
size_t rb_available(const RingBuffer* rb);

// returns amount of bytes unread sitting in the buffer
size_t rb_read(RingBuffer* rb, uint8_t* out_data, size_t max_length);

// Helper: push many bytes (stops when full). Returns number pushed.
size_t rb_write(RingBuffer* rb, const uint8_t* in_data, size_t len);

// clear everything in case of error
void rb_reset(RingBuffer* rb);

#endif
