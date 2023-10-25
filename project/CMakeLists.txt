add_executable(lora_driver lora_driver.c)

# pull in common dependencies
target_link_libraries(lora_driver pico_stdlib)

# Enables outputs on the serial monitor
pico_enable_stdio_usb(lora_driver 1)

# Create map/bin/hex file etc.
pico_add_extra_outputs(lora_driver)

# Add url via pico_set_program_url
example_auto_set_url(lora_driver)

