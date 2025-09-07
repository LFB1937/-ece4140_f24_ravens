#include "cb.h"

char ac_returnBuffer[CD_BUFFER_SIZE];

bool cb_isEmpty(circular_buffer *cb){
    if(cb->u32_count == 0){
        return true;
    }
    else{
        return false;
    }
}

void cb_init(circular_buffer *cb){
    uint32_t status = save_and_disable_interrupts();
    cb->u32_count = 0;
    cb->pc_head = cb->ac_buffer;
    cb->pc_tail = cb->ac_buffer;
    cb->pc_buffer_end = &cb->ac_buffer[CD_BUFFER_SIZE-1];
    cb->u32_capacity =  CD_BUFFER_SIZE;  // maximum number of items in the buffer
    restore_interrupts_from_disabled(status);
}

void cb_push_back(circular_buffer *cb, char *c_item){
    uint32_t status = save_and_disable_interrupts();
    if(cb->u32_count == cb->u32_capacity){
        //error handling right now is to do nothing
        //*c_item = '!';
        return;
    }
    
    //add char item and increment pc_head
    *cb->pc_head = *c_item;
    
    if(cb->pc_head == cb->pc_buffer_end){
        cb->pc_head = cb->ac_buffer;
    }
    else{
        cb->pc_head++;
    }
    cb->u32_count += 1;
    restore_interrupts_from_disabled(status);

}

void cb_pop_front(circular_buffer *cb, char *c_item){
    uint32_t status = save_and_disable_interrupts();
    if(cb->u32_count == 0){
        *c_item = '?';
        return;
    }
    
    *c_item = *cb->pc_tail;
    if(cb->pc_tail == cb->pc_buffer_end){
        cb->pc_tail = cb->ac_buffer;
    }
    else{
        cb->pc_tail = (cb->pc_tail + 1);
    }
    cb->u32_count--;
    restore_interrupts_from_disabled(status);
}

char *cb_pop_multiple(circular_buffer *cb, uint32_t u32_num){
    uint32_t status = save_and_disable_interrupts();
    if(cb->u32_count == 0){
        //handle error
    }
    if(cb->u32_count < u32_num){
        //handle error
    }
    
    for(uint32_t u32_i = 0; u32_i < u32_num; u32_i++){ 
        ac_returnBuffer[u32_i] = *cb->pc_tail;
        if(cb->pc_tail == cb->pc_buffer_end){
            cb->pc_tail = cb->ac_buffer;
        }
        else{
            cb->pc_tail = (cb->pc_tail + 1);
        }
        cb->u32_count--;
    }
    restore_interrupts_from_disabled(status);
    return ac_returnBuffer;

}

void cb_print_cstring_to_buffer(circular_buffer *cb, char* pc_cString){
    uint32_t status = save_and_disable_interrupts();
    char * pc_charPointer;

    while(*pc_cString != '\0'){
        /////pushBackFunction Re pasted (critical reigon already made)
        if(cb->u32_count == cb->u32_capacity){
        //error handling right now is to do nothing
        //*c_item = '!';
        return;
        }
        //add char item and increment pc_head
        *cb->pc_head = *pc_cString++;
        
        if(cb->pc_head == cb->pc_buffer_end){
            cb->pc_head = cb->ac_buffer;
        }
        else{
            cb->pc_head++;
        }
        cb->u32_count += 1;
    }
    
    restore_interrupts_from_disabled(status);
    return;
}

void cb_print_float_to_buffer(circular_buffer *cb, float f_num){
    uint32_t status = save_and_disable_interrupts();
    char ac_tempChar[20];
    int8_t i8_numCharsWritten;
    char * pc_charPointer;
    i8_numCharsWritten = sprintf(ac_tempChar, "%f", f_num);

    if(i8_numCharsWritten < 0){
        return;
    }

    for(uint8_t u8_i = 0; u8_i < i8_numCharsWritten; u8_i++){
        pc_charPointer = &ac_tempChar[u8_i];
        /////pushBackFunction Re pasted (critical reigon already made)
        if(cb->u32_count == cb->u32_capacity){
        //error handling right now is to do nothing
        //*c_item = '!';
        return;
        }
        //add char item and increment pc_head
        *cb->pc_head = *pc_charPointer;
        
        if(cb->pc_head == cb->pc_buffer_end){
            cb->pc_head = cb->ac_buffer;
        }
        else{
            cb->pc_head++;
        }
        cb->u32_count += 1;
    }
    
    restore_interrupts_from_disabled(status);
    return;
}