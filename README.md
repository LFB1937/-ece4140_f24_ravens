# ece4140_f24_ravens
repo for the RAVENs for Fall2024 Embedded Systems class

DS3231 - This file contains an implementation of the DS3231 RTC I2C to display the current time when the watchdog timer goes off displayed via the uart. It contains a heartbeat pico LED set turning on every 1 second. It implements LEDs 2 and 3. LED2 turns off when the watchdog timer goes off, and on after 2-3 seconds upon power on reset (POR). LED3 toggles every 5 seconds regardless of POR and the current state when that occurs is displayed via the uart.    

I2C_code - basic I2C example. Not explicitly part of project.  

Watchdog&UART - basic watchdog and UART examples. Not explicitly part of project.

i2c_to_MCP4725 - This file contains two separate directories for two different ways to use the MCP4725 via I2C. Read in README in this folder for more info.

m4_ADC_LM45_TempSensor - This file contains code that displays the temperature in real time. It displays the information via UART at 57600 baud rate. See the picoedub.h file for definitions.

m4_ADC_LM45_TempSensor_Interrupt - This file contains code that displays the temperature in real time. This file is similar to the above example except is utilizes the FIFO for the ADC in order to read the ADC. Instead of running continuously back to back conversions like the above example, the code sets the clock_div for the ADC to make it have one conversion ever 1ms by default. This sample is then read into a variable and processed in the main. The example also uses the timer in order to play a buzzer if the read temperature is above a certain level. By default this is 75 deg F. The fifo interrupt for the ADC is also set to go off at a threshold of 1. This means that as soon as one sample is ready in the FIFO, the INTR will go off until the FIFO is below 1 sample in capacity. See the datasheet for information regarding the intr function. All these definitions can be changed in the picoedub.h file. Including the FIFO threshold, the temp threshold, and the clock_div value.

m4_ADC_POT - This folder contains code that reads the onboard potentiometer and displays it via UART.

