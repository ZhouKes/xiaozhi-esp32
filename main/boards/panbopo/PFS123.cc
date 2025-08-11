/*
 * @Date: 2025-08-11 21:57:53
 * @LastEditors: zhouke
 * @LastEditTime: 2025-08-11 22:58:42
 * @FilePath: \xiaozhi-esp32\main\boards\panbopo\PFS123.cc
 */
#include "PFS123.h"

#include <esp_log.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <driver/gpio.h>
#include <esp_event.h>
#include <driver/uart.h>
#include "application.h"
#include "system_info.h"
#include "assets/lang_config.h"
#include "settings.h"
#include "board.h"
#include "display.h"
#include "lcd_display.h"
#include "lvgl.h"
#define TAG "PFS123"
#define UART_PORT_PFS123 UART_NUM_0
#define RX_PIN_FPS123 GPIO_NUM_9
extern volatile bool is_battery_low;
#define BUF_SIZE (512) // 缓冲区大小
extern "C" void uart_init_PFS123(void)
{
    uart_config_t uart_conf = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        // .source_clk = UART_SCLK_APB,
    };

    uart_driver_install(UART_PORT_PFS123, BUF_SIZE * 2, BUF_SIZE * 2, 0, NULL, 0);

    uart_param_config(UART_PORT_PFS123, &uart_conf);
    // uart_set_pin(UART_PORT_YT, UART_PIN_NO_CHANGE, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_set_pin(UART_PORT_PFS123, UART_PIN_NO_CHANGE, RX_PIN_FPS123, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

 
}


void uart_receive_task_PFS123(void *pvParameters)
{
    uint8_t *data = (uint8_t *)malloc(256);
    uint32_t check = 0;
    while (1)
    {
        int len = uart_read_bytes(UART_PORT_PFS123, data, 256, 20 / portTICK_PERIOD_MS);
        if (len > 0)
        {
            // 验证数据 并释放信号量
            if(data[0] == 0xFF) 
            {
                is_battery_low = true;
            }else{
                is_battery_low = false;
            }
            printf("\n");
            // gpio_reset_pin(GPIO_NUM_9);
            // gpio_set_direction(GPIO_NUM_9, GPIO_MODE_OUTPUT);
            // gpio_set_level(GPIO_NUM_9, 0); // 拉低
        }
        
    }
    free(data);
    vTaskDelete(NULL);
}
void PFS123_init()
{
    uart_init_PFS123();
    xTaskCreate(uart_receive_task_PFS123, "uart_receive_task_PFS123", 1024, NULL, 12, NULL);
}