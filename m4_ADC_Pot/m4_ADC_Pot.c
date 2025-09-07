/**
 * Written by Gabriel Buckner Fall 24'
 */

#include "picoedub.h"

//Variables for ADC
// 12-bit conversion, assume max value == ADC_VREF == 3.3 V
    //change this to no float...
    const float f_conversion_factor = 3.3f / (1 << 12);
    uint16_t u16_ADC_out;
    float f_ADC_out;
    uint8_t u8_temp;
    uint8_t *pu8_temp = &u8_temp;
    uint32_t u32_outputIn_mV;


//variables for timer and alarm
int8_t i8_alarmNum = 0;
uint32_t u32_expireTime;
uint32_t u32_refTime = 0;
bool b_resetAlarm = false;
uint32_t u32_time2Expire= 1000;


//circular buffers
circular_buffer  cb_inputBuffer;
circular_buffer *pcb_inputBuffer = &cb_inputBuffer;
circular_buffer  cb_outputBuffer;
circular_buffer *pcb_outputBuffer = &cb_outputBuffer;
bool b_toggle = false;
uint8_t u8_buf = 0;
uint8_t *pu8_buf = &u8_buf;
uint8_t u8_callbackBuf = 0;
uint8_t *pu8_callbackBuf = &u8_callbackBuf;


void alarmCallback(){
    if(!b_toggle){
        pico_set_led(true);
        b_toggle = 1;
    } else if(b_toggle){
        //gpio_put(PICOEDUB_LED3_PIN, false);
        pico_set_led(false);
        b_toggle = 0;
    }  
    u32_refTime = time_us_32();  
    hardware_alarm_set_target(i8_alarmNum, u32_refTime + u32_time2Expire);
    watchdog_update();     
}

void uartCallback(){
    /*
    if(uart_is_readable(UART_ID)){
        
        u8_callbackBuf = uart_getc(UART_ID);
        cb_push_back(pcb_inputBuffer, pu8_callbackBuf); 
    } 
    */
    if(uart_is_writable(UART_ID)){
        if(!cb_isEmpty(pcb_outputBuffer)){
            //test led
            gpio_put(PICOEDUB_LED2_PIN, true);
            cb_pop_front(pcb_outputBuffer, pu8_callbackBuf);
            uart_putc_nonBlocking(UART_ID, *pu8_callbackBuf, DEFAULT_TIMEOUT_ITERATIONS);
        }
        else{
            uart_set_irqs_enabled(UART_ID, false, false);
            //this disables the TX interrupt. This is here to prevent unnessesary calls to this function

        }
        
    }
}

//initializations needed for this program
void edub_init(){
    //initialize basic peripherals
    stdio_init_all();
    pico_led_init();

    //initialze circular buffers
    cb_init(pcb_outputBuffer);
    cb_init(pcb_inputBuffer);

//Start UART init*********************************************************
    // Set up our UART with a basic baud rate.
    uart_init(UART_ID, 2400);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select    
    gpio_set_function(UART_TX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_TX_PIN));
    gpio_set_function(UART_RX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_RX_PIN));

    // Actually, we want a different speed
    // The call will return the actual baud rate selected, which will be as close as
    // possible to that requested    
    uint32_t u32_baud_actual = uart_set_baudrate(UART_ID, BAUD_RATE);

    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(UART_ID, false, false);
    uart_set_fifo_enabled (UART_ID, false);
    // Set our data format
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
    //uart_set_fifo_enabled(UART_ID, false);
//End UART init*****************************************************

//Start TIMER init************************************************** 
    //will not panic with false
    i8_alarmNum = hardware_alarm_claim_unused(false);
    u32_time2Expire = 1000;
    //sets callback function for the timer ISR
    hardware_alarm_set_callback(i8_alarmNum, &alarmCallback);
//End TIMER init**************************************************** 

//Start ADC init*****************************************************
    //initialize ADC hardware
    adc_init();
    //disabling digital functions of GPIO27
    adc_gpio_init(PICO_ADC_1_PIN);
    adc_select_input(PICO_ADC_1_CHANNEL);

    //turn on free sampling mode. Will make the ADC run back to back (takes 96 clock cycles per sample per datasheet)
    adc_hw->cs |= (1<<3);

//End ADC init*******************************************************

    //Turn on all interrupts nedded
         
    //sets the callback function for uart
    irq_set_exclusive_handler (UART0_IRQ, uartCallback);
    //enables the actual iterrupt on the calling core
    irq_set_enabled(UART0_IRQ, true);
    //getting the reference time to set the alarm
    u32_refTime = time_us_32();
    hardware_alarm_set_target(i8_alarmNum, u32_refTime + u32_time2Expire);
    
    //only want to send on transmit but only want to start when we have data. Will be set in main
    uart_set_irqs_enabled(UART_ID, false, false);
}

int main() {
    edub_init();
    if (watchdog_caused_reboot()) {
        cb_print_cstring_to_buffer(pcb_outputBuffer, (char *) &"WATCHDOG REBOOT\n\r");
        gpio_put(PICOEDUB_LED3_PIN, true);
        
    } else {
        cb_print_cstring_to_buffer(pcb_outputBuffer, (char *) &"CLEAN BOOT\n\r");
    }
    
    // Enable the watchdog, requiring the watchdog to be updated every 100ms or the chip will reboot
    // second arg is pause on debug which means the watchdog will pause when stepping through code
    //watchdog_enable(1000, 1);
    
    //Tx interrupt only works when the Tx is ready for more data. So send something to make the Tx intr start firing.
    uart_putc_nonBlocking(UART_ID, '\r', DEFAULT_TIMEOUT_ITERATIONS);

    cb_print_cstring_to_buffer(pcb_outputBuffer, (char *) &"HELLO ADC POT BY GABRIEL BUCKNER AND THE RAVENS F24!\n\r");

    while (true) {
        //read the 12 bit data from ADC
      
        sleep_ms(10);
        //can use this line instead to read adc
        //u16_ADC_out = ((uint16_t)adc_hw->result & (0b111111111111));
        u16_ADC_out = (uint16_t) adc_hw->result;
        //u16_ADC_out = adc_read();
        f_ADC_out = (float)u16_ADC_out * f_conversion_factor;
        
        u8_temp = ' ';
        cb_push_back(pcb_outputBuffer, pu8_temp);
        cb_print_float_to_buffer(pcb_outputBuffer, f_ADC_out);
        u8_temp = ' ';
        cb_push_back(pcb_outputBuffer, pu8_temp);
        u8_temp = 'V';
        cb_push_back(pcb_outputBuffer, pu8_temp);
        u8_temp = '\r';
        cb_push_back(pcb_outputBuffer, pu8_temp);
    
        uart_set_irqs_enabled(UART_ID, false, true);
    }
}
