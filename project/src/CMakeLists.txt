# Start of lora_driver
add_executable(lora_driver
    lora_driver.c 
    ../ssd1306.c
)

target_include_directories(lora_driver PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../
)

target_link_libraries(lora_driver 
    pico_stdlib 
    hardware_i2c 
)

# Enables outputs on the serial monitor
pico_enable_stdio_usb(lora_driver 1)

pico_add_extra_outputs(lora_driver)
# End of lora_driver