#include "circular_buffer.h"

void cb_init(circular_buffer *cb) { 
    cb->last_index = &cb->buffer[BUFFER_SIZE - 1];        //I set the size
    cb->head = cb->buffer;          //I set both at starting point
    cb->tail = cb->buffer;
    cb->full = 0;       //full is false
    cb->empty = 1;       //empty is true
}

int cb_push(circular_buffer *cb, uint8_t *data){
    //This should protect the code from interrupts
    uint32_t u32_register = save_and_disable_interrupts(); 
   // Check if the buffer is full
    if (cb->full) {
        restore_interrupts(u32_register);
        return -1; // Buffer is full
    }

    // Write the data to the buffer
    *cb->head = *data;

    // Move the head forward
    cb->head++;
       
    // wrap around if I am at the last index
    if (cb->head > cb->last_index) {
        cb->head = cb->buffer; 
    }
    
    // Update the full and empty status
    cb->empty = 0;
    if (cb->head == cb->tail) {
        cb->full = 1;       // Buffer is full
    }

    restore_interrupts(u32_register);

    return(0);
};

int cb_pop_next(circular_buffer *cb, uint8_t *data){
    //This should protect the code from interrupts
    uint32_t u32_register = save_and_disable_interrupts();
    // Check if the buffer is empty
    if (cb->empty) {
        restore_interrupts(u32_register);
        return -1; // Buffer is empty
    }

    // Read the data from the buffer
    *data = *cb->tail;

    // Move the tail forward
    cb->tail++; 

    // Update the full and empty status
    cb->full = 0;

    // if tail reaches the end, wrap around
    if (cb->tail > cb->last_index) {
        cb->tail = cb->buffer; 
    }

    if (cb->head == cb->tail) {
        cb->empty = 1; // Buffer is now empty
    }

    //restore interrupts
    restore_interrupts(u32_register);

    return(0);
};

int cb_pop_recent(circular_buffer *cb, uint8_t *data){
    //This should protect the code from interrupts
    uint32_t u32_register = save_and_disable_interrupts();
    
    // Check if the buffer is empty
    if (cb->empty) {
        restore_interrupts(u32_register);
        return -1; // Buffer is empty
    }

    // Move the head back to get the most recent item
    cb->head--;
    
    // Wrap around if needed
    if (cb->head < cb->buffer) {
        cb->head = cb->last_index;  // Wrap around to the last index
    }

    *data = *cb->head; // obtain data

    cb->full = 0; // update full var
    if (cb->head == cb->tail) {
        cb->empty = 1; // Buffer is now empty
    }

    restore_interrupts(u32_register);
    return(0);
};

//simply return empty var
uint8_t cb_is_empty(circular_buffer *cb){
    return(cb->empty);
};

//return full var
uint8_t cb_is_full(circular_buffer *cb){
    return(cb->full);
};
