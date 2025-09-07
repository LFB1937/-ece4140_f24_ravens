/**************************************************************
 * FileName:         I2C_application1.c
 * Dependencies:     see INCLUDE section below 
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Purpose:
 * This program will interface the DS3231 RTC by utilizing
 * a uart to print the current time upon Power-On Reset (POR).
 * 
 * The program turns on the pico LED every second which will 
 * act as a heartbeat for this program. The program will toggle
 * LED3 every 5 seconds and indicate the current state of LED3 
 * on the uart. The program will turn on LED2 2-3 seconds after
 * the watchdog timer implemented in this program activates.
 *
 * Side Effects:
 * The timing between the DS 3231 differs from the system clock
 * causing LED3 to toggle earlier than expected or to miss its 
 * timing. 
 * The Uart RX and TX interrupts were not implemented 
 * because the I2C can only block for read and write. 
 * After running for a long time, the uart disconnects upon POR. 
 * To fix the issue, hit the reset switch on the eduboard.
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Contributors            Date
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Tate Finley       12/6/2024 
 * Tucker Basham     12/6/2024
 **************************************************************/

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "hardware/watchdog.h"
#include "hardware/i2c.h"

#define DS3231_ADDRESS 0x68 // RTC I2C Address
#define SDA_PIN 4           // GPIO for SDA
#define SCL_PIN 5           // GPIO for SCL
#define LED_PIN 25          // GPIO for onboard LED
#define PICOEDUB_LED3_PIN      3
#define PICOEDUB_LED2_PIN      2

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

// This global variable controls the length of my timer and the period 
// of the LED toggling. It's default is a second (half on, half off)
uint16_t u16_period = 1000;


bool pico_led_state = true; //pico led state upon POR

// Write to a DS3231 register
void ds3231_write_register(uint8_t reg, uint8_t data){
    uint8_t buf[2] = {reg, data};
    i2c_write_blocking(i2c0, DS3231_ADDRESS, buf, 2, false);
}

// Read from a DS3231 register
void ds3231_read_register(uint8_t reg, uint8_t *data){
    i2c_write_blocking(i2c0, DS3231_ADDRESS, &reg, 1, true);
    i2c_read_blocking(i2c0, DS3231_ADDRESS, data, 1, false);
}

// Read the current time (seconds, minutes, hours) from the DS3231
void read_time(uint8_t *hours, uint8_t *minutes, uint8_t *seconds){
    uint8_t time[3]; // Seconds, Minutes, Hours
    for (uint8_t i = 0; i < 3; i++) {
        ds3231_read_register(i, &time[i]);
    }
    *seconds = time[0] & 0x7F; // Mask out CH bit
    *minutes = time[1] & 0x7F;
    *hours = time[2] & 0x3F;
}

/*****************************************************************
 * Function: void ds3231_init(void)
 * 
 * PreCondition: None
 * 
 * Input: None
 * 
 * Output: None
 * 
 * Side Effects: None
 * 
 * Overview: This routine intializes the DS3231 to work with the
            raspberry pi and eduboard.
 *****************************************************************/
void ds3231_init(){

    i2c_init(i2c0, 100 * 1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1); // Start with LED ON
}

/*****************************************************************
 * Function: void alarm_irq(void)
 * 
 * PreCondition: Timer is initialized
 * 
 * Input: None
 * 
 * Output: None
 * 
 * Side Effects: None
 * 
 * Overview: This routine changes the state of the pico LED upon 
             timer end.
 *****************************************************************/
static void alarm_irq(void) {
    // Clear the alarm irq
    hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);
    
    //turn on pico LED whenever timer goes off
    pico_led_state = !pico_led_state;         // Toggle LED state
    gpio_put(LED_PIN, pico_led_state);  // Update LED

    // This computes the next clock intstance at which the
    // alarm will "fire". The 500 * modifies u16_period to fit 
    // the how the alarm works
    uint64_t target = timer_hw->timerawl + (500 * u16_period);

    // Write the lower 32 bits of the target time to the alarm which
    // will arm it
    timer_hw->alarm[ALARM_NUM] = (uint32_t) target;
}

int main() {
    stdio_init_all();
    gpio_init(PICOEDUB_LED2_PIN);
    gpio_set_dir(PICOEDUB_LED2_PIN, GPIO_OUT);
    gpio_init(PICOEDUB_LED3_PIN);
    gpio_set_dir(PICOEDUB_LED3_PIN, GPIO_OUT);
    watchdog_enable(12000, 1); //enable watchdog every 12 seconds.
    ds3231_init();
    
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

    uint8_t previous_seconds = 0xFF; // Initialize to an invalid value to force the first display
    uint8_t previous_minutes = 0xFF;
    uint8_t previous_hours = 0xFF;

    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    
    //Read current time upon POR
    read_time(&hours, &minutes, &seconds);
    printf("Current Time: %02X:%02X:%02X\n", hours, minutes, seconds);
    
    gpio_put(PICOEDUB_LED2_PIN, false); //turn off LED2 when watchdog timer activates.

    
    bool led3_state = true; // LED3 starts as ON
    gpio_put(PICOEDUB_LED3_PIN, led3_state);
    uint8_t watchdog_on = 0; // Incremental value until LED2 turns on

    while (true) {
        //Keep track of previous time
        previous_minutes = minutes; 
        previous_hours = hours;
        
        read_time(&hours, &minutes, &seconds);
        
            
        // Toggle LED every 5 seconds
        if (previous_seconds != seconds && seconds % 5 == 0) {
            printf("LED %s at seconds: %02X\n", led3_state ? "ON" : "OFF", seconds);
            previous_seconds = seconds; 
            led3_state = !led3_state;
            gpio_put(PICOEDUB_LED3_PIN, led3_state);
            
        }else 
        {
            if(previous_seconds != seconds)
            {
                previous_seconds = seconds;
                watchdog_on++;
                //Turn LED2 on 2-3 seconds after POR.
                if(watchdog_on >= 3)
                {
                    gpio_put(PICOEDUB_LED2_PIN, true);
                }   
            }
        }

        
        sleep_ms(100); // Polling interval
    }

    return 0;
}
