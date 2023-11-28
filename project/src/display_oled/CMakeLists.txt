# Start of lora_driver
add_executable(display_oled
    display_oled.c 
    ../../ssd1306.c
)

target_include_directories(display_oled PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../../
)

target_link_libraries(display_oled
    pico_stdlib 
    hardware_i2c 
)

# Enables outputs on the serial monitor
pico_enable_stdio_usb(display_oled 1)

pico_add_extra_outputs(display_oled)
# End of lora_driver