# Steps to Run simplified
1. Create build directory.
2. Navigate to build directory.
3. Run command "cmake -DPICO_BOARD=pico2 -DPICO_DEOPTIMIZED_DEBUG=1 -DCMAKE_BUILD_TYPE=Debug .."
4. Run command "make"
5. Drag uf2 file to RP2350.
6. Set Baud Rate to 115200 to communicate with device.
