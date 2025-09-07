#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/sync.h"

#ifndef CD_BUFFER_SIZE
    #define CD_BUFFER_SIZE 256
#endif
//make safe
typedef struct circular_buffer
{
    char ac_buffer[CD_BUFFER_SIZE];     // data buffer
    char *pc_buffer_end;
    uint32_t u32_capacity;  // maximum number of items in the buffer
    uint32_t u32_count;     // number of items in the buffer
    char *pc_head;       // pointer to head
    char *pc_tail;       // pointer to tail
} circular_buffer;

//returns true if cb is empty
bool cb_isEmpty(circular_buffer *cb);

void cb_init(circular_buffer *cb);

void cb_push_back(circular_buffer *cb, char *c_item);

void cb_pop_front(circular_buffer *cb, char *c_item);

char* cb_pop_multiple(circular_buffer *cb, uint32_t u32_num);

void cb_print_cstring_to_buffer(circular_buffer *cb, char* pc_cString);

void cb_print_float_to_buffer(circular_buffer *cb, float f_num);