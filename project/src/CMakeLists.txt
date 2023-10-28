add_executable(${ProjectName}
    lora_driver.c
)

target_include_directories(${ProjectName} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(${ProjectName} 
    pico_stdlib 
    FreeRTOS-Kernel-Heap4 
)

# Enables outputs on the serial monitor
pico_enable_stdio_usb(${ProjectName} 1)

pico_add_extra_outputs(${ProjectName})