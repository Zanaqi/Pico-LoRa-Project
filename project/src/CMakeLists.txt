if (NOT PICO_NO_HARDWARE)
    add_subdirectory(display_oled)
    add_subdirectory(lora_driver)
endif ()
