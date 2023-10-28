#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/uart.h"

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"

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

// Define task priorities
#define RECEIVE_TASK_PRIORITY 1
#define SEND_TASK_PRIORITY 2
#define CHECKAUX_TASK_PRIORITY 3

#define BAUD_RATE 9600

// Data to send
const char data_to_send[] = "Hello, Pico!";

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

// Task to check AUX output
void check_aux_task(void *pvParameters)
{
    while (1)
    {
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
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Task function for sending data
void send_task(void *pvParameters) 
{
    while (1) 
    {
        printf("Sending: %s\n", data_to_send);
        uart_puts(UART_ID, data_to_send);
        vTaskDelay(pdMS_TO_TICKS(1000)); // Send data every 1 second
    }
}

// Task function for receiving data
void receive_task(void *pvParameters) {
    char received_data[50];

    while (1) 
    {
        uart_read_blocking(UART_ID, received_data, sizeof(received_data));
        received_data[sizeof(received_data) - 1] = '\0';

        // Process and handle received data
        printf("Received: %s\n", received_data);
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
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);

    gpio_init(M0_PIN);
    gpio_set_dir(M0_PIN, GPIO_OUT);

    gpio_init(M1_PIN);
    gpio_set_dir(M1_PIN, GPIO_OUT);

    // Setting module in normal mode
    change_mode(NORMAL_MODE);

    // AUX pin on the module is an output pin so we set GPIO as input
    gpio_init(AUX_PIN);
    gpio_set_dir(AUX_PIN, GPIO_IN);

    // Create tasks
    xTaskCreate(send_task, "Send Task", configMINIMAL_STACK_SIZE, NULL, SEND_TASK_PRIORITY, NULL);
    xTaskCreate(receive_task, "Receive Task", configMINIMAL_STACK_SIZE, NULL, RECEIVE_TASK_PRIORITY, NULL);
    xTaskCreate(check_aux_task, "Check AUX Task", configMINIMAL_STACK_SIZE, NULL, CHECKAUX_TASK_PRIORITY, NULL);

    // Start the FreeRTOS scheduler
    vTaskStartScheduler();

    while (1) 
    {
        tight_loop_contents();
    }

    return 0;
}