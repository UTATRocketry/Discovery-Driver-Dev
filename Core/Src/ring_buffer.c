/*
 * ring_buffer.c
 *
 *  Created on: Mar 3, 2026
 *      Author: prith
 *
 */
#include "ring_buffer.h"

/* -----------------------
 *  Internal Helper Functions
 * ----------------------- */
// For optimzation of buffer size, size must be power of two
static bool is_power_of_two(size_t x) {
    return (x != 0) && ((x & (x - 1)) == 0);
}

// adds a byte to ring buffer. returns false if buffer filled (head will point to tail next)
static bool rb_push_byte(RingBuffer* rb, uint8_t byte) {
    size_t head = rb->head;
    size_t next_head = (head + 1) & rb->mask;  // the mask will handle wrap around

    // Full if advancing head catches tail
    if (next_head == rb->tail) {
        rb->overflow_count++;
        return false;
    }

    rb->buffer[head] = byte;
    rb->head = next_head;  // increment head
    return true;
}

/* -----------------------
 *  Core Functions
 * ----------------------- */
// Initialize ring buffer at provided storageLocation for size
bool rb_init(RingBuffer* rb, uint8_t* storage_location, size_t size) {
    // check for validity of input parameters
    if (!rb || !storage_location) return false;
    if (size < 2) return false;                // size must be >= 2 for ring buffers
    if (!is_power_of_two(size)) return false;  // just for the sake of optimization

    rb->buffer = storage_location;  // define the pointer to the buffer
    rb->buffer_size = size;
    rb->mask = size - 1;

    // start head and tail at 0 (same index)
    rb->head = 0;
    rb->tail = 0;
    rb->overflow_count = 0;
    rb->buffer_init = true;

    return true;
}

// Read up to maxLength bytes into outData. Returns number of bytes read
size_t rb_read(RingBuffer* rb, uint8_t* out_data, size_t max_length) {
    // check for validity of input parameters
    if (!rb || !rb->buffer || !out_data || (max_length == 0)) return 0;

    size_t count = 0;  // counts how many bytes have been read into outData

    while (count < max_length) {
        size_t tail = rb->tail;
        if (tail == rb->head) break;  // Empty when tail catches head

        out_data[count] = rb->buffer[tail];
        count++;
        rb->tail = (tail + 1) & rb->mask;  // rb->mask, mask will wrap around if at the end of the buffer (only change tail)
    }
    return count;
}

// Helper: push many bytes (stops when full). Returns number pushed.
size_t rb_write(RingBuffer* rb, const uint8_t* in_data, size_t length) {
    // check for validity of input parameters
    if (!rb || !rb->buffer || !in_data || (length == 0)) return 0;

    size_t pushed = 0;  // counts how many bytes have been pushing from inData to rb

    for (size_t i = 0; i < length; i++) {
        if (!rb_push_byte(rb, in_data[i]))  // if rbPushByte returns false, buffer full, pushing stops. else, add byte to buffer
            break;

        pushed++;
    }

    return pushed;
}

// returns amount of bytes unread sitting in the buffer
size_t rb_available(const RingBuffer* rb) {
    if (!rb) return 0;
    return (rb->head - rb->tail) & rb->mask;  // The mask automatically handles the wrap-around math
}

// clear everything in case of error
void rb_reset(RingBuffer* rb) {
    if (!rb) return;

    rb->head = 0;
    rb->tail = 0;
    rb->overflow_count = 0;  // not keeping overflow as history, clearing it too
}
