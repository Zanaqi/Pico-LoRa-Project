#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/uart.h"

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"

// Define the UART ID and GPIO pins
#define UART_ID uart0
#define TX_PIN 0 // Connect to RX on EBYTE module
#define RX_PIN 1 // Connect to TX on EBYTE module
#define M0_PIN 3
#define M1_PIN 4
#define AUX_PIN 16
#define BTN_PIN 20

// Define module modes
#define NORMAL_MODE 0
#define WAKEUP_MODE 1
#define POWERSAVING_MODE 2
#define SLEEP_MODE 3

#define BAUD_RATE 9600

#define DEBOUNCE_50MS 50000 

void flush_buffer()
{
    stdio_flush();
    unsigned long amt = time_us_64();

    while (uart_is_readable(UART_ID))
    {
        uart_getc(UART_ID);
        if ((time_us_64() - amt) > 5000) {
            printf("runaway\n");
            break;
        }
    }
}

// Function to change the mode of the module
void change_mode(int mode)
{
    flush_buffer();
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

void get_current_config()
{
    flush_buffer();

    uint8_t rxbuffer[6];
    unsigned char hex = 0xC1;
    uint8_t hexcode[] = {hex, hex, hex};

    uart_write_blocking(UART_ID, hexcode, sizeof(hexcode));

    uart_read_blocking(UART_ID, rxbuffer, 6);
    for (int i = 0; i < sizeof(rxbuffer) / sizeof(uint8_t); i++)
    {
        printf("0x%02X ", rxbuffer[i]);
    }
    printf("\n");
}

void set_config()
{
    flush_buffer();

    unsigned char head, addhigh, addlow, speed, channel, option;
    uint8_t rxbuffer[6];

    head = 0xC0;
    addhigh = 0xFF;
    addlow = 0xFF;
    speed = 0x1A;
    channel = 0x06;
    option = 0x40;
    uint8_t config_hex[] = {head, addhigh, addlow, speed, channel, option};

    uart_write_blocking(UART_ID, config_hex, sizeof(config_hex));

    uart_read_blocking(UART_ID, rxbuffer, 6);
    for (int i = 0; i < sizeof(rxbuffer) / sizeof(uint8_t); i++)
    {
        printf("0x%02X ", rxbuffer[i]);
    }
    printf("\n");
}

void reset_config()
{
    flush_buffer();

    uint8_t rxbuffer[6];
    unsigned char hex = 0xC4;
    uint8_t hexcode[] = {hex, hex, hex};

    uart_write_blocking(UART_ID, hexcode, sizeof(hexcode));
}

// Task to check AUX output
void check_aux_task()
{
    // Read the state of the AUX pin
    bool aux_state = gpio_get(AUX_PIN);

    /*
        AUX = high (1) - no data in the buffer
        AUX = low (0)  - data in the buffer
    */
    if (aux_state)
    {
        printf("AUX pin is high: %d\n", aux_state);
    }
    else
    {
        printf("AUX pin is low: %d\n", aux_state);
    }
}

// Broadcast Transmission
void broadcast_msg()
{
    uint32_t current_time = time_us_32(); 
    static uint32_t button_last_time = 0;
    if (current_time - button_last_time >= DEBOUNCE_50MS)
    {
        if (gpio_get(AUX_PIN))
        {
            unsigned char addhigh, addlow, channel;
            // addhigh = 0xAA;
            // addlow = 0xBB;
            // channel = 0xCC;
            // addhigh = 0xFF;
            // addlow = 0xFF;
            // channel = 0x06;
            // uint8_t hexcode[] = {addhigh, addlow, channel, 0x00, 0xAA, 0xBB, 0xCC};
            char msg[] = "I love embedded programming very very much indeed";
            uart_write_blocking(UART_ID, msg, sizeof(msg));
            // uart_puts(UART_ID, hexcode);

            // uart_write_blocking(UART_ID, hexcode, sizeof(hexcode));
        }
        button_last_time = current_time;
    }
}

void receive_msg_hex()
{
    uint8_t rxbuffer[1];
    uart_read_blocking(UART_ID, rxbuffer, 1);
    printf("%s", rxbuffer);
    // for (int i = 0; i < sizeof(rxbuffer) / sizeof(uint8_t); i++)
    // {
    //     printf("%d ", rxbuffer[i]);
    // }
    // if (uart_is_readable_within_us(UART_ID, 5000))
    // {
    //     uint8_t rxchar;
    //     rxchar = uart_getc(UART_ID);
    //     printf("0x%02X ", rxchar);
    // }
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

    // AUX pin on the module is an output pin so we set GPIO as input
    gpio_init(AUX_PIN);
    gpio_set_dir(AUX_PIN, GPIO_IN);

    // Initializing a button at GP20
    gpio_init(BTN_PIN);
    gpio_set_dir(BTN_PIN, GPIO_IN);

    change_mode(SLEEP_MODE);
    sleep_ms(5000);
    get_current_config();
    sleep_ms(5000);
    set_config();   

    sleep_ms(5000);
    flush_buffer();
    change_mode(NORMAL_MODE);
    sleep_ms(2);

    // broadcast_msg();

    // gpio_set_irq_enabled_with_callback(BTN_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &broadcast_msg);

    while (1)
    {
        receive_msg_hex();
    }

    return 0;
}