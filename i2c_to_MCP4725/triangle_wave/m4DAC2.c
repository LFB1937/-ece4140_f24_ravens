/**
 * m4 MCP4725 DAC example 2
 * In this example, I2C communication is utilized to
 * generate a triangular signal from the DAC. To increase
 * it's frequency, simply press "-". To lower it's frequency,
 * simply press "+". This is done via UART, so you need a
 * serial communication application like putty for this to 
 * function.
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
#include "pico/binary_info.h"

// Here I create the values that will configure UART
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

    //We return the value of the running clock
    //to be used in our timer ISR
    return ((uint64_t) hi << 32u) | lo;
}

// Use alarm 0. Like the above UART values, this configures my timers
#define ALARM_NUM 0
#define ALARM_IRQ timer_hardware_alarm_get_irq_num(timer_hw, ALARM_NUM)

// This global variable holds the state of the led. It is off in the beginning
bool b_led_flag = 0;

// This global variable controls the length of my timer and the period 
// of the LED toggling. It's set to a millisecond (half on, half off)
uint16_t u16_period = 10;

static void alarm_irq(void) {
    // Clear the alarm irq
    hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);
    
    // Here I toggle and set the LED
    b_led_flag = !b_led_flag;
    pico_set_led(b_led_flag);
    
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
volatile uint8_t u8_read_or_write = 0;

// I use these global variables to receive from uart,
// pass the value to the buffers, then send from uart.
// I made it global to prevent further isr instructions.
uint8_t u8_ch;
uint8_t *pu8_ch = &u8_ch;

void on_uart_rx() {
    //I made the uart ISR a critical region to prevent
    //potential race conditions with the variable u8_read_or_write.
    uint32_t u32_register = save_and_disable_interrupts(); 

    //This is when I've decided to update my watchdog, so 
    //if nobody types anything for the alloted time, the 
    //system will reboot.
    watchdog_update();

    if(!u8_read_or_write){
        u8_ch = uart_getc(UART_ID); //if I'm receiving, i get the value and place
        cb_push(p_cb_in, pu8_ch);   //it in the appropriate buffer
    }
    else {
        if(!cb_is_empty(p_cb_out)){         //if I am sending, I pop my buffer after being 
            cb_pop_next(p_cb_out, pu8_ch);  //sure it's not empty and output it to uart
            uart_putc(UART_ID, u8_ch);
        }
        u8_read_or_write = 0;                       //Here I reassign my variable so that the isr
        uart_set_irq_enables(UART_ID, true, false); //will receive, and I disable the TX interrupt.
    }

    //end critical region
    restore_interrupts(u32_register);
}

//This function will control the DAC's output via i2c
bool DACInput(uint16_t inputCode);

//This provides a UART intro to the serial communications
void printIntro();

int main() {
    //I run my general initializations
    stdio_init_all();
    edub_init();

    //This function enables watchdog for 10 seconds
    watchdog_enable(10000, 1);

    //This initializes by i2c channel
    i2c_init(i2c_default, 100 * 1000);                          //I set the channel and transfer rate
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C); 
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));

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

    //This variable dictates the increase or decrease of the signal
    bool up_down = 0;

    //This var represents the signal to send to the DAC
    int16_t count = 0;

    //This var is how much to increase or decrease count.
    //It controls the frequency.
    uint8_t step = 1;

    //Here I call the funciton to print some instructions
    printIntro();

    while (1) {

        if(!cb_is_empty(p_cb_in)){          //If my input cb holds values, 
            cb_pop_next(p_cb_in, pu8_ch);   //we pop the next one

            //The following if statements check to see if 
            //a "+" or "-" is pressed. It will not print 
            //if the frequency is altered, but it will if it has 
            //reached the smallest or greatest step. It could go
            //be larger, but for the sake of this program, that is
            //unnecessary.
            if((*pu8_ch == 43)&&(step != 200)){
                step++;
            }
            else if((*pu8_ch == 45)&&(step != 1)){
                step--;
            }
            else {
                cb_push(p_cb_out, pu8_ch);
            }
        }

        if(!cb_is_empty(p_cb_out)){                     //if the output buffer isn't empty, enable it's IRQ and set the ISR to write
            u8_read_or_write = 1;                       //through the u8_read_or_write variable
            uart_set_irq_enables(UART_ID, true, true);
        }

        //Here, I increase or decrease the signal, depending
        //on up_down
        if(!up_down){
            count += step;
        }
        else {
            count -= step;
        }

        //Here I switch the direction of the signal
        if(count >= 4095){
            up_down = 1;
        }
        else if(count <= 0){
            up_down = 0;
        }

        //I added this function before initiating contact with the 
        //DAC peripheral. If the contact is interrupted by a watchdog
        //reboot, the DAC will get stuck to a value. This prevents that.
        if(watchdog_get_time_remaining_ms() > 1000){
            DACInput(count);
        }
    }   
}

//This function transmits information to the DAC. It sets the DAC 
//to "FastMode" and "PowerDownOff" modes and updates the DAC registers
//to output a new voltage. 
bool DACInput(uint16_t inputCode) {

    //The data buffer var will be sent to the DAC
    uint8_t dataBuffer[2];

    //This will be the second byte sent
	uint8_t lowByte = 0;

    //This will be the first
	uint8_t highByte = 0;

    //This is for error detection
    uint8_t ReturnCode = 0;

    //I make the low byte the bottom half of the value passed
    //by the function.
    lowByte  = (uint8_t)(inputCode & 0x00FF);

    //I make the high byte the top half of the value passed by 
    //the function.
	highByte = (uint8_t)((inputCode >> 8) & 0x00FF);

    //The following command could be modified to change
    //the mode the DAC is operating in. for more information,
    //observe the data sheet.
	dataBuffer[0] = 0x00 | (0x00 << 4) | highByte; //C2,C1,PD1,PD0,D11,D10,D9,D8
	dataBuffer[1] = lowByte;                            //D7,D6,D5,D4,D3,D2,D1,D0

    //Finally, I write to the DAC via an i2c timeout funciton. The timeout occurs if
    //the message isn't sent within 50 milliseconds.
	ReturnCode = i2c_write_timeout_us(i2c_default, 0x60, dataBuffer, 2 , true, 50000);

    //I check to make sure the message sent
	if (ReturnCode < 1){
		return false;
	}

    return true;
}

//This simply prints a message to the user that explains the program.
void printIntro(){
    uint8_t au8_message[] = "Welcome to I2C and MCP4725 Demonstration. Press '+' to raise the signal's frequency and '-' to lower it. ";
    
    for(uint8_t i = 0; i < 105; i++){
        cb_push(p_cb_out, &au8_message[i]);
        //uart_putc(UART_ID, au8_message[i]);
    }
}
