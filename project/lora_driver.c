#include "pico/stdlib.h"
#include "hardware/uart.h"

// Define the UART ID and GPIO pins
#define UART_ID uart0
#define TX_PIN 0
#define RX_PIN 1
#define AUX_PIN 2

// Define the GPIO pins for M0 and M1
#define M0_PIN 3
#define M1_PIN 4

// Function to set M0 and M1 pins to enter/exit parameter setting mode (sleep mode)
void set_parameter_setting_mode(bool enable) 
{
    gpio_put(M0_PIN, enable ? 1 : 0);
    gpio_put(M1_PIN, enable ? 1 : 0);

    // Wait for the module to enter sleep mode
    sleep_ms(1000);
}

// Function to send AT commands over UART
void uart_send_at_command(const char *command) 
{
    uart_puts(UART_ID, command);
    uart_puts(UART_ID, "\r\n");  // Send carriage return and line feed for proper command termination
}

// Function to configure the E32-900T30D
void configure_module() 
{
    // Set M0 and M1 pins to enter configuration mode
    set_parameter_setting_mode(true);

    // Set Baud Rate to 9600 bps
    uart_send_at_command("AT+UART=8,E,1,9600");

    // Set Address
    uart_send_at_command("AT+ADDR=01");

    // Set Frequency (adjust values as needed)
    uart_send_at_command("AT+CFG=433.5,10,6,1,0,0");

    // Set Transmission Mode to Fixed
    uart_send_at_command("AT+TMOD=0");

    // Save Configurations
    uart_send_at_command("AT+SAVE");

    // Set M0 and M1 pins to exit configuration mode
    set_parameter_setting_mode(false);
}

// Function to send data over UART
void uart_send_data(const char *data) 
{
    uart_puts(UART_ID, data);
}

int main() 
{
    stdio_init_all();

    // Initialize the UART interface
    uart_init(UART_ID, 9600);
    gpio_set_function(TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(RX_PIN, GPIO_FUNC_UART);

    // gpio_set_dir(AUX_PIN, GPIO_IN);
    // gpio_set_pulls(BTN_PIN, true, false);

    // Configure the module
    configure_module();

    while (true) 
    {
        // Send "Hello, World"
        uart_send_data("Hello, World\n");

        sleep_ms(1000);  // Adjust the delay as needed
    }

    return 0;
}
