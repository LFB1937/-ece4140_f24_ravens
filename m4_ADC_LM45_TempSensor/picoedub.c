
#include "picoedub.h"
/*
void gpio_callback(uint gpio, uint32_t events) {
}
*/


void gpio_input_reset(uint8_t u8_pin_num){
    gpio_set_dir(u8_pin_num, GPIO_OUT);
    gpio_put(u8_pin_num, false);
    gpio_set_dir(u8_pin_num, GPIO_IN);
}



// Initialize the GPIO for the LED
void pico_led_init(void) {
#ifdef PICO_DEFAULT_LED_PIN
    // A device like Pico that uses a GPIO for the LED will define PICO_DEFAULT_LED_PIN
    // so we can use normal GPIO functionality to turn the led on and off
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#endif
}

// Turn the LED on or off
void pico_set_led(bool b_led_on) {
#if defined(PICO_DEFAULT_LED_PIN)
    // Just set the GPIO on or off
    gpio_put(PICO_DEFAULT_LED_PIN, b_led_on);
#endif
}

void gpio_to_EDUB_led_init(uint8_t u8_PIN_NUM) {
    gpio_init(u8_PIN_NUM);
    gpio_set_dir(u8_PIN_NUM, GPIO_OUT);
}


void flash_led_gpio_to_EDUB(uint8_t  u8_PIN_NUM){
    gpio_put(u8_PIN_NUM, true);
    sleep_ms(LED_DELAY_MS);
    gpio_put(u8_PIN_NUM, false);
}

void keypad_init(){
    //initialize columns as sio
    gpio_set_function(PICOEDUB_COL0_PIN,GPIO_FUNC_SIO);
    gpio_set_function(PICOEDUB_COL1_PIN,GPIO_FUNC_SIO);
    gpio_set_function(PICOEDUB_COL2_PIN,GPIO_FUNC_SIO);
    gpio_set_function(PICOEDUB_COL3_PIN,GPIO_FUNC_SIO);
    //set output for columns
    gpio_set_dir(PICOEDUB_COL0_PIN, GPIO_OUT);
    gpio_set_dir(PICOEDUB_COL1_PIN, GPIO_OUT);
    gpio_set_dir(PICOEDUB_COL2_PIN, GPIO_OUT);
    gpio_set_dir(PICOEDUB_COL3_PIN, GPIO_OUT);
    //set up columns as high
    gpio_set_mask(0b1111<< PICOEDUB_COL0_PIN);

    //initalize rows as input, with pulldown.
    gpio_init(PICOEDUB_ROW0_PIN);
    gpio_init(PICOEDUB_ROW1_PIN);
    gpio_init(PICOEDUB_ROW2_PIN);
    gpio_init(PICOEDUB_ROW3_PIN);
    gpio_set_dir(PICOEDUB_ROW0_PIN, GPIO_IN);
    gpio_set_dir(PICOEDUB_ROW1_PIN, GPIO_IN);
    gpio_set_dir(PICOEDUB_ROW2_PIN, GPIO_IN);
    gpio_set_dir(PICOEDUB_ROW3_PIN, GPIO_IN);
    gpio_pull_down(PICOEDUB_ROW0_PIN);
    gpio_pull_down(PICOEDUB_ROW1_PIN);
    gpio_pull_down(PICOEDUB_ROW2_PIN);
    gpio_pull_down(PICOEDUB_ROW3_PIN);

    //set up interrupts for rows
    gpio_set_irq_enabled_with_callback(PICOEDUB_ROW0_PIN, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
    gpio_set_irq_enabled(PICOEDUB_ROW1_PIN, GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(PICOEDUB_ROW2_PIN, GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(PICOEDUB_ROW3_PIN, GPIO_IRQ_EDGE_RISE, true);
}
//non blocking write char function based off sdk. Returns true for wrote, returns false for timeout
bool uart_putc_nonBlocking(uart_inst_t *uart, const uint8_t src, uint32_t numAttempts){
    
    for (size_t i = 0; i < numAttempts; ++i) {
            if(!uart_is_writable(uart)){
                continue;
            }
            else{
                uart_get_hw(uart)->dr = src;
                return true;
            }
    }
    return false;
}

