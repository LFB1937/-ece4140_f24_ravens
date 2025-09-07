#define PICO_ADC_0_PIN         26
#define PICO_ADC_0_CHANNEL     0
#define PICO_ADC_1_PIN         27
#define PICO_ADC_1_CHANNEL     1
#define PICO_ADC_2_PIN         28
#define PICO_ADC_2_CHANNEL     2

#define PICO_SPK_PIN           22

#define PICOEDUB_LED0_PIN      1
#define PICOEDUB_LED1_PIN      0
#define PICOEDUB_LED2_PIN      2
#define PICOEDUB_LED3_PIN      3

#define PICOEDUB_ROW0_PIN      6
#define PICOEDUB_ROW1_PIN      7
#define PICOEDUB_ROW2_PIN      8
#define PICOEDUB_ROW3_PIN      9

#define PICOEDUB_SW2_PIN       9
#define PICOEDUB_SW3_PIN       8
#define PICOEDUB_SW4_PIN       7
#define PICOEDUB_SW5_PIN       6

#define PICOEDUB_COL0_PIN      10
#define PICOEDUB_COL1_PIN      11
#define PICOEDUB_COL2_PIN      12
#define PICOEDUB_COL3_PIN      13

//first row gpio pin is 6
#define PICO_ROW_GPIO_OFFSET   6
#define PICO_COL_GPIO_OFFSET   10
#define PICO_SW_PIN_OFFSET     6

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 250
#endif

#define DEFAULT_TIMEOUT_ITERATIONS 5000

/****************************************
 * Want to make a sample every ms or so
 * 48 Mhz * 1ms = 4800 cycles
*****************************************/
#define DEFUALT_ADC_PERIOD_CYCLES 4800
#define PICO2_FIFO_INTR_SIZE 1
#define LM45_mV_to_degC 0.010f
#define C_to_F_scaler (9.0f/5.0f)
#define C_to_F_offset 32
#define Buzzer_Threshold 75
//EVERY SENSOR IS SLIGHTLY DIFFERENT, THE FOLLOWING DEFININTION CAN BE CHANGED 
//TO CALIBRATE THE SENSOR. THE ONE I HAVE IS ABOUT 6.9 DEGREES OFF
#define CALIBRATION_FACTOR -6.9f

#define UART_ID uart0
#define BAUD_RATE 57600
#define DATA_BITS 8
#define STOP_BITS 1
#define START_BITS 1
#define PARITY    UART_PARITY_ODD

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 0
#define UART_RX_PIN 1




//Needed for gpio IRQ
#include <stdio.h>
#include "pico/stdlib.h"
#include "cb.h"

#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/irq.h"
#include "hardware/watchdog.h"
#include "hardware/adc.h"
#include "hardware/structs/adc.h"
#include "hardware/structs/uart.h"

void edub_init();
void keypad_init();
//uint gpio has to be uint or compiler flashes warning if uint_32t...
void gpio_callback(uint gpio, uint32_t events);
void pico_led_init(void);
void pico_set_led(bool b_led_on);
void gpio_to_EDUB_led_init(uint8_t u8_PIN_NUM);
void flash_led_gpio_to_EDUB(uint8_t  u8_PIN_NUM);
void gpio_input_reset(uint8_t u8_pin_num);
bool uart_putc_nonBlocking(uart_inst_t *uart, const uint8_t src, uint32_t numAttempts);
bool adc_fifo_drain_nonBlocking(uint32_t u32_timeOut);