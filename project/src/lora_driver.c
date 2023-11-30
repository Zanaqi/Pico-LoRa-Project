#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/i2c.h"

// OLED Library
#include "ssd1306.h"

// Define the UART ID and GPIO pins
#define UART_ID uart0
#define I2C_ID i2c1
#define TX_PIN 0 // Connect to RX on EBYTE module
#define RX_PIN 1 // Connect to TX on EBYTE module
#define M0_PIN 4
#define M1_PIN 5
#define AUX_PIN 16
#define SDA_PIN 6
#define SCL_PIN 7

// Define buttons
#define BROADCAST_BTN_PIN 20
#define SEND_MODULE_1_BTN_PIN 21
#define SEND_MODULE_2_BTN_PIN 22

#define SAVE_CONFIG 0xC0 // Save configurations even after module power down

// Define module modes
#define NORMAL_MODE 0
#define WAKEUP_MODE 1
#define POWERSAVING_MODE 2
#define SLEEP_MODE 3

// Define char limits for OLED
#define CHAR_LIMIT_X 120
#define CHAR_LIMIT_Y 54

// Define baud rates
#define BAUD_RATE 9600
#define OLED_BAUD_RATE 400000

#define DEBOUNCE_50MS 50000
#define PIN_RECOVER 1000

/**
*   Define node addresses
*   Byte format: { SAVE_CONFIG, high address, low address, speed, channel, options }
*/
const uint8_t BROADCAST_CONFIG[] = { SAVE_CONFIG, 0xFF, 0xFF, 0x3A, 0x04, 0xC4 };
const uint8_t NODE1_CONFIG[] = { SAVE_CONFIG, 0x00, 0x01, 0x1A, 0x02, 0xC4 };
const uint8_t NODE2_CONFIG[] = { SAVE_CONFIG, 0x00, 0x02, 0x1A, 0x04, 0xC4 };
const uint8_t NODE3_CONFIG[] = { SAVE_CONFIG, 0x00, 0x03, 0x1A, 0x06, 0xC4 };
const uint8_t NODE4_CONFIG[] = { SAVE_CONFIG, 0x00, 0x04, 0x1A, 0x06, 0xC4 };
const uint8_t NODE5_CONFIG[] = { SAVE_CONFIG, 0x00, 0x05, 0x1A, 0x06, 0xC4 };

char combined_string[50];
volatile int counter = 0;

int xCursor = 0;
int yCursor = 6;

// Structure for OLED display configuration
ssd1306_t disp;

// Clears the modules buffer
void flush_buffer()
{
    stdio_flush();
    unsigned long amt = time_us_64();

    while (uart_is_readable(UART_ID))
    {
        uart_getc(UART_ID);
        if ((time_us_64() - amt) > 5000) 
        {
            printf("runaway\n");
            break;
        }
    }
}

/**
*   @brief Changes the mode of the EBYTE module
*   @param mode Input which mode to set the module (NORMAL_MODE, WAKEUP_MODE, POWERSAVING_MODE, SLEEP_MODE)
*/
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

// Gets the current configuration of the EBYTE module and prints it out
void get_current_config()
{
    flush_buffer();
    sleep_ms(PIN_RECOVER);

    uint8_t rxbuffer[6];

    // Sends 0xC1 three times in SLEEP_MODE
    unsigned char hex = 0xC1;
    uint8_t hexcode[] = {hex, hex, hex}; 
    uart_write_blocking(UART_ID, hexcode, sizeof(hexcode));

    // Module automatically sends back current configurations
    uart_read_blocking(UART_ID, rxbuffer, 6);
    for (int i = 0; i < sizeof(rxbuffer) / sizeof(uint8_t); i++)
    {
        printf("0x%02X ", rxbuffer[i]);
    }
    printf("\n");
}

/**
 * @brief Set the configuration of the EBYTE module
 * @param hexArr Set of parameters used to change the configurations
 */
