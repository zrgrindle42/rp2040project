cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_PATH "/Users/elizabethgodsey/pico-sdk")
#set(PICO_BOOT_STAGE2_NONE 1)

include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(rp2040project C CXX ASM)

pico_sdk_init()

# FreeRTOS config interface
add_library(freertos_config INTERFACE)
target_include_directories(freertos_config SYSTEM
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

set(FREERTOS_PORT GCC_RP2040 CACHE STRING "")
set(FREERTOS_HEAP 4 CACHE STRING "")

set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/freertos)
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

add_subdirectory(freertos)

add_executable(rp2040project src/main.c
			      src/i2c.c
			      src/adc.c
			      src/lib/RP2040-LCD-1.28/c/lib/LCD/LCD_1in28.c
			      src/lib/RP2040-LCD-1.28/c/lib/GUI/GUI_PAINT.c
				
			      src/lib/RP2040-LCD-1.28/c/lib/Config/DEV_Config.c

			      src/lib/RP2040-LCD-1.28/c/lib/Fonts/font8.c
			      src/lib/RP2040-LCD-1.28/c/lib/Fonts/font12.c
			      src/lib/RP2040-LCD-1.28/c/lib/Fonts/font16.c
			      src/lib/RP2040-LCD-1.28/c/lib/Fonts/font20.c
			      src/lib/RP2040-LCD-1.28/c/lib/Fonts/font24.c)


target_include_directories(rp2040project PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/freertos/include
    ${CMAKE_CURRENT_SOURCE_DIR}/freertos/portable/ThirdParty/GCC/RP2040/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/RP2040-LCD-1.28/c/lib/LCD
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/RP2040-LCD-1.28/c/lib/GUI
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/RP2040-LCD-1.28/c/lib/Fonts
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/RP2040-LCD-1.28/c/lib/Config
)

target_link_libraries(rp2040project
    PRIVATE
    pico_stdlib
    freertos_config          # <-- was missing
    FreeRTOS-Kernel
    FreeRTOS-Kernel-Heap4
    pico_malloc
    hardware_i2c
    hardware_spi
    hardware_adc
    hardware_dma
    hardware_irq
    hardware_pwm
)

pico_enable_stdio_usb(rp2040project 1)
pico_enable_stdio_uart(rp2040project 0)
pico_add_extra_outputs(rp2040project)