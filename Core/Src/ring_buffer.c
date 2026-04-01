/*
 * ring_buffer.c
 *
 *  Created on: Mar 3, 2026
 *      Author: prith
 *
 *  Status: done
 */
#include "ring_buffer.h"

/* -----------------------
 *  Internal Helper Functions
 * ----------------------- */

// For optimzation of buffer size, size must be power of two
static bool isPowerOfTwo(size_t x) {
    return (x != 0) && ((x & (x - 1)) == 0);
}

// adds a byte to ring buffer. returns false if buffer filled (head will point to tail next)
static bool rbPushByte(RingBuffer* rb, uint8_t byte) {
    size_t head = rb->head;
    size_t nextHead = (head + 1) & rb->mask;  // the mask will handle wrap around

    // Full if advancing head catches tail
    if (nextHead == rb->tail) {
        rb->overflowCount++;
        return false;
    }

    rb->buffer[head] = byte;
    rb->head = nextHead;  // increment head
    return true;
}

/* -----------------------
 *  Core Functions
 * ----------------------- */
// Initialize ring buffer at provided storageLocation for size
bool rbInit(RingBuffer* rb, uint8_t* storageLocation, size_t size) {
    // check for validity of input parameters
    if (!rb || !storageLocation) return false;
    if (size < 2) return false;             // size must be >= 2 for ring buffers
    if (!isPowerOfTwo(size)) return false;  // just for the sake of optimization

    rb->buffer = storageLocation;  // define the pointer to the buffer
    rb->bufferSize = size;
    rb->mask = size - 1;

    // start head and tail at 0 (same index)
    rb->head = 0;
    rb->tail = 0;
    rb->overflowCount = 0;
    rb->bufferInit = true;

    return true;
}

// Read up to maxLength bytes into outData. Returns number of bytes read
size_t rbRead(RingBuffer* rb, uint8_t* outData, size_t maxLength) {
    // check for validity of input parameters
    if (!rb || !rb->buffer || !outData || (maxLength == 0)) return 0;

    size_t count = 0;  // counts how many bytes have been read into outData

    while (count < maxLength) {
        size_t tail = rb->tail;
        if (tail == rb->head) break;  // Empty when tail catches head

        outData[count] = rb->buffer[tail];
        count++;
        rb->tail = (tail + 1) & rb->mask;  // rb->mask, mask will wrap around if at the end of the buffer (only change tail)
    }
    return count;
}

// Helper: push many bytes (stops when full). Returns number pushed.
size_t rbWrite(RingBuffer* rb, const uint8_t* inData, size_t length) {
    // check for validity of input parameters
    if (!rb || !rb->buffer || !inData || (length == 0)) return 0;

    size_t pushed = 0;  // counts how many bytes have been pushing from inData to rb

    for (size_t i = 0; i < length; i++) {
        if (!rbPushByte(rb, inData[i]))  // if rbPushByte returns false, buffer full, pushing stops. else, add byte to buffer
            break;

        pushed++;
    }

    return pushed;
}

// returns amount of bytes unread sitting in the buffer
size_t rbAvailable(const RingBuffer* rb) {
    if (!rb) return 0;
    return (rb->head - rb->tail) & rb->mask;  // The mask automatically handles the wrap-around math
}

// clear everything in case of error
void rbReset(RingBuffer* rb) {
    if (!rb) return;

    rb->head = 0;
    rb->tail = 0;
    rb->overflowCount = 0;  // not keeping overflow as history, clearing it too
}
