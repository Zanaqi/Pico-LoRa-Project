#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"

#include <stdio.h>

#define PIN_RECOVER 40 // Adjust this value as needed

// Define EBYTE modes
#define MODE_NORMAL 0
#define MODE_WAKEUP 1
#define MODE_POWERDOWN 2
#define MODE_PROGRAM 3

// Define pins and UART
#define UART_ID uart0
#define PIN_TX 0
#define PIN_RX 1
#define PIN_M0 3
#define PIN_M1 4
#define PIN_AUX 16

#define DEBOUNCE_50MS 50000 

typedef struct 
{
    uart_inst_t *uart;
    uint8_t pin_TX;
    uint8_t pin_RX;
    uint8_t pin_M0;
    uint8_t pin_M1;
    uint8_t pin_AUX;

    uint8_t _Save;
    uint8_t _AddressHigh;
    uint8_t _AddressLow;
    uint8_t _Speed;
    uint8_t _Channel;
    uint8_t _Options;

    uint16_t _Address;
    uint8_t _ParityBit;
    uint8_t _UARTDataRate;
    uint8_t _AirDataRate;

    uint8_t _OptionTrans;
    uint8_t _OptionPullup;
    uint8_t _OptionWakeup;
    uint8_t _OptionFEC;
    uint8_t _OptionPower;
}  EBYTE;

typedef struct 
{
    uint8_t value1;
    uint16_t value2;
} MyData;

EBYTE transceiver;

volatile bool uart_readable = 0;

static char event_str[128];

void gpio_event_string(char *buf, uint32_t events);

void ebyte_init(EBYTE *transceiver, uart_inst_t *uart, uint8_t pin_TX, uint8_t pin_RX, uint8_t pin_M0, uint8_t pin_M1, uint8_t pin_AUX) {
    transceiver->uart = uart;
    transceiver->pin_TX = pin_TX;
    transceiver->pin_RX = pin_RX;
    transceiver->pin_M0 = pin_M0;
    transceiver->pin_M1 = pin_M1;
    transceiver->pin_AUX = pin_AUX;

    uart_init(transceiver->uart, 9600);
    gpio_set_function(transceiver->pin_TX, GPIO_FUNC_UART);
    gpio_set_function(transceiver->pin_RX, GPIO_FUNC_UART);
    uart_set_hw_flow(transceiver->uart, false, false);
    uart_set_format(transceiver->uart, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(transceiver->uart, true);

    sleep_ms(10);

    gpio_init(transceiver->pin_AUX);
    gpio_set_dir(transceiver->pin_AUX, GPIO_IN);
    gpio_init(transceiver->pin_M0);
    gpio_init(transceiver->pin_M1);
    gpio_set_dir(transceiver->pin_M0, GPIO_OUT);
    gpio_set_dir(transceiver->pin_M1, GPIO_OUT);

    sleep_ms(10);
}

// bool ebyte_available(EBYTE *transceiver) {
//     return uart_is_readable(transceiver->uart);
// }

// void ebyte_flush(EBYTE *transceiver) {
//     while (uart_is_readable(transceiver->uart)) {
//         uart_getc(transceiver->uart);
//     }
// }

void ebyte_set_address_l(EBYTE *transceiver, uint8_t val) {
    transceiver->_AddressLow = val;
}


void ebyte_send_byte(EBYTE *transceiver, uint8_t data) {
    uart_putc(transceiver->uart, data);
    printf("%d\n", data);
}

uint8_t ebyte_get_byte(EBYTE *transceiver) {
    return uart_getc(transceiver->uart);
}

void ebyte_clear_buffer(EBYTE *transceiver) {
    unsigned long amt = time_us_64();

    while (uart_is_readable(transceiver->uart)) {
        uart_getc(transceiver->uart);
        if ((time_us_64() - amt) > 5000) {
            printf("runaway\n");
            break;
        }
    }
}

void ebyte_set_mode(EBYTE *transceiver, uint8_t mode) 
{
    if (mode == MODE_NORMAL) {
        gpio_put(transceiver->pin_M0, 0);
        gpio_put(transceiver->pin_M1, 0);
    } else if (mode == MODE_WAKEUP) {
        gpio_put(transceiver->pin_M0, 1);
        gpio_put(transceiver->pin_M1, 0);
    } else if (mode == MODE_POWERDOWN) {
        gpio_put(transceiver->pin_M0, 0);
        gpio_put(transceiver->pin_M1, 1);
    } else if (mode == MODE_PROGRAM) {
        gpio_put(transceiver->pin_M0, 1);
        gpio_put(transceiver->pin_M1, 1);
    }

    // sleep_ms(PIN_RECOVER);
    // ebyte_clear_buffer(transceiver);
}

bool ebyte_read_parameters(EBYTE *transceiver) {
    uint8_t params[6] = {0};

    ebyte_set_mode(transceiver, MODE_PROGRAM);

    uart_putc(transceiver->uart, 0xC1);
    uart_putc(transceiver->uart, 0xC1);
    uart_putc(transceiver->uart, 0xC1);

    uart_read_blocking(transceiver->uart, params, sizeof(params));

    // Extract parameters and update transceiver struct
    transceiver->_Save = params[0];
    transceiver->_AddressHigh = params[1];
    transceiver->_AddressLow = params[2];
    transceiver->_Speed = params[3];
    transceiver->_Channel = params[4];
    transceiver->_Options = params[5];

    printf("Save: 0x%02X\n", transceiver->_Save);
    printf("AddressHigh: 0x%02X\n", transceiver->_AddressHigh);
    printf("AddressLow: 0x%02X\n", transceiver->_AddressLow);
    printf("Speed: 0x%02X\n", transceiver->_Speed);
    printf("Channel: 0x%02X\n", transceiver->_Channel);
    printf("Options: 0x%02X\n", transceiver->_Options);

    transceiver->_Address = (transceiver->_AddressHigh << 8) | transceiver->_AddressLow;
    transceiver->_ParityBit = (transceiver->_Speed & 0xC0) >> 6;
    transceiver->_UARTDataRate = (transceiver->_Speed & 0x38) >> 3;
    transceiver->_AirDataRate = transceiver->_Speed & 0x07;

    transceiver->_OptionTrans = (transceiver->_Options & 0x80) >> 7;
    transceiver->_OptionPullup = (transceiver->_Options & 0x40) >> 6;
    transceiver->_OptionWakeup = (transceiver->_Options & 0x38) >> 3;
    transceiver->_OptionFEC = (transceiver->_Options & 0x07) >> 2;
    transceiver->_OptionPower = transceiver->_Options & 0x03;

    ebyte_set_mode(transceiver, MODE_NORMAL);

    return (0xC0 == params[0]);
}

void ebyte_save_parameters(EBYTE *transceiver) {
    uint8_t params[6] = {0};
    ebyte_set_mode(transceiver, MODE_PROGRAM);

    ebyte_clear_buffer(transceiver);

    ebyte_send_byte(transceiver, 0xC0);
    ebyte_send_byte(transceiver, transceiver->_AddressHigh);
    ebyte_send_byte(transceiver, transceiver->_AddressLow);
    ebyte_send_byte(transceiver, transceiver->_Speed);
    ebyte_send_byte(transceiver, transceiver->_Channel);
    ebyte_send_byte(transceiver, transceiver->_Options);

    sleep_ms(PIN_RECOVER);

    // uart_read_blocking(transceiver->uart, params, sizeof(params));

    // // Extract parameters and update transceiver struct
    // transceiver->_Save = params[0];
    // transceiver->_AddressHigh = params[1];
    // transceiver->_AddressLow = params[2];
    // transceiver->_Speed = params[3];
    // transceiver->_Channel = params[4];
    // transceiver->_Options = params[5];

    // printf("Save: 0x%02X\n", transceiver->_Save);
    // printf("AddressHigh: 0x%02X\n", transceiver->_AddressHigh);
    // printf("AddressLow: 0x%02X\n", transceiver->_AddressLow);
    // printf("Speed: 0x%02X\n", transceiver->_Speed);
    // printf("Channel: 0x%02X\n", transceiver->_Channel);
    // printf("Options: 0x%02X\n", transceiver->_Options);

    ebyte_set_mode(transceiver, MODE_NORMAL);
}

bool ebyte_send_struct(EBYTE *transceiver, const void *theStructure, uint16_t size_) 
{
    ebyte_send_byte(transceiver, 0xFF);
    ebyte_send_byte(transceiver, 0xFF);
    ebyte_send_byte(transceiver, 0x06);

    // Send each byte of the structure
    for (uint16_t i = 0; i < size_; i++) 
    {
        uint8_t data_byte = *((uint8_t *)theStructure + i);
        ebyte_send_byte(transceiver, data_byte);
    }

    return true;
}

bool ebyte_get_struct(EBYTE *transceiver, const void *theStructure, uint8_t size_) {
    uint8_t *buffer = (uint8_t *)theStructure;

    // Add a delay to allow the module to finish transmitting
    // sleep_ms(1000);

    // if (transceiver->ebyte_available(transceiver) < size_) {
    //     return false; // Not enough data available
    // }

    for (uint16_t i = 0; i < size_; i++) {
        buffer[i] = ebyte_get_byte(transceiver);
    }

    // Add another delay after reading to ensure the module is done
    // sleep_ms(1000);

    return true;
}

void gpio_callback(uint gpio, uint32_t events) 
{
    uint32_t current_time = time_us_32(); 
    static uint32_t start_button_last_time = 0;

    gpio_event_string(event_str, events);
    printf("GPIO %d %s\n", gpio, event_str);

    if (current_time - start_button_last_time >= DEBOUNCE_50MS)
    {
        if ((gpio_get(PIN_AUX))) 
        {
            const char *send_data = "Hello Pico, I Hate YOU";

            // MyData myData;
            // myData.value1 = 42;
            // myData.value2 = 1234;
            // ebyte_send_struct(&transceiver, &myData, sizeof(myData));

            printf("AUX before send: %d\n", gpio_get(PIN_AUX));
            printf("SENDING: %s\n", send_data);
            uart_write_blocking(UART_ID, send_data, sizeof(send_data));
            printf("AUX after send: %d\n", gpio_get(PIN_AUX));

            // printf("UART Readable: %d\n", (uart_is_readable_within_us(UART_ID, 5000)));
            
            // // Buffer to store received data
            // char buffer[22];
            // if (uart_is_readable_within_us(UART_ID, 5000))
            // {
            //     // Read a specified number of bytes (BLOCKING)
            //     uart_read_blocking(UART_ID, buffer, sizeof(buffer));
            // }

            // // Print the received data
            // printf("Received bytes: %s\n", buffer);
        }
        else 
        {
            printf("AUX pin is low\n");
        }
        start_button_last_time = current_time; 
    }

    // if (gpio == PIN_AUX && events == GPIO_IRQ_EDGE_FALL)
    // {
    //     printf("AUX pin is low\n");

    //     // MyData receivedData;
            
    //     // printf("AUX before receive: %d\n", gpio_get(PIN_AUX));

    //     // while (uart_is_readable_within_us(UART_ID, 5000)) 
    //     // {
    //     //     ebyte_get_struct(&transceiver, &receivedData, sizeof(receivedData));
    //     //     // Print received data
    //     //     printf("Received Data: Field1=%d, Field2=%d\n", 
    //     //         receivedData.value1, 
    //     //         receivedData.value2);
    //     // } 
    //     // printf("Receiving failed\n");
    // }
    // else if (gpio == PIN_AUX && GPIO_IRQ_LEVEL_HIGH)
    // {
        
    // }
}

int main() 
{
    stdio_init_all();

    // Create an instance of EBYTE
    ebyte_init(&transceiver, UART_ID, PIN_TX, PIN_RX, PIN_M0, PIN_M1, PIN_AUX);
    sleep_ms(5);

    ebyte_set_address_l(&transceiver, 0x01);
    ebyte_save_parameters(&transceiver);
    sleep_ms(5);

    ebyte_set_mode(&transceiver, MODE_POWERDOWN);
    sleep_ms(1);

    gpio_init(20);
    gpio_set_dir(20, GPIO_IN);

    // gpio_set_irq_enabled_with_callback(20, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    while (1)
    {
        // Your main program logic here
        // ebyte_read_parameters(&transceiver);

        printf("%d\n", gpio_get(PIN_AUX));
        // printf("M0: %d, M1: %d\n", gpio_get(PIN_M0), gpio_get(PIN_M1));
        if (!gpio_get(PIN_AUX))
        {
            sleep_ms(5);
            // printf("%d\n", (uart_is_readable_within_us(UART_ID, 5000)));
        }
        
        // // Buffer to store received data
        // char buffer;

        // while (uart_is_readable_within_us(UART_ID, 5000))
        // {
        //     printf("READABLE\n");
        //     // Read a specified number of bytes (BLOCKING)
        //     buffer = uart_getc(UART_ID);
        // }

        // // Print the received data
        // printf("Received bytes: %s\n", buffer);
    }
    return 0;
}

static const char *gpio_irq_str[] = {
        "LEVEL_LOW",  // 0x1
        "LEVEL_HIGH", // 0x2
        "EDGE_FALL",  // 0x4
        "EDGE_RISE"   // 0x8
};

void gpio_event_string(char *buf, uint32_t events) {
    for (uint i = 0; i < 4; i++) {
        uint mask = (1 << i);
        if (events & mask) {
            // Copy this event string into the user string
            const char *event_str = gpio_irq_str[i];
            while (*event_str != '\0') {
                *buf++ = *event_str++;
            }
            events &= ~mask;

            // If more events add ", "
            if (events) {
                *buf++ = ',';
                *buf++ = ' ';
            }
        }
    }
    *buf++ = '\0';
}
