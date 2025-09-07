/**
 * Mathias Hagewood CB
 */
#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include "pico/stdlib.h"
#include "hardware/sync.h"
#include <stdio.h>

#define BUFFER_SIZE 256

typedef struct {
    uint8_t buffer[BUFFER_SIZE];     // Pointer to the buffer
    uint8_t *head;   // Index of the head (next write position)
    uint8_t *tail;   // Index of the tail (next read position)
    uint8_t *last_index;       // Size of the buffer (must be a power of 2)
    uint8_t full;          // Is the buffer full?
    uint8_t empty;         // Is the buffer empty?
}circular_buffer;

void cb_init(circular_buffer *cb);
int cb_push(circular_buffer *cb, uint8_t *data);
int cb_pop_next(circular_buffer *cb, uint8_t *data);
int cb_pop_recent(circular_buffer *cb, uint8_t *data);
uint8_t cb_is_empty(circular_buffer *cb);
uint8_t cb_is_full(circular_buffer *cb);
#endif