void set_config(uint8_t hexArr[])
{
    flush_buffer();
    sleep_ms(PIN_RECOVER);

    uint8_t rxbuffer[6];

    uart_write_blocking(UART_ID, hexArr, 6);

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

// Send messages to be transmitted
void send_msg(uint gpio, uint32_t events)
{
    flush_buffer();
    uint32_t current_time = time_us_32();
    static uint32_t button_last_time = 0;

    if (gpio == BROADCAST_BTN_PIN && events == GPIO_IRQ_EDGE_RISE)
    {
        if (current_time - button_last_time >= DEBOUNCE_50MS)
        {
            // AUX pin high = Buffer is empty
            // If empty, we allow sending
            if (gpio_get(AUX_PIN))
            {
                // Transmission mode takes in first three bytes to direct transmit to
                unsigned char addhigh, addlow, channel;
                // FFFF in this case broadcasts the message to all devices in the provided channel
                addhigh = 0xFF;
                addlow = 0XFF; 
                channel = 0x04;

                char msg_to_send[] = "Hello, everyone!";
                uint8_t bytes_to_send[] = {addhigh, addlow, channel};
                size_t bytes_len = sizeof(bytes_to_send);
                size_t msg_len = sizeof(msg_to_send);

                // Create a new combined array with enough space for both arrays
                uint8_t combined_array[bytes_len + msg_len];

                // Copy bytes_to_send into combined_array
                memcpy(combined_array, bytes_to_send, bytes_len);

                // Convert characters from msg_to_send to uint8_t and append them to combined_array
                for (size_t i = 0; i < msg_len; i++)
                {
                    combined_array[bytes_len + i] = (uint8_t)msg_to_send[i];
                }
                uart_write_blocking(UART_ID, combined_array, sizeof(combined_array));
            }
            button_last_time = current_time;
        }
    }

    if (gpio == SEND_MODULE_1_BTN_PIN && events == GPIO_IRQ_EDGE_RISE)
    {
        if (current_time - button_last_time >= DEBOUNCE_50MS)
        {
            if (gpio_get(AUX_PIN))
            {
                unsigned char addhigh, addlow, channel;
                addhigh = 0x00;
                addlow = 0X02;
                channel = 0x04;
                char msg_to_send[] = "Hello, Node 2!";
                uint8_t bytes_to_send[] = {addhigh, addlow, channel};
                size_t bytes_len = sizeof(bytes_to_send);
                size_t msg_len = sizeof(msg_to_send);

                // Create a new combined array with enough space for both arrays
                uint8_t combined_array[bytes_len + msg_len];

                // Copy bytes_to_send into combined_array
                memcpy(combined_array, bytes_to_send, bytes_len);

                // Convert characters from msg_to_send to uint8_t and append them to combined_array
                for (size_t i = 0; i < msg_len; i++)
                {
                    combined_array[bytes_len + i] = (uint8_t)msg_to_send[i];
                }
                uart_write_blocking(UART_ID, combined_array, sizeof(combined_array));
            }
            button_last_time = current_time;
        }
    }

    if (gpio == SEND_MODULE_2_BTN_PIN && events == GPIO_IRQ_EDGE_RISE)
    {
        if (current_time - button_last_time >= DEBOUNCE_50MS)
        {
            if (gpio_get(AUX_PIN))
            {
                unsigned char addhigh, addlow, channel;
                addhigh = 0x00;
                addlow = 0X01;
                channel = 0x02;
                char msg_to_send[] = "Hello, Node 1!";
                uint8_t bytes_to_send[] = {addhigh, addlow, channel};
                size_t bytes_len = sizeof(bytes_to_send);
                size_t msg_len = sizeof(msg_to_send);

                // Create a new combined array with enough space for both arrays
                uint8_t combined_array[bytes_len + msg_len];

                // Copy bytes_to_send into combined_array
                memcpy(combined_array, bytes_to_send, bytes_len);

                // Convert characters from msg_to_send to uint8_t and append them to combined_array
                for (size_t i = 0; i < msg_len; i++)
                {
                    combined_array[bytes_len + i] = (uint8_t)msg_to_send[i];
                }
                uart_write_blocking(UART_ID, combined_array, sizeof(combined_array));
            }
            button_last_time = current_time;
        }
    }
}

// Draws line separators on OLED display to split into 4 rows
void draw_separators()
{
    ssd1306_draw_line(&disp, 0, 15, 127, 15);
    ssd1306_draw_line(&disp, 0, 31, 127, 31);
    ssd1306_draw_line(&disp, 0, 47, 127, 47);
    ssd1306_draw_line(&disp, 0, 63, 127, 63);
}

// Print data received from the RX_PIN
void print_on_oled(char rxchar)
{
    if(xCursor!=CHAR_LIMIT_X)
    {
        ssd1306_draw_char(&disp, xCursor, yCursor, 1, rxchar);
        ssd1306_show(&disp);
        xCursor += 8;
    }
    else if(xCursor==CHAR_LIMIT_X && yCursor!=CHAR_LIMIT_Y)
    {
        yCursor += 16;
        xCursor = 0;
        ssd1306_draw_char(&disp, xCursor, yCursor, 1, rxchar);
        ssd1306_show(&disp);
    }
    else
    {
        xCursor = 0;
        yCursor = 6;
        ssd1306_clear(&disp);
        draw_separators();
        ssd1306_draw_char(&disp, xCursor, yCursor, 1, rxchar);
        ssd1306_show(&disp);
        xCursor += 8;
    }
}

void printCombinedString()
{
    printf("%s\n", combined_string);
}

void receive_msg_hex()
{   
    if (uart_is_readable(UART_ID))
    {
        char rxchar = uart_getc(UART_ID);
        combined_string[counter] = rxchar;
        print_on_oled(rxchar);
        counter += 1;

        if (rxchar == '\n' || rxchar == '\0')
        {
            counter = 0;
            printCombinedString();
        }
    }
}

void init_config()
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

    // Initializing buttons
    gpio_init(BROADCAST_BTN_PIN);
    gpio_set_dir(BROADCAST_BTN_PIN, GPIO_IN);
    gpio_init(SEND_MODULE_1_BTN_PIN);
    gpio_set_dir(SEND_MODULE_1_BTN_PIN, GPIO_IN);
    gpio_init(SEND_MODULE_2_BTN_PIN);
    gpio_set_dir(SEND_MODULE_2_BTN_PIN, GPIO_IN);

    // Initialize I2C for OLED
    i2c_init(I2C_ID, OLED_BAUD_RATE);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);

    const char configMsg[] = "CONFIG DONE";

    disp.external_vcc=false;
    ssd1306_init(&disp, 128, 64, 0x3C, I2C_ID);
    ssd1306_clear(&disp);

    ssd1306_draw_string(&disp, 30, 32, 1, configMsg);
    ssd1306_show(&disp);
    ssd1306_clear(&disp);
}

int main()
{
    init_config();

    change_mode(SLEEP_MODE);
    get_current_config();
    set_config(NODE2_CONFIG);
    change_mode(NORMAL_MODE);
    flush_buffer();
    draw_separators();
    ssd1306_show(&disp);

    gpio_set_irq_enabled_with_callback(BROADCAST_BTN_PIN, GPIO_IRQ_EDGE_RISE, true, &send_msg);
    gpio_set_irq_enabled_with_callback(SEND_MODULE_1_BTN_PIN, GPIO_IRQ_EDGE_RISE, true, &send_msg);
    gpio_set_irq_enabled_with_callback(SEND_MODULE_2_BTN_PIN, GPIO_IRQ_EDGE_RISE, true, &send_msg);
    
    while (1)
    {
        receive_msg_hex();
    }

    return 0;
}