/**
 * Written by Gabriel Buckner Fall 24'
 */

#include "picoedub.h"

//Variables for ADC
// 12-bit conversion, assume max value == ADC_VREF == 3.3 V
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



//variable for speaker
bool b_toggleSpeaker = false;

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
        if(b_toggleSpeaker){
            gpio_put(PICO_SPK_PIN, true);
        }
    } else if(b_toggle){
        pico_set_led(false);
        b_toggle = 0;
        if(b_toggleSpeaker){
            gpio_put(PICO_SPK_PIN, false);
        }
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
            cb_pop_front(pcb_outputBuffer, pu8_callbackBuf);
            uart_putc_nonBlocking(UART_ID, *pu8_callbackBuf, DEFAULT_TIMEOUT_ITERATIONS);
        }
        else{
            uart_set_irqs_enabled(UART_ID, false, false);
            //this disables the TX interrupt. This is here to prevent unnessesary calls to this function

        }
        
    }
}

void ADC_callback(){
    //bool b_temp = false;
    //If some toggling function is desired when intr is called then it can be done in the next if statement.
    /*
    if(!b_temp){
        b_temp = 1;
    } else if(b_temp){
        b_temp = 0;
    } 
    */

    while(!adc_fifo_is_empty()){
            u16_ADC_out =  adc_fifo_get();
        }
}

//initializations needed for this program
void edub_init(){
    //initialize basic peripherals
    stdio_init_all();
    pico_led_init();

    gpio_init(PICOEDUB_LED2_PIN);
    gpio_set_dir(PICOEDUB_LED2_PIN, GPIO_OUT);
    gpio_put(PICOEDUB_LED2_PIN, false);

    gpio_init(PICOEDUB_LED3_PIN);
    gpio_set_dir(PICOEDUB_LED3_PIN, GPIO_OUT);
    gpio_put(PICOEDUB_LED3_PIN, false);

    //initialze circular buffers
    cb_init(pcb_outputBuffer);
    cb_init(pcb_inputBuffer);
    
    //speaker setup
    gpio_init(PICO_SPK_PIN);
    gpio_set_dir(PICO_SPK_PIN, GPIO_OUT);
    gpio_put(PICO_SPK_PIN, false);

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
    irq_set_exclusive_handler(ADC_IRQ_FIFO, ADC_callback);
    
    //disabling digital functions of GPIO28 and selecting channel 2 in the adc input multiplexor
    adc_gpio_init(PICO_ADC_2_PIN );
    adc_select_input(PICO_ADC_2_CHANNEL);


    //turn on ADC and its clock
    adc_hw->cs |= 1;
    //turn on FIFO
    adc_hw->fcs |= 1;

    //enable IRQ
    adc_hw->inte |= 1;
    //Interrupt still needs to be enabled in NVIC
    //irq_set_enabled(ADC_IRQ_FIFO, true); ------> Is done at bottom of initializations
    //set the threshold field
    adc_hw->fcs |= PICO2_FIFO_INTR_SIZE  << 24;
    //turn on byte shift
    //adc_hw->fcs |= 1 << 1; 



    //This function can be used instead to setup the FIFO
    //adc_fifo_setup (bool en, bool dreq_en, uint16_t dreq_thresh, bool err_in_fifo, bool byte_shift)
    //this will turn on free running sampling mode
    adc_run(true);
    
    
    /****************************************
     * Want to make a sample every ms or so
     * 48 Mhz * 1ms = 4800 cycles
    *****************************************/
    adc_set_clkdiv(DEFUALT_ADC_PERIOD_CYCLES);
     


//End ADC init*******************************************************

    //Turn on all interrupts nedded
         
    //sets the callback function for uart
    irq_set_exclusive_handler (UART0_IRQ, uartCallback);
    //enables the actual iterrupt on the calling core
    
    irq_set_enabled(UART0_IRQ, true);
    
    irq_set_enabled(ADC_IRQ_FIFO, true);
    
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

    

    cb_print_cstring_to_buffer(pcb_outputBuffer, (char *) &"HELLO ADC INTR BY GABRIEL BUCKNER AND THE RAVENS F24!\n\r");

    while (true) {
        //read the 12 bit data from ADC
      
        sleep_ms(10);

            f_ADC_out = (float)u16_ADC_out * f_conversion_factor;
            
            /***********************************
             * From LM45 datasheet
             * Vout = (10 mv/°C * x°C)
             * sensor is accurate +- 3.6 °F
            ************************************/
        //following line converts volts to °C then to °F
        f_ADC_out = (f_ADC_out / LM45_mV_to_degC) * C_to_F_scaler + C_to_F_offset + CALIBRATION_FACTOR;
        if(f_ADC_out >= Buzzer_Threshold){
            b_toggleSpeaker = true;
        }
        else{
            b_toggleSpeaker = false;
        }
        


        u8_temp = ' ';
        cb_push_back(pcb_outputBuffer, pu8_temp);
        cb_print_float_to_buffer(pcb_outputBuffer, f_ADC_out);

        cb_print_cstring_to_buffer(pcb_outputBuffer, (char *) &" \'F\r");
        uart_set_irqs_enabled(UART_ID, false, true);

        
    }
}
