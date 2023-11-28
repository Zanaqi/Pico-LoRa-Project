#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/i2c.h"

#include "ssd1306.h"

// Define the UART ID and GPIO pins
#define OLED_UART_ID uart1
#define OLED_RX_PIN 9
#define SDA_PIN 6
#define SCL_PIN 7

#define BAUD_RATE 115200
#define OLED_BAUD_RATE 400000

#define CHAR_LIMIT_X 120
#define CHAR_LIMIT_Y 54

#define SCREEN_COLUMNS 4
#define MAX_STRING_LENGTH 58

void draw_seperators(ssd1306_t *disp)
{
    ssd1306_draw_line(disp, 0, 15, 127, 15);
    ssd1306_draw_line(disp, 0, 31, 127, 31);
    ssd1306_draw_line(disp, 0, 47, 127, 47);
    ssd1306_draw_line(disp, 0, 63, 127, 63);
}

int main()
{
    stdio_init_all();

    // Initialize UART with desired settings
    uart_init(OLED_UART_ID, BAUD_RATE);
    gpio_set_function(OLED_RX_PIN, GPIO_FUNC_UART);
    uart_set_hw_flow(OLED_UART_ID, false, false);
    uart_set_format(OLED_UART_ID, 8, 1, UART_PARITY_NONE);

    // Initialize I2C for OLED
    i2c_init(i2c1, OLED_BAUD_RATE);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);

    const char configMsg[] = "CONFIG DONE";

    int xCursor = 0;
    int yCursor = 6;

    ssd1306_t disp;
    disp.external_vcc=false;
    ssd1306_init(&disp, 128, 64, 0x3C, i2c1);
    ssd1306_clear(&disp);

    ssd1306_draw_string(&disp, 30, 32, 1, configMsg);
    ssd1306_show(&disp);
    sleep_ms(3000);
    ssd1306_clear(&disp);

    draw_seperators(&disp);
    ssd1306_show(&disp);

    while (1)
    {
        if(uart_is_readable(OLED_UART_ID))
        {
            uint8_t rxchar = uart_getc(OLED_UART_ID);

            //uart_read_blocking(OLED_UART_ID, charData, 1);

            printf("%c\n", rxchar);
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
                draw_seperators(&disp);
                ssd1306_draw_char(&disp, xCursor, yCursor, 1, rxchar);
                ssd1306_show(&disp);
                xCursor += 8;
            }
        }
    }

    return 0;
}