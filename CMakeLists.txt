cmake_minimum_required(VERSION 3.13)

# Set the Pico SDK path directly
set(PICO_SDK_PATH "C:/Users/zach/Desktop/pico-sdk")

# Disable boot_stage2 (so Python is not required)
set(PICO_BOOT_STAGE2_NONE 1)

# Import the Pico SDK
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(rp2040project C CXX ASM)

# Initialize the Pico SDK
pico_sdk_init()

# Add your executable
add_executable(rp2040project src/main.c)

# Link the Pico standard library
target_link_libraries(rp2040project pico_stdlib)

# Enable stdio over USB
pico_enable_stdio_usb(rp2040project 1)
pico_enable_stdio_uart(rp2040project 0)

# Create UF2 and other outputs for flashing
pico_add_extra_outputs(rp2040project)