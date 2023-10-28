cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_BOARD pico_w)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

# Initialize the SDK
pico_sdk_init()

add_executable(lora_driver lora_driver.c)

# pull in common dependencies
target_link_libraries(lora_driver 
                        pico_stdlib
                        pico_cyw43_arch_none
                    )

# Enables outputs on the serial monitor
pico_enable_stdio_usb(lora_driver 1)

# Create map/bin/hex file etc.
pico_add_extra_outputs(lora_driver)
