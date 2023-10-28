#include "pico/stdlib.h"
#include "hardware/uart.h"
#include <stdio.h>

// Define the UART ID and GPIO pins
#define UART_ID uart0
#define TX_PIN 0    // Connect to RX on EBYTE module
#define RX_PIN 1    // Connect to TX on EBYTE module
#define AUX_PIN 16

// Define the GPIO pins for M0 and M1
#define M0_PIN 3
#define M1_PIN 4

// Define module modes
#define NORMAL_MODE 0
#define WAKEUP_MODE 1
#define POWERSAVING_MODE 2
#define SLEEP_MODE 3

#define BAUD_RATE 9600

// Function to change the mode of the module
void change_mode(int mode) 
{
    switch (mode)
    {
        case 0:
            // Set M0 and M1 pins to LOW
            gpio_put(M0_PIN, 0);
            gpio_put(M1_PIN, 0);
            break;
        case 1:
            // Set M0 pin to HIGH and M1 pin to LOW
            gpio_put(M0_PIN, 1);
            gpio_put(M1_PIN, 0);
            break;
        case 2:
            // Set M0 pin to LOW and M1 pin to HIGH
            gpio_put(M0_PIN, 0);
            gpio_put(M1_PIN, 1);
            break;
        case 3:
            // Set M0 and M1 pins to HIGH
            gpio_put(M0_PIN, 1);
            gpio_put(M1_PIN, 1);
            break;
        default:
            // Invalid mode, do nothing
            break;
    }
}

int main() 
{
    stdio_init_all();

    // Initialize UART with desired settings
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(RX_PIN, GPIO_FUNC_UART);
    uart_set_hw_flow(UART_ID, false, false);
    
    // Disable FIFO
    uart_set_fifo_enabled(UART_ID, false);

    gpio_init(M0_PIN);
    gpio_set_dir(M0_PIN, GPIO_OUT);

    gpio_init(M1_PIN);
    gpio_set_dir(M1_PIN, GPIO_OUT);

    // AUX pin on the module is an output pin so we set GPIO as input
    gpio_init(AUX_PIN);
    gpio_set_dir(AUX_PIN, GPIO_IN);

    // Setting module in normal mode
    change_mode(NORMAL_MODE);

    while (1) 
    {
        // Send data to the E32 module
        uart_puts(UART_ID, "Hello, E32!\r\n");

        // Read the state of the AUX pin
        bool aux_state = gpio_get(AUX_PIN);

        /*
            AUX = high (1) - no data in the buffer
            AUX = low (0)  - data in the buffer
        */
        if (aux_state) 
        {
            printf("AUX pin is high\n");
        } 
        else 
        {
            printf("AUX pin is low\n");
        }

        // Wait for a short time before sending more data
        sleep_ms(1000);
    }

    return 0;
}