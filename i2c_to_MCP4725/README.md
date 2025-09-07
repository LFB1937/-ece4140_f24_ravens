# Directions for using the MCP4725 via I2C

This directory contains code to generate a triangular and sinusoidal signal from the peripheral. In both examples, the period of the signal can be increased by pressing "+" and decreased by pressing "-" via UART. To do that, you need some serial interface tool installed such as putty.   
  
To view the signal being produced from the MCP4725, an oscilloscope is required.   
  
The way the peripheral functions depends on the mode it's in. The code in this directory configures it to "FastMode" and "PowerDownOff". This means we only write to 3 bytes via I2C and the output acts as though it were connected to high impedence instead of some large resistance. For using other modes, reference the datasheet.  
  
The following programs assume you have cloned the pico 2 SDK and installed picotool to your local machine.  
  
To run either program, clone either the triangle_signal or sinusoidal_signal directory from the repo. Then, in a command terminal, type the following:  
  
mkdir build  
cd build  
cmake -DPICO_BOARD=pico2 -DPICO_DEOPTIMIZED_DEBUG=1 -DCMAKE_BUILD_TYPE=Debug ..  
make  

## Other 

There is an IRQ that toggles an LED every millisecond.  
There is a watchdog function that resets the system every 10 seconds if there is not UART input.  

## Disclaimer

It should be noted that the MCP4725 does not handle warm resets, so it may become stuck to some power level. I've accounted for this in the code so that there is very little chance the watchdog function disrupts operation; however, if a manual warm reset is made and the peripheral fails, simply unplug it from power to properly reset it.  

### Resources
[Datasheet](https://ww1.microchip.com/downloads/aemDocuments/documents/APID/ProductDocuments/DataSheets/MCP4725-12-Bit-Digital-to-Analog-Converter-with-EEPROM-Memory-DS20002039.pdf)  
[Useful Example Code](https://github.com/gavinlyonsrepo/MCP4725_PICO)

