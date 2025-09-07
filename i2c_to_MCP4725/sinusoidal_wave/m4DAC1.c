/**
 * m4 MCP4725 DAC example 2
 * In this example, I2C communication is utilized to
 * generate a sinusoidal signal from the DAC. To increase
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

//The array is essentially a table with values in an order
//that will represent a sinusoidal via the DAC
const uint16_t DACLookupSineWave[512];

int main() {
    //I run my general initializations
    stdio_init_all();
    edub_init();

    //I initialize the watchdog for 10 seconds.
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

    // We need to set up the handlers firsttrue
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

    //I output a little explanation of the program
    //via UART.
    printIntro();

    //The count variable will be how we increment through the 
    //sinusoidal table. It's value will dtermine the frequency
    //of the signal.
    uint8_t count = 1;

    //This marks what index in the table we will be referecing.
    uint16_t state = 0;

    while (1) {

        if(!cb_is_empty(p_cb_in)){          //If my input cb holds values, 
            cb_pop_next(p_cb_in, pu8_ch);   //we pop the next one

            //The following if statements check to see if 
            //a "+" or "-" is pressed. It will not print 
            //if the frequency is altered, but it will if it has 
            //reached the smallest or greatest count. It could go
            //be larger, but for the sake of this program, that is
            //unnecessary.
            if((*pu8_ch == 43)&&(count != 50)){
                count += 1;
            }
            else if((*pu8_ch == 45)&&(count != 1)){
                count -= 1;
            }
            else {
                cb_push(p_cb_out, pu8_ch);
            }
        }

        if(!cb_is_empty(p_cb_out)){                     //if the output buffer isn't empty, enable it's IRQ and set the ISR to write
            u8_read_or_write = 1;                       //through the u8_read_or_write variable
            uart_set_irq_enables(UART_ID, true, true);
        }

        //I added this function before initiating contact with the 
        //DAC peripheral. If the contact is interrupted by a watchdog
        //reboot, the DAC will get stuck to a value. This prevents that.
        if(watchdog_get_time_remaining_ms() > 1000){
            DACInput(DACLookupSineWave[state]);
        }

        //Here I update the index to get from the table.
        state = state + count;

        //Recycle through the matrix
        if(state > 512){
            state = 0;
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
    uint8_t au8_message[] = "Welcome to I2C and MCP4725 Demonstration. Press '+' to raise the signal's voltage and '-' to lower it. ";
    
    for(uint8_t i = 0; i < 103; i++){
        cb_push(p_cb_out, &au8_message[i]);
    }
}

//This is the table that represents the sinusoid
const uint16_t DACLookupSineWave[512]  =  
{
	2048, 2073, 2098, 2123, 2148, 2174, 2199, 2224, 
	2249, 2274, 2299, 2324, 2349, 2373, 2398, 2423,
	2448, 2472, 2497, 2521, 2546, 2570, 2594, 2618,
	2643, 2667, 2690, 2714, 2738, 2762, 2785, 2808,
	2832, 2855, 2878, 2901, 2924, 2946, 2969, 2991,
	3013, 3036, 3057, 3079, 3101, 3122, 3144, 3165,
	3186, 3207, 3227, 3248, 3268, 3288, 3308, 3328,
	3347, 3367, 3386, 3405, 3423, 3442, 3460, 3478,
	3496, 3514, 3531, 3548, 3565, 3582, 3599, 3615,
	3631, 3647, 3663, 3678, 3693, 3708, 3722, 3737,
	3751, 3765, 3778, 3792, 3805, 3817, 3830, 3842,
	3854, 3866, 3877, 3888, 3899, 3910, 3920, 3930,
	3940, 3950, 3959, 3968, 3976, 3985, 3993, 4000,
	4008, 4015, 4022, 4028, 4035, 4041, 4046, 4052,
	4057, 4061, 4066, 4070, 4074, 4077, 4081, 4084,
	4086, 4088, 4090, 4092, 4094, 4095, 4095, 4095, 
	4095, 4095, 4095, 4095, 4094, 4092, 4090, 4088,
	4086, 4084, 4081, 4077, 4074, 4070, 4066, 4061,
	4057, 4052, 4046, 4041, 4035, 4028, 4022, 4015,
	4008, 4000, 3993, 3985, 3976, 3968, 3959, 3950,
	3940, 3930, 3920, 3910, 3899, 3888, 3877, 3866,
	3854, 3842, 3830, 3817, 3805, 3792, 3778, 3765,
	3751, 3737, 3722, 3708, 3693, 3678, 3663, 3647,
	3631, 3615, 3599, 3582, 3565, 3548, 3531, 3514,
	3496, 3478, 3460, 3442, 3423, 3405, 3386, 3367,
	3347, 3328, 3308, 3288, 3268, 3248, 3227, 3207,
	3186, 3165, 3144, 3122, 3101, 3079, 3057, 3036,
	3013, 2991, 2969, 2946, 2924, 2901, 2878, 2855,
	2832, 2808, 2785, 2762, 2738, 2714, 2690, 2667,
	2643, 2618, 2594, 2570, 2546, 2521, 2497, 2472,
	2448, 2423, 2398, 2373, 2349, 2324, 2299, 2274,
	2249, 2224, 2199, 2174, 2148, 2123, 2098, 2073,
	2048, 2023, 1998, 1973, 1948, 1922, 1897, 1872, 
	1847, 1822, 1797, 1772, 1747, 1723, 1698, 1673,
	1648, 1624, 1599, 1575, 1550, 1526, 1502, 1478,
	1453, 1429, 1406, 1382, 1358, 1334, 1311, 1288,
	1264, 1241, 1218, 1195, 1172, 1150, 1127, 1105,
	1083, 1060, 1039, 1017,  995,  974,  952,  931,
	 910,  889,  869,  848,  828,  808,  788,  768,
	 749,  729,  710,  691,  673,  654,  636,  618,
	 600,  582,  565,  548,  531,  514,  497,  481,
	 465,  449,  433,  418,  403,  388,  374,  359,
	 345,  331,  318,  304,  291,  279,  266,  254,
	 242,  230,  219,  208,  197,  186,  176,  166,
	 156,  146,  137,  128,  120,  111,  103,   96,
		88,   81,   74,   68,   61,   55,   50,   44,
		39,   35,   30,   26,   22,   19,   15,   12,
		10,    8,    6,    4,    2,    1,    1,    0, 
		 0,    0,    1,    1,    2,    4,    6,    8,
		10,   12,   15,   19,   22,   26,   30,   35,
		39,   44,   50,   55,   61,   68,   74,   81,
		88,   96,  103,  111,  120,  128,  137,  146,
	 156,  166,  176,  186,  197,  208,  219,  230,
	 242,  254,  266,  279,  291,  304,  318,  331,
	 345,  359,  374,  388,  403,  418,  433,  449,
	 465,  481,  497,  514,  531,  548,  565,  582,
	 600,  618,  636,  654,  673,  691,  710,  729,
	 749,  768,  788,  808,  828,  848,  869,  889,
	 910,  931,  952,  974,  995, 1017, 1039, 1060,
	1083, 1105, 1127, 1150, 1172, 1195, 1218, 1241,
	1264, 1288, 1311, 1334, 1358, 1382, 1406, 1429,
	1453, 1478, 1502, 1526, 1550, 1575, 1599, 1624,
	1648, 1673, 1698, 1723, 1747, 1772, 1797, 1822,
	1847, 1872, 1897, 1922, 1948, 1973, 1998, 2023  
};
