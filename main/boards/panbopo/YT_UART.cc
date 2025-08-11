#include "YT_UART.h"

#include <esp_log.h>
#include <nvs_flash.h>
#include <nvs.h>
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
#include "font_awesome_symbols.h"
#define TAG "YT_UART"

uint8_t yt_command_flag =0;
uint8_t yt_bluetooth_flag =YT_OFF;
static uint8_t flag_sound=0;  //蓝牙控制声音与c3标志
static uint8_t Bluetooth_connect=0;  //蓝牙连接上才能控制标志位
uint8_t Yt_cmd[5] = {0xAA, 0xAA, 0x02, 0x01, 0x03};

extern "C" void uart_yt_init(void)
{
    uart_config_t uart_conf = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        // .source_clk = UART_SCLK_APB,
    };
    uart_param_config(UART_YT_PORT, &uart_conf);
    uart_set_pin(UART_YT_PORT, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_YT_PORT, BUF_SIZE * 2, BUF_SIZE * 2, 0, NULL, 0);
    ESP_LOGI(TAG, "000000000000000000000000\n");
}
/*串口接受函数*/
void uart_receive_task_YT(void *pvParameters)   
{
    uint8_t *data = (uint8_t *)malloc(1024);
    uint32_t check = 0;
    if (data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate UART buffer!");
        vTaskDelete(NULL);
        return;
    }
    while (1)
    {
        int len = uart_read_bytes(UART_YT_PORT, data, 1024, 12 / portTICK_PERIOD_MS);  //20
        if (len > 0)
        {
            if (data[0] == 0xaa && data[1] == 0xAA && data[2] == 0x01)
            {
                
                if (data[4] == (data[3] + 1))
                {
                    ESP_LOGI(TAG, "YT2228: %x,%x,%x,%x,%x ", data[0], data[1], data[2], data[3], data[4]);
                    // gpio_set_level(GPIO_NUM_11, 1);
                    if (data[3] == 0x01&&yt_bluetooth_flag==YT_OFF) //唤醒小智
                    {
                        yt_command_flag=Wakeup_Xiaozhi;
                        // yt_bluetooth_flag =YT_OFF;
                    }
                    if (data[3] == 0x01&&yt_bluetooth_flag==YT_ON) //YT BT发送的
                    {
                        yt_command_flag=BT_Wakeup_Xiaozhi;
                    }
                    if (data[3] == 0x2A &&yt_bluetooth_flag ==YT_OFF) //进入配网
                    {
                        yt_command_flag=Distribution_network_mode;
                    }
                    else if (data[3] == 0x31||data[3] == 0x34 ||data[3] == 0x35 ) //学习唤醒词过程
                    {
                        yt_command_flag=Wake_word_pattern;
                    }
                    else if (data[3] == 0x32) //唤醒词成功或者失败
                    {
                        yt_command_flag=Wake_word_ended;
                    }
                    else if (data[3] == 0x33) //唤醒词失败
                    {
                        yt_command_flag=Wake_word_false_ended;
                    }
                    else if (data[3] == 0x1c ||data[3] == 0x2B ||data[3] == 0x1D) //蓝牙模式 或打开蓝牙  音响模式
                    {
                        flag_sound=1; //c3关闭标志
                        yt_command_flag=Bluetooth_mode;
                        yt_bluetooth_flag =YT_ON;
                    }
                    else if (data[3] == 0x2D) //增大音量
                    {
                        yt_command_flag=Increase_volume;
                    }
                    else if (data[3] == 0x2E) //减少音量
                    {
                        yt_command_flag=Decrease_volume;
                    }
                    // else if (data[3] == 0x22 || data[3] == 0x23) //上一首
                    // {
                    //     yt_command_flag=Last_song;
                    // }
                    // else if (data[3] == 0x24 || data[3] == 0x25) //下一首
                    // {
                    //     yt_command_flag=Next_song;
                    // }
                    else if (data[3] == 0x2c ||data[3] == 0x20 ||data[3] == 0x21) //关闭蓝牙 AI模式 AI智能体
                    {
                        flag_sound=0;
                        // yt_bluetooth_flag=YT_OFF;
                        yt_command_flag=Bluetooth_off;
                    }
                    // else if (data[3] == 0x26) //暂停蓝牙音乐
                    // {
                    //     yt_command_flag=Bluetooth_pause;
                    // }
                    // else if (data[3] == 0x27 ||data[3] == 0x28) //播放蓝牙音乐
                    // {
                    //     yt_command_flag=Bluetooth_playing;
                    // }
                    else if (data[3] == 0x37) //蓝牙已断开    
                    {
                        yt_command_flag=Bluetooth_disconnected;
                    }
                    else if (data[3] == 0x36) //蓝牙已连接
                    {
                        yt_command_flag=Bluetooth_connected;
                    }
                    else if (data[3] == 0x2f) //最大音量
                    {
                        yt_command_flag=Maximum_volume;
                    }
                    else if (data[3] == 0x30) //最小音量
                    {
                        yt_command_flag=Minimum_volume;
                    }
                }
            }
            // if (data[0] == 0xaa && data[1] == 0xAA && data[2] == 0x03)
            // {
            //     if (data[4] == (data[2] + data[3]))
            //     {
            //         if(data[3]== 2&&yt_bluetooth_flag==YT_OFF)
            //         {
            //             yt_command_flag=Wakeup_Xiaozhi;
            //         }
            //         else if(data[3]== 1&&yt_bluetooth_flag==YT_OFF)
            //         {
            //             yt_command_flag=BT_Wakeup_Xiaozhi;
            //         }
            //     }
            // }
        }
        vTaskDelay(200 / portTICK_PERIOD_MS);  // 无命令时降低CPU占用   //100
    }
    free(data);
    vTaskDelete(NULL);
}
void yt_command_handler_task(void *pvParameters)
{
    auto &board = Board::GetInstance();
    auto display = board.GetDisplay();
    static bool cmd_sent = false;  // 静态变量保持状态
    while (1) {
        // 根据标志位执行操作
        // esp_task_wdt_reset(); // 喂狗
        switch (yt_command_flag) {
            case Wakeup_Xiaozhi:  {// 播放声音
                
                gpio_set_level(GPIO_NUM_11, 0);
                // Application::GetInstance().Clearaudio();
                // vTaskDelay(pdMS_TO_TICKS(200)); //200
                Yt_cmd[2] = 2;
                Yt_cmd[3] = 0x01;   //0x01
                Yt_cmd[4] = 0x03;   //0x03
                uart_write_bytes(UART_YT_PORT, Yt_cmd, 5);
                uart_wait_tx_done(UART_YT_PORT, pdMS_TO_TICKS(100));
                // Application::GetInstance().PlaySound(Lang::Sounds::P3_SUCCESS);
                vTaskDelay(pdMS_TO_TICKS(630)); //200
                // Application::GetInstance().Clearaudio();
                // vTaskDelay(pdMS_TO_TICKS(250)); //200
                ESP_LOGI(TAG, "你好，小智 ");
                // auto &app = Application::GetInstance();
                // app.ToggleChatState();
                Application::GetInstance().WakeWordInvoke1();
                // Application::GetInstance().Clearaudio();
                // vTaskDelay(pdMS_TO_TICKS(50)); //200
                // 只在第一次进入时发送
                if (!cmd_sent) {
                    Yt_cmd[2] = 2;
                    Yt_cmd[3] = 0x20;
                    Yt_cmd[4] = 0x21;
                    int len = uart_write_bytes(UART_YT_PORT, Yt_cmd, 5);
                    uart_wait_tx_done(UART_YT_PORT, pdMS_TO_TICKS(100));
                    ESP_LOGI(TAG, "1111111111111111111111111 ");
                    cmd_sent = true;  // 标记为已发送
                }
                yt_command_flag = 20;  // 清除标志
                break;
            }
            case Distribution_network_mode:  // 重启配网
            {
                Settings settings("wifi", true);
                settings.SetInt("force_ap", 1);
                // GetDisplay()->ShowNotification(Lang::Strings::ENTERING_WIFI_CONFIG_MODE);
                // Reboot the device
                esp_restart(); 
                yt_bluetooth_flag = 0;
                break;
            }
            case Wake_word_pattern:  // 唤醒词模式
            {
                if(yt_bluetooth_flag ==YT_OFF)
                {
                    vTaskDelay(pdMS_TO_TICKS(120));
                    Application::GetInstance().SetDeviceState(kDeviceStateIdle);
                    vTaskDelay(pdMS_TO_TICKS(10));
                    gpio_set_level(GPIO_NUM_11, 0);
                    ESP_LOGI(TAG, "开始学习唤醒词 ");
                    display->SetStatus("开始学习唤醒词");
                    display->SetEmotion("neutral");
                }
                yt_command_flag = 0;  // 清除标志
                break;
            }
            case Wake_word_ended:  // 唤醒词结束
            {
                if(yt_bluetooth_flag ==YT_OFF)
                {
                    vTaskDelay(pdMS_TO_TICKS(1000));  //语言出不来 延时1s可以
                    Application::GetInstance().SetDeviceState(kDeviceStateIdle);
                    // vTaskDelay(pdMS_TO_TICKS(120));
                    gpio_set_level(GPIO_NUM_11, 1);
                    ESP_LOGI(TAG, "学习完成 ");
                    display->SetStatus("学习完成");
                    display->SetEmotion("happy");
                }
                yt_command_flag = 0;  // 清除标志
                break;
            }
            case Wake_word_false_ended:  // 唤醒词结束
            {
                if(yt_bluetooth_flag ==YT_OFF)
                {
                    vTaskDelay(pdMS_TO_TICKS(1000));  //语言出不来 延时1s可以
                    Application::GetInstance().SetDeviceState(kDeviceStateIdle);
                    // vTaskDelay(pdMS_TO_TICKS(120));
                    gpio_set_level(GPIO_NUM_11, 1);
                    ESP_LOGI(TAG, "学习失败 ");
                    display->SetStatus("学习失败");
                    display->SetEmotion("crying");
                }
                yt_command_flag = 0;  // 清除标志
                break;
            }
            case Bluetooth_mode:  // 蓝牙模式
            {
                Bluetooth_connect=0;
                // vTaskDelay(pdMS_TO_TICKS(120));
                Application::GetInstance().SetDeviceState(kDeviceStateIdle);
                vTaskDelay(pdMS_TO_TICKS(300));
                gpio_set_level(GPIO_NUM_11, 0); //暂时留着

                // A5 AA 01 22 23
                Yt_cmd[2] = 2;
                Yt_cmd[3] = 0x22;
                Yt_cmd[4] = 0x23;
                int len = uart_write_bytes(UART_YT_PORT, Yt_cmd, 5);
                uart_wait_tx_done(UART_YT_PORT, pdMS_TO_TICKS(100)); // 等待最多100ms
                vTaskDelay(pdMS_TO_TICKS(2000));
                gpio_set_level(GPIO_NUM_11, 1); 
                // LcdDisplay::UpdateBluetoothStatus(0,"xiaodou_bt");
                display->SetStatus("蓝牙模式");
                // display->SetEmotion("neutral");
                display->SetIcon(FONT_AWESOME_BLUETOOTH);
                display->SetChatMessage("assistant", "蓝牙模式");
                yt_command_flag = 0;  // 清除标志
                break;
            }
            case Bluetooth_off:  // 蓝牙关闭
            {
                if(yt_bluetooth_flag ==YT_ON)
                {
                    yt_bluetooth_flag=YT_OFF;
                    // vTaskDelay(pdMS_TO_TICKS(120));
                    // Application::GetInstance().SetDeviceState(kDeviceStateIdle);
                    // vTaskDelay(pdMS_TO_TICKS(300));   //120
                    gpio_set_level(GPIO_NUM_11, 0); //暂时留着
                    // gpio_set_level(GPIO_NUM_11, 1);
                    ESP_LOGI(TAG, "关闭蓝牙 ");
                    // A5 AA 01 22 23
                    Yt_cmd[2] = 2;
                    Yt_cmd[3] = 0x23;
                    Yt_cmd[4] = 0x24;
                    int len = uart_write_bytes(UART_YT_PORT, Yt_cmd, 5);

                    vTaskDelay(pdMS_TO_TICKS(2200));
                    gpio_set_level(GPIO_NUM_11, 1); 
                    display->SetStatus("待命");
                    display->SetEmotion("neutral");
                    display->SetChatMessage("assistant", "AI模式");
                    yt_bluetooth_flag=YT_OFF;
                    yt_command_flag = 0;  // 清除标志
                }
                else{
                    yt_command_flag = 20;  // 清除标志
                }
                break;
            }
            case Bluetooth_pause:  // 蓝牙音乐暂停
            {
                if(yt_bluetooth_flag ==YT_ON&&Bluetooth_connect==1)
                {
                    gpio_set_level(GPIO_NUM_11, 1);
                    ESP_LOGI(TAG, "暂停 ");
                    display->SetStatus("暂停");
                    // display->SetEmotion("neutral");
                    Yt_cmd[2] = 2;
                    Yt_cmd[3] = 0x20;
                    Yt_cmd[4] = 0x21;
                    int len = uart_write_bytes(UART_YT_PORT, Yt_cmd, 5); 
                    uart_wait_tx_done(UART_YT_PORT, pdMS_TO_TICKS(100)); // 等待最多100ms
                    yt_command_flag = 0;  // 清除标志
                }
                else
                {
                    yt_command_flag = 20;  // 清除标志
                }
                break;
            }
            case Bluetooth_playing:  // 蓝牙音乐播放
            {
                if(yt_bluetooth_flag ==YT_ON&&Bluetooth_connect==1)
                {
                    gpio_set_level(GPIO_NUM_11, 0);
                    ESP_LOGI(TAG, "播放 ");
                    display->SetStatus("播放");
                    Yt_cmd[2] = 2;
                    Yt_cmd[3] = 0x21;
                    Yt_cmd[4] = 0x22;
                    int len = uart_write_bytes(UART_YT_PORT, Yt_cmd, 5);
                    yt_command_flag = 0;  // 清除标志
                }
                else
                {
                    yt_command_flag = 20;  // 清除标志
                }
                break;
            }
            case Bluetooth_connected:  // 蓝牙已连接
            {

                if(yt_bluetooth_flag ==YT_ON)
                {
                    gpio_set_level(GPIO_NUM_11, 0);
                    Bluetooth_connect=1;
                    display->SetStatus("蓝牙已连接");
                    yt_command_flag = 0;  // 清除标志
                }
                else
                {
                    yt_command_flag = 20;  // 清除标志
                }
                break;
            }
            case Bluetooth_disconnected:  // 蓝牙已断开
            {
                if(yt_bluetooth_flag ==YT_ON)
                {
                    Bluetooth_connect=0;
                    gpio_set_level(GPIO_NUM_11, 0);
                    vTaskDelay(pdMS_TO_TICKS(1500));
                    gpio_set_level(GPIO_NUM_11, 1);
                    display->SetStatus("蓝牙已断开");
                    yt_command_flag = 0;  // 清除标志
                }
                else
                {
                    yt_command_flag = 20;  // 清除标志
                }
                break;
            }
            case Increase_volume:  // 增大音量
            {
                if(yt_bluetooth_flag ==YT_ON&&Bluetooth_connect==1)
                {
                    gpio_set_level(GPIO_NUM_11, 0);
                    ESP_LOGI(TAG, "增大音量 ");
                    // A5 AA 01 22 23
                    Yt_cmd[2] = 2;
                    Yt_cmd[3] = 0x24;
                    Yt_cmd[4] = 0x25;
                    int len = uart_write_bytes(UART_YT_PORT, Yt_cmd, 5);
                    uart_wait_tx_done(UART_YT_PORT, pdMS_TO_TICKS(100)); // 等待最多100ms
                    display->SetStatus("增大音量");
                    yt_command_flag = 0;  // 清除标志
                }
                else
                {
                    yt_command_flag = 20;  // 清除标志
                }
                break; 
            }
            case Decrease_volume:  // 减少音量
            {
                if(yt_bluetooth_flag ==YT_ON&&Bluetooth_connect==1)
                {
                    gpio_set_level(GPIO_NUM_11, 0);
                    ESP_LOGI(TAG, "减少音量 ");
                    // A5 AA 01 22 23
                    Yt_cmd[2] = 2;
                    Yt_cmd[3] = 0x25;
                    Yt_cmd[4] = 0x26;
                    int len = uart_write_bytes(UART_YT_PORT, Yt_cmd, 5);
                    uart_wait_tx_done(UART_YT_PORT, pdMS_TO_TICKS(100)); // 等待最多100ms
                    display->SetStatus("减少音量");
                    yt_command_flag = 0;  // 清除标志
                }
                else
                {
                    yt_command_flag = 20;  // 清除标志
                }
                break;
            }
            case Last_song:  // 上一首
            {
                if(yt_bluetooth_flag ==YT_ON&&Bluetooth_connect==1)
                {
                    gpio_set_level(GPIO_NUM_11, 0);
                    ESP_LOGI(TAG, "上一首 ");
                    // A5 AA 01 22 23
                    Yt_cmd[2] = 2;
                    Yt_cmd[3] = 0x1E;
                    Yt_cmd[4] = 0x1F;
                    int len = uart_write_bytes(UART_YT_PORT, Yt_cmd, 5);
                    uart_wait_tx_done(UART_YT_PORT, pdMS_TO_TICKS(100)); // 等待最多100ms
                    display->SetStatus("上一首");
                    yt_command_flag = 0;  // 清除标志
                }
                else
                {
                    yt_command_flag = 20;  // 清除标志
                }
                break;
            }
            case Next_song:  // 下一首
            {
                if(yt_bluetooth_flag ==YT_ON&&Bluetooth_connect==1)
                {
                    gpio_set_level(GPIO_NUM_11, 0);
                    ESP_LOGI(TAG, "下一首 ");
                    // A5 AA 01 22 23
                    Yt_cmd[2] = 2;
                    Yt_cmd[3] = 0x1F;
                    Yt_cmd[4] = 0x20;
                    int len = uart_write_bytes(UART_YT_PORT, Yt_cmd, 5);
                    uart_wait_tx_done(UART_YT_PORT, pdMS_TO_TICKS(100)); // 等待最多100ms
                    display->SetStatus("下一首");
                    yt_command_flag = 0;  // 清除标志
                }
                else
                {
                    yt_command_flag = 20;  // 清除标志
                }
                break;
            }
            case Maximum_volume:  // 最大音量
            {
                if(yt_bluetooth_flag ==YT_ON&&Bluetooth_connect==1)
                {
                    gpio_set_level(GPIO_NUM_11, 0);
                    ESP_LOGI(TAG, "最大音量 ");
                    // A5 AA 01 22 23
                    Yt_cmd[2] = 2;
                    Yt_cmd[3] = 0x26;
                    Yt_cmd[4] = 0x27;
                    int len = uart_write_bytes(UART_YT_PORT, Yt_cmd, 5);
                    uart_wait_tx_done(UART_YT_PORT, pdMS_TO_TICKS(100)); // 等待最多100ms
                    display->SetStatus("最大音量");
                    yt_command_flag = 0;  // 清除标志
                }
                else
                {
                    yt_command_flag = 20;  // 清除标志
                }
                break;
            }
            case Minimum_volume:  // 最小音量
            {
                if(yt_bluetooth_flag ==YT_ON&&Bluetooth_connect==1)
                {
                    gpio_set_level(GPIO_NUM_11, 0);
                    ESP_LOGI(TAG, "最小音量 ");
                    // A5 AA 01 22 23
                    Yt_cmd[2] = 2;
                    Yt_cmd[3] = 0x27;
                    Yt_cmd[4] = 0x28;
                    int len = uart_write_bytes(UART_YT_PORT, Yt_cmd, 5);
                    uart_wait_tx_done(UART_YT_PORT, pdMS_TO_TICKS(100)); // 等待最多100ms
                    display->SetStatus("最小音量");
                    //5.6加
                    // vTaskDelay(pdMS_TO_TICKS(2100));
                    // gpio_set_level(GPIO_NUM_11, 1); 
                    yt_command_flag = 0;  // 清除标志
                }
                else
                {
                    yt_command_flag = 20;  // 清除标志
                }
                break;
            }
            case BT_Wakeup_Xiaozhi:  // 蓝牙唤醒小智
            {
                // vTaskDelay(pdMS_TO_TICKS(120));
                // Application::GetInstance().SetDeviceState(kDeviceStateIdle);
                if(yt_bluetooth_flag ==YT_ON)
                {
                    gpio_set_level(GPIO_NUM_11, 0);
                    ESP_LOGI(TAG, "蓝牙小智唤醒 ");
                    // A5 AA 01 22 23
                    Yt_cmd[2] = 2;
                    Yt_cmd[3] = 0x01;
                    Yt_cmd[4] = 0x03;
                    int len = uart_write_bytes(UART_YT_PORT, Yt_cmd, 5);
                    uart_wait_tx_done(UART_YT_PORT, pdMS_TO_TICKS(100)); // 等待最多100ms

                    vTaskDelay(pdMS_TO_TICKS(1000));
                    gpio_set_level(GPIO_NUM_11, 1);
                    display->SetStatus("蓝牙唤醒");
                    display->SetIcon(FONT_AWESOME_BLUETOOTH);
                    yt_command_flag = 0;  // 清除标志
                }
                else
                {
                    yt_command_flag = 20;  // 清除标志
                }
                break;
            }
            default:
                vTaskDelay(100 / portTICK_PERIOD_MS);  // 无命令时降低CPU占用   200
            break;
        }
    }
}
void YT_init()
{
    uart_yt_init();
    xTaskCreate(uart_receive_task_YT, "uart_receive_task_YT", 8192, NULL, 7, NULL);   //
    xTaskCreate(yt_command_handler_task, "yt_handler", 10240, NULL, 8, NULL);  // 优先级低于UART任务
}