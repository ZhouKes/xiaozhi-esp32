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

    ESP_LOGI(TAG, "000000000000000000000001\n");
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
                // auto display = Board::GetInstance().GetDisplay();
                // display->SetEmotion("shutdown");
                // display->SetChatMessage("system", "电量低，即将关机...\n");

                // Application::GetInstance().GetDisplay().ShowLowBatteryPopup(true);
                lv_obj_t* low_battery_popup_ = nullptr;
                auto screen = lv_screen_active();
                low_battery_popup_ = lv_obj_create(screen);
                lv_obj_set_scrollbar_mode(low_battery_popup_, LV_SCROLLBAR_MODE_OFF);
                lv_obj_set_size(low_battery_popup_, LV_HOR_RES * 0.9, LV_VER_RES * 0.9);
                lv_obj_align(low_battery_popup_, LV_ALIGN_BOTTOM_MID, 0, 0);
                lv_obj_set_style_bg_color(low_battery_popup_, lv_color_hex(0xFF0000), 0);
                lv_obj_set_style_radius(low_battery_popup_, 10, 0);
                lv_obj_t* low_battery_label = lv_label_create(low_battery_popup_);
                lv_label_set_text(low_battery_label, Lang::Strings::BATTERY_NEED_CHARGE);
                lv_obj_set_style_text_color(low_battery_label, lv_color_white(), 0);
                lv_obj_center(low_battery_label);
                auto& app = Application::GetInstance();
                app.PlaySound(Lang::Sounds::P3_LOW_BATTERY);
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
    xTaskCreate(uart_receive_task_PFS123, "uart_receive_task_PFS123", 4096, NULL, 12, NULL);
}