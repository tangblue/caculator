/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <u8g2.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "qrcode.h"
#include "sdkconfig.h"
#include "u8g2_esp32_hal.h"

/* Can run 'make menuconfig' to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO CONFIG_BLINK_GPIO

#define PIN_SDA     4
#define PIN_SCL     15
#define PIN_RESET   16

esp_err_t example_configure_stdin_stdout()
{
    // Initialize VFS & UART so we can use std::cout/cin
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK( uart_driver_install( (uart_port_t)CONFIG_CONSOLE_UART_NUM,
            256, 0, 0, NULL, 0) );
    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONFIG_CONSOLE_UART_NUM);
    esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);
    return ESP_OK;
}

bool isNum(int c) {
    return c >= '0' && c <= '9';
}
bool isOp(int c) {
    return c == '+' || c == '-' || c == '*' || c == '/';
}
int doOp(int num1, int num2, char op) {
    int ret = 0;

    switch (op) {
    case '+':
        ret = num1 + num2;
        break;
    case '-':
        ret = num1 - num2;
        break;
    case '*':
        ret = num1 * num2;
        break;
    case '/':
        if (num2 == 0) {
            ret = 0;
        } else {
            ret = num1 / num2;
        }
        break;
    }

    return ret;
}

void draw(u8g2_t *u8g2, int num1, int num2, char op, int result) {
    char buf[32];

    u8g2_ClearBuffer(u8g2);
    if (op != 0) {
        snprintf(buf, sizeof(buf), "%d%c%d", num1, op, num2);
    } else {
        snprintf(buf, sizeof(buf), "%d", num2);
    }
    u8g2_SetFont(u8g2, u8g2_font_ncenB08_tr);
    u8g2_DrawStr(u8g2, 2,10,buf);
    printf("%s\n", buf);

    if (result != 0) {
        snprintf(buf, sizeof(buf), "%d", result);
        u8g2_DrawStr(u8g2, 2,20,buf);
        printf("result: %s\n", buf);

        int version = 1;
        QRCode qrcode;
        uint8_t qrcodeData[qrcode_getBufferSize(version)];
        qrcode_initText(&qrcode, qrcodeData, version, ECC_QUARTILE, buf);
        u8g2_DrawXBMP(u8g2, 32, 32, qrcode.size, qrcode.size, qrcodeData);
    }
    u8g2_UpdateDisplay(u8g2);
}

void app_main()
{
    /* Configure the IOMUX register for pad BLINK_GPIO (some pads are
       muxed to GPIO on reset already, but some default to other
       functions and need to be switched to GPIO. Consult the
       Technical Reference for a list of pads and their default
       functions.)
    */
    gpio_pad_select_gpio(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
    u8g2_esp32_hal.sda   = PIN_SDA;
    u8g2_esp32_hal.scl   = PIN_SCL;
    u8g2_esp32_hal.reset = PIN_RESET;
    u8g2_esp32_hal_init(u8g2_esp32_hal);


    u8g2_t u8g2;
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(
        &u8g2, U8G2_R0, u8g2_esp32_i2c_byte_cb, u8g2_esp32_gpio_and_delay_cb);
    u8x8_SetI2CAddress(&u8g2.u8x8,0x78);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    u8g2_ClearBuffer(&u8g2);

    example_configure_stdin_stdout();

    int num, num1, result;
    char op = 0;
    num = num1 = result = 0;
    while(true) {
        int c = fgetc(stdin);

        if (isNum(c)) {
            if (result != 0) {
                num = num1 = result = 0;
                op = 0;
            }
            num = num * 10 + (c - '0');
        } else if (isOp(c)) {
            op = c;
            num1 = num;
            num = 0;
        } else if (c == '=') {
            result = doOp(num1, num, op);
        } else if (c == 'c') {
            num = 0;
        }

        draw(&u8g2, num1, num, op, result);
    }
}
