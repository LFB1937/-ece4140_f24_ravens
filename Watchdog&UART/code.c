/**
 * m4
*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "picoedub.h"
#include "hardware/uart.h"
#include "circular_buffer.h"
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "hardware/watchdog.h"
#include "hardware/i2c.h"

// Here I create the values that ill configure UART
#define UART_ID uart0
#define BAUD_RATE 57600
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_ODD

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used. This comment is from
// the pico 2 example repo
#define UART_TX_PIN 0
#define UART_RX_PIN 1

// The function below allows me to access clock time 
// to use in the timer.
static uint64_t get_time(void) {
    // Reading low latches the high value
    uint32_t lo = timer_hw->timelr;
    uint32_t hi = timer_hw->timehr;
    return ((uint64_t) hi << 32u) | lo;
}
/// \end::get_time[]

/// \tag::alarm_standalone[]
// Use alarm 0. Like the above UART values, this configures my timers
#define ALARM_NUM 0
#define ALARM_IRQ timer_hardware_alarm_get_irq_num(timer_hw, ALARM_NUM)

// This global variable holds the state of the led. It is off in the beginning
bool b_led_flag = 0;

// This global variable controls the length of my timer and the period 
// of the LED toggling. It's default is a second (half on, half off)
uint16_t u16_period = 1000;

static void alarm_irq(void) {
    // Clear the alarm irq
    hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);
    
    // Here I toggle and set the LED
    b_led_flag = !b_led_flag;
    pico_set_LED3(b_led_flag);
    
    // This computes the next clock intstance at which the
    // alarm will "fire". The 500 * modifies u16_period to fit 
    // the how the alarm works
    uint64_t target = timer_hw->timerawl + (500 * u16_period);

    // Write the lower 32 bits of the target time to the alarm which
    // will arm it
    timer_hw->alarm[ALARM_NUM] = (uint32_t) target;
}

// Here I initialize my two circular buffers
// as well as pointers to refer them through
circular_buffer cb_in;
circular_buffer cb_out;
circular_buffer *p_cb_in = &cb_in;
circular_buffer *p_cb_out = &cb_out;

// This global variable controls
// what will be happening in my 
// uart isr
uint8_t u8_read_or_write = 0;

// I use these global variables to receive from uart,
// pass the value to the buffers, then send from uart.
// I made it global to prevent further isr instructions.
uint8_t u8_ch;
uint8_t *pu8_ch = &u8_ch;

void on_uart_rx() {
    watchdog_update();
    if(!u8_read_or_write){
        u8_ch = uart_getc(UART_ID); //if I'm receiving, i get the value and place
        cb_push(p_cb_in, pu8_ch);   //it in the appropriate buffer
    }
    if (u8_read_or_write) {
        if(!cb_is_empty(p_cb_out)){         //if I am sending, I pop my buffer after being 
            cb_pop_next(p_cb_out, pu8_ch);  //sure it's not empty and output it to uart
            uart_putc(UART_ID, u8_ch);
        }
        u8_read_or_write = 0;                       //Here I reassign my this variable so that the isr
        uart_set_irq_enables(UART_ID, true, false); //will receive, and I disable the TX interrupt.
    }
}

int main() {
    //I run my general initializations
    stdio_init_all();
    edub_init();
    watchdog_enable(10000, 1);

    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    // Make the I2C pins available to picotool
    //bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));

    // Set up our UART with a basic baud rate.
    uart_init(UART_ID, BAUD_RATE);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_TX_PIN));
    gpio_set_function(UART_RX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_RX_PIN));

    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(UART_ID, false, false);

    // Set our data format
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(UART_ID, false);

    // We need to set up the handlers first
    // Select correct interrupts for the UART we are using
    int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

    // Set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);

    //irq_set_exclusive_handler(UART_IRQ, on_uart_tx);
    irq_set_enabled(UART_IRQ, true);

    // Enable the UART to send interrupts for RX and TX
    uart_set_irq_enables(UART_ID, true, false); // Enable both RX and TX interrupts

    // Enable the interrupt for our alarm (the timer outputs 4 alarm irqs)
    hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);

    // Set irq handler for alarm irq
    irq_set_exclusive_handler(ALARM_IRQ, alarm_irq);

    // Enable the alarm irq
    irq_set_enabled(ALARM_IRQ, true);

    // This is the first time the alarmed will be armed.
    // Afterwards, it'll be alarmed through the isr.
    // Alarm is only 32 bits so if trying to delay more
    // than that need to be careful and keep track of the upper
    // bits
    uint64_t target = timer_hw->timerawl + (500 * u16_period);

    // Write the lower 32 bits of the target time to the alarm which
    // will arm it
    timer_hw->alarm[ALARM_NUM] = (uint32_t) target;

    // I initialize both circular buffers
    cb_init(p_cb_in);
    cb_init(p_cb_out);

    // There is a small odditywhere the first 
    // bit printed to UART isn't processed. I can't tell
    // if this a function of the UART or the serial monitor
    // app I am using, but printing an emtpy space solves the issue.
    uart_putc(UART_ID, ' ');

    while (1) {

        if(!cb_is_empty(p_cb_in)){          //If my input cb holds values, 
            cb_pop_next(p_cb_in, pu8_ch);   //we pop the next one
            cb_push(p_cb_out, pu8_ch);
        }

        if(!cb_is_empty(p_cb_out)){                     //if the output buffer isn't empty, enable it's IRQ and set the ISR to write
            u8_read_or_write = 1;                       //through the u8_read_or_write variable
            uart_set_irq_enables(UART_ID, true, true);
        }
    }   
}
