cmake_minimum_required(VERSION 3.13)

# initialize the SDK directly
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(my_project)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project
add_executable(m4DAC1
    m4DAC1.c
    circular_buffer.c    
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(m4DAC1 pico_stdlib hardware_i2c)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(m4DAC1 pico_stdlib)
pico_enable_stdio_usb(m4DAC1 1)
pico_enable_stdio_uart(m4DAC1 0)

function(pico_add_dis_output2 TARGET)
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_OBJDUMP} -S $<TARGET_FILE:${TARGET}> >$<IF:$<BOOL:$<TARGET_PROPERTY:${TARGET},OUTPUT_NAME>>,$<TARGET_PROPERTY:${TARGET},OUTPUT_NAME>,$<TARGET_PROPERTY:${TARGET},NAME>>.dis2)

    if (PICO_COMPILER STREQUAL "pico_arm_gcc")
        pico_find_compiler(PICO_COMPILER_SIZE ${PICO_GCC_TRIPLE}-size)
        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND ${PICO_COMPILER_SIZE} ${CMAKE_CURRENT_LIST_DIR}/../build/src/$<IF:$<BOOL:$<TARGET_PROPERTY:${TARGET},OUTPUT_NAME>>,$<TARGET_PROPERTY:${TARGET},OUTPUT_NAME>,$<TARGET_PROPERTY:${TARGET},NAME>>.elf
            VERBATIM
        )
    endif()
endfunction()

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(m4DAC1)

# also create additional disassembled file
pico_add_dis_output2(m4DAC1)

