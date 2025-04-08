#include "font/lv_font.h"
#include "misc/lv_types.h"
#include "wifi_board.h"
#include "audio_codecs/no_audio_codec.h"
#include "display/lcd_display.h"
#include "system_reset.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "iot/thing_manager.h"

#include <esp_log.h>
#include "i2c_device.h"
#include <driver/i2c_master.h>
#include <driver/ledc.h>
#include <wifi_station.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_spd2010.h>
#include <esp_timer.h>
#include "esp_io_expander_tca9554.h"
#include "lcd_display.h"
#include <iot_button.h>
#include <cstring>
#include "spd2010_touch.h"
#include <font_awesome_symbols.h>
#include "assets/lang_config.h"
#include <esp_http_client.h>
#include <cJSON.h>
#include "power_manager.h"
#include "power_save_timer.h"
#include <esp_sleep.h>
#include "button.h"
#define TAG "waveshare_lcd_1_46a"

LV_FONT_DECLARE(time40);
LV_FONT_DECLARE(font_puhui_16_4);
LV_FONT_DECLARE(font_awesome_16_4);
LV_FONT_DECLARE(font_awesome_30_4);
#define I2C_CLK_SPEED               400000



// Color definitions for dark theme
#define DARK_BACKGROUND_COLOR       lv_color_hex(0x121212)     // Dark background
#define DARK_TEXT_COLOR             lv_color_white()           // White text
#define DARK_CHAT_BACKGROUND_COLOR  lv_color_hex(0x1E1E1E)     // Slightly lighter than background
#define DARK_USER_BUBBLE_COLOR      lv_color_hex(0x1A6C37)     // Dark green
#define DARK_ASSISTANT_BUBBLE_COLOR lv_color_hex(0x333333)     // Dark gray
#define DARK_SYSTEM_BUBBLE_COLOR    lv_color_hex(0x2A2A2A)     // Medium gray
#define DARK_SYSTEM_TEXT_COLOR      lv_color_hex(0xAAAAAA)     // Light gray text
#define DARK_BORDER_COLOR           lv_color_hex(0x333333)     // Dark gray border
#define DARK_LOW_BATTERY_COLOR      lv_color_hex(0xFF0000)     // Red for dark mode

// Color definitions for light theme
#define LIGHT_BACKGROUND_COLOR       lv_color_white()           // White background
#define LIGHT_TEXT_COLOR             lv_color_black()           // Black text
#define LIGHT_CHAT_BACKGROUND_COLOR  lv_color_hex(0xE0E0E0)     // Light gray background
#define LIGHT_USER_BUBBLE_COLOR      lv_color_hex(0x95EC69)     // WeChat green
#define LIGHT_ASSISTANT_BUBBLE_COLOR lv_color_white()           // White
#define LIGHT_SYSTEM_BUBBLE_COLOR    lv_color_hex(0xE0E0E0)     // Light gray
#define LIGHT_SYSTEM_TEXT_COLOR      lv_color_hex(0x666666)     // Dark gray text
#define LIGHT_BORDER_COLOR           lv_color_hex(0xE0E0E0)     // Light gray border
#define LIGHT_LOW_BATTERY_COLOR      lv_color_black()           // Black for light mode

// Theme color structure
struct ThemeColors {
    lv_color_t background;
    lv_color_t text;
    lv_color_t chat_background;
    lv_color_t user_bubble;
    lv_color_t assistant_bubble;
    lv_color_t system_bubble;
    lv_color_t system_text;
    lv_color_t border;
    lv_color_t low_battery;
};

// Define dark theme colors
static const ThemeColors DARK_THEME = {
    .background = DARK_BACKGROUND_COLOR,
    .text = DARK_TEXT_COLOR,
    .chat_background = DARK_CHAT_BACKGROUND_COLOR,
    .user_bubble = DARK_USER_BUBBLE_COLOR,
    .assistant_bubble = DARK_ASSISTANT_BUBBLE_COLOR,
    .system_bubble = DARK_SYSTEM_BUBBLE_COLOR,
    .system_text = DARK_SYSTEM_TEXT_COLOR,
    .border = DARK_BORDER_COLOR,
    .low_battery = DARK_LOW_BATTERY_COLOR
};

// Define light theme colors
static const ThemeColors LIGHT_THEME = {
    .background = LIGHT_BACKGROUND_COLOR,
    .text = LIGHT_TEXT_COLOR,
    .chat_background = LIGHT_CHAT_BACKGROUND_COLOR,
    .user_bubble = LIGHT_USER_BUBBLE_COLOR,
    .assistant_bubble = LIGHT_ASSISTANT_BUBBLE_COLOR,
    .system_bubble = LIGHT_SYSTEM_BUBBLE_COLOR,
    .system_text = LIGHT_SYSTEM_TEXT_COLOR,
    .border = LIGHT_BORDER_COLOR,
    .low_battery = LIGHT_LOW_BATTERY_COLOR
};
LV_IMG_DECLARE(bg1);
LV_IMG_DECLARE(bg2);
LV_IMG_DECLARE(bg3);
LV_IMG_DECLARE(bg4);
LV_IMG_DECLARE(min);
// Current theme - initialize based on default config
static ThemeColors current_theme = LIGHT_THEME;

// 全局静态变量，用于天气任务函数访问
static lv_obj_t* s_weather_temp_label = nullptr;

// 定义一个全局缓冲区，用于HTTP事件处理器
static char* s_weather_response_buffer = NULL;
static int s_weather_response_len = 0;

// HTTP事件处理函数
static esp_err_t weather_http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            // 清空缓冲区
            if (s_weather_response_buffer) {
                free(s_weather_response_buffer);
                s_weather_response_buffer = NULL;
            }
            s_weather_response_len = 0;
            break;
        case HTTP_EVENT_HEADERS_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADERS_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            // 首次收到数据时分配内存
            if (s_weather_response_buffer == NULL) {
                s_weather_response_buffer = (char *)malloc(esp_http_client_get_content_length(evt->client) + 1);
                if (s_weather_response_buffer == NULL) {
                    ESP_LOGE(TAG, "Failed to allocate memory for response buffer");
                    return ESP_FAIL;
                }
                s_weather_response_len = 0;
            }
            // 复制数据到缓冲区
            memcpy(s_weather_response_buffer + s_weather_response_len, evt->data, evt->data_len);
            s_weather_response_len += evt->data_len;
            s_weather_response_buffer[s_weather_response_len] = 0; // 确保字符串以NULL结尾
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            break;
        default:
            ESP_LOGD(TAG, "未处理的HTTP事件: %d", evt->event_id);
            break;
    }
    return ESP_OK;
}

// 天气获取函数
static bool fetch_weather() {
    if (!s_weather_temp_label) return false;
    
    // 检查网络连接状态
    if (!WifiStation::GetInstance().IsConnected()) {
        // 如果网络未连接，更新标签显示
        lv_async_call([](void*) {
            lv_label_set_text(s_weather_temp_label, "正在获取天气信息...");
        }, NULL);
        return false; // 返回获取失败
    }
    
    // 创建HTTP请求获取天气数据
    esp_http_client_config_t config = {
        .url = "http://xiaozhiota.online:5006/json",
        .method = HTTP_METHOD_GET,
        .timeout_ms = 30000, // 设置超时时间为30秒
        .event_handler = weather_http_event_handler, // 设置事件处理器
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    // 重置响应缓冲区
    if (s_weather_response_buffer) {
        free(s_weather_response_buffer);
        s_weather_response_buffer = NULL;
    }
    s_weather_response_len = 0;
    
    esp_err_t err = esp_http_client_perform(client);
    bool fetch_success = false; // 获取结果标志
    
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200) {
            // 检查是否收到了响应数据
            if (s_weather_response_buffer && s_weather_response_len > 0) {
                // 解析JSON数据
                cJSON *root = cJSON_Parse(s_weather_response_buffer);
                if (root) {
                    cJSON *location = cJSON_GetObjectItem(root, "location");
                    cJSON *weather = cJSON_GetObjectItem(root, "weather");
                    cJSON *temperature = cJSON_GetObjectItem(root, "temperature");
                    cJSON *humidity = cJSON_GetObjectItem(root, "humidity");
                    
                    if (location && weather && temperature && humidity) {
                        // 格式化天气信息为两行显示
                        char weather_info[128];
                        snprintf(weather_info, sizeof(weather_info), 
                                 "%s\n%s  温度:%s°C  湿度:%s%%", 
                                 location->valuestring, 
                                 weather->valuestring,
                                 temperature->valuestring,
                                 humidity->valuestring);
                        
                        // 更新UI (注意: 这需要在主线程中执行)
                        // 使用LVGL异步调用确保在主线程更新UI
                        char *info_copy = strdup(weather_info);
                        if (info_copy) {
                            lv_async_call([](void* data) {
                                char* info = (char*)data;
                                lv_label_set_text(s_weather_temp_label, info);
                                free(info);  // 释放复制的字符串
                            }, info_copy);
                            fetch_success = true; // 成功获取并显示天气数据
                        }
                    }
                    
                    cJSON_Delete(root);
                } else {
                    // 在主线程更新UI
                    lv_async_call([](void*) {
                        lv_label_set_text(s_weather_temp_label, "天气数据解析失败");
                    }, NULL);
                }
            } else {
                // 在主线程更新UI
                lv_async_call([](void*) {
                    lv_label_set_text(s_weather_temp_label, "未收到天气数据");
                }, NULL);
            }
        } else {
            // 显示HTTP错误
            char error_msg[64];
            snprintf(error_msg, sizeof(error_msg), "HTTP错误: %d", status_code);
            char *msg_copy = strdup(error_msg);
            if (msg_copy) {
                lv_async_call([](void* data) {
                    char* msg = (char*)data;
                    lv_label_set_text(s_weather_temp_label, msg);
                    free(msg);  // 释放复制的字符串
                }, msg_copy);
            }
        }
    } else {
        // 显示连接错误
        lv_async_call([](void*) {
            lv_label_set_text(s_weather_temp_label, "连接服务器失败");
        }, NULL);
    }
    
    // 释放HTTP客户端和响应缓冲区
    esp_http_client_cleanup(client);
    if (s_weather_response_buffer) {
        free(s_weather_response_buffer);
        s_weather_response_buffer = NULL;
    }
    s_weather_response_len = 0;
    
    return fetch_success; // 返回获取结果
}

// 天气任务函数
static void weather_task_func(void* arg) {
    const int RETRY_INTERVAL = 10 * 1000; // 10秒重试间隔（毫秒）
    const int SUCCESS_INTERVAL = 60 * 60 * 1000; // 1小时成功后间隔（毫秒）
    
    for (;;) {
        // 执行天气获取
        bool success = fetch_weather();
        
        if (success) {
            // 获取成功，等待1小时
            ESP_LOGI(TAG, "天气数据获取成功，1小时后重试");
            vTaskDelay(pdMS_TO_TICKS(SUCCESS_INTERVAL));
        } else {
            // 获取失败，10秒后重试
            ESP_LOGI(TAG, "天气数据获取失败，10秒后重试");
            vTaskDelay(pdMS_TO_TICKS(RETRY_INTERVAL));
        }
    }
}

// 创建天气任务
static void create_weather_task() {
    static TaskHandle_t weather_task_handle = NULL;
    
    // 如果任务已存在则不再创建
    if (weather_task_handle != NULL) {
        return;
    }
    
    xTaskCreate(
        weather_task_func,
        "weather_task",
        4096,  // 栈大小
        NULL,
        5,     // 优先级
        &weather_task_handle
    );
}

class CustomLcdDisplay : public SpiLcdDisplay {
public:
    SPD2010TouchDriver *touch_driver_ = nullptr;
    lv_timer_t *idle_timer_ = nullptr;  // Add timer pointer member variable
    static void rounder_event_cb(lv_event_t * e) {
        lv_area_t * area = (lv_area_t *)lv_event_get_param(e);
        uint16_t x1 = area->x1;
        uint16_t x2 = area->x2;

        area->x1 = (x1 >> 2) << 2;          // round the start of coordinate down to the nearest 4M number
        area->x2 = ((x2 >> 2) << 2) + 3;    // round the end of coordinate up to the nearest 4N+3 number
    }
    lv_obj_t * tab1 = nullptr;
    lv_obj_t * tab2 = nullptr;
    lv_obj_t * bg_img = nullptr;  // Background image object
    lv_obj_t * bg_img2 = nullptr;  // Background image object
    lv_obj_t * emotion_gif = nullptr;
    uint8_t bg_index = 1;         // Current background index (1-4)
    lv_obj_t * bg_switch_btn = nullptr;  // Button to switch backgrounds
    lv_obj_t * container_toggle_btn = nullptr;  // Button to toggle container visibility
    bool container_visible = true;  // Track container visibility state

    CustomLcdDisplay(esp_lcd_panel_io_handle_t io_handle, 
                    esp_lcd_panel_handle_t panel_handle,
                    int width,
                    int height,
                    int offset_x,
                    int offset_y,
                    bool mirror_x,
                    bool mirror_y,
                    bool swap_xy) 
        : SpiLcdDisplay(io_handle, panel_handle,
                    width, height, offset_x, offset_y, mirror_x, mirror_y, swap_xy,
                    {
                        .text_font = &font_puhui_16_4,
                        .icon_font = &font_awesome_16_4,
                        .emoji_font = font_emoji_32_init(),
                    }) {
        DisplayLockGuard lock(this);
        lv_display_add_event_cb(display_, rounder_event_cb, LV_EVENT_INVALIDATE_AREA, NULL);
        SetupUI();
    }

    void SetIdle(bool status) override 
    {
        // If status is false, just return
        if (status == false)
        {
            if (idle_timer_ != nullptr) {
                lv_timer_del(idle_timer_);
                idle_timer_ = nullptr;
            }
            return;
        } 

        // If there's an existing timer, delete it first
        if (idle_timer_ != nullptr) {
            lv_timer_del(idle_timer_);
            idle_timer_ = nullptr;
        }

        // Create a timer to switch to tab2 after 15 seconds
        idle_timer_ = lv_timer_create([](lv_timer_t * t) {
            CustomLcdDisplay *display = (CustomLcdDisplay *)lv_timer_get_user_data(t);
            if (!display) return;
            
            // Find the tabview and switch to tab2
            lv_obj_t *tabview = lv_obj_get_parent(lv_obj_get_parent(display->tab2));
            if (tabview) {
                lv_tabview_set_act(tabview, 1, LV_ANIM_ON);
            }
            
            // Delete the timer after it's done
            lv_timer_del(t);
            display->idle_timer_ = nullptr;
        }, 15000, this);  // 15000ms = 15 seconds
    }
#if CONFIG_USE_WECHAT_MESSAGE_STYLE
void SetupTab1() {
 
    lv_obj_set_style_text_font(tab1, fonts_.text_font, 0);
    lv_obj_set_style_text_color(tab1, current_theme.text, 0);
    lv_obj_set_style_bg_color(tab1, current_theme.background, 0);

    /* Background image for tab1 */
    bg_img = lv_img_create(tab1);
    
    /* Set the image descriptor */
    lv_img_set_src(bg_img, &bg1);
    
    /* Position the image to cover the entire tab */
    lv_obj_set_size(bg_img, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_pos(bg_img, -16, -16);
    
    /* Send to background */
    lv_obj_move_background(bg_img);

    emotion_gif = lv_gif_create(tab1);
    int gif_size = LV_HOR_RES;
    lv_obj_set_size(emotion_gif, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_border_width(emotion_gif, 0, 0);
    lv_obj_set_style_bg_opa(emotion_gif, LV_OPA_TRANSP, 0);
    lv_obj_set_pos(emotion_gif, -16, -16);
    lv_gif_set_src(emotion_gif, &min);
    
    /* Container */
    container_ = lv_obj_create(tab1);
    // 将container_高度改为原来的四分之三，宽度保持不变
    lv_obj_set_size(container_, LV_HOR_RES * 0.7, LV_VER_RES * 0.7 * 0.75);
    // 将container_放置在屏幕下方
    lv_obj_align(container_, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_flex_flow(container_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_row(container_, 0, 0);
    lv_obj_set_style_bg_color(container_, current_theme.background, 0);
    lv_obj_set_style_border_color(container_, current_theme.border, 0);
    lv_obj_set_style_bg_opa(container_, LV_OPA_TRANSP, 0);
    
    /* Create right-side toggle button */
    container_toggle_btn = lv_btn_create(tab1);
    lv_obj_set_size(container_toggle_btn, 60, 60);  // 50% larger (40->60)
    // 位置调整为垂直居中
    lv_obj_align(container_toggle_btn, LV_ALIGN_RIGHT_MID, -2, 0);  // 右侧中间位置
    lv_obj_set_style_bg_opa(container_toggle_btn, LV_OPA_50, 0);    // Make semi-transparent
    lv_obj_set_style_radius(container_toggle_btn, 30, 0);           // Round corners (increased to match new size)
    
    lv_obj_t *toggle_btn_label = lv_label_create(container_toggle_btn);
    lv_label_set_text(toggle_btn_label, FONT_AWESOME_XMARK);      // Use X mark icon
    lv_obj_set_style_text_font(toggle_btn_label, fonts_.icon_font, 0);
    lv_obj_center(toggle_btn_label);
    
    // Add click event handler for the toggle button
    lv_obj_add_event_cb(container_toggle_btn, [](lv_event_t *e) {
        CustomLcdDisplay *display = (CustomLcdDisplay *)lv_event_get_user_data(e);
        if (!display) return;
        
        // Toggle container visibility
        display->container_visible = !display->container_visible;
        
        // Update container visibility
        if (display->container_visible) {
            lv_obj_clear_flag(display->container_, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(lv_obj_get_child(display->container_toggle_btn, 0), FONT_AWESOME_XMARK);
        } else {
            lv_obj_add_flag(display->container_, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(lv_obj_get_child(display->container_toggle_btn, 0), FONT_AWESOME_COMMENT);
        }
    }, LV_EVENT_CLICKED, this);
    
    /* Create background switch button */
    bg_switch_btn = lv_btn_create(tab1);
    lv_obj_set_size(bg_switch_btn, 60, 60);  // 50% larger (40->60)
    // 位置调整为垂直居中
    lv_obj_align(bg_switch_btn, LV_ALIGN_LEFT_MID, 2, 0);  // 左侧中间位置
    lv_obj_set_style_bg_opa(bg_switch_btn, LV_OPA_30, 0);    // Make semi-transparent
    lv_obj_set_style_radius(bg_switch_btn, 30, 0);           // Round corners (increased to match new size)
    
    lv_obj_t *btn_label = lv_label_create(bg_switch_btn);
    lv_label_set_text(btn_label, FONT_AWESOME_ARROW_RIGHT);  // Use arrow icon
    lv_obj_set_style_text_font(btn_label, fonts_.icon_font, 0);
    lv_obj_center(btn_label);
    
    // Add click event handler for the button
    lv_obj_add_event_cb(bg_switch_btn, [](lv_event_t *e) {
        CustomLcdDisplay *display = (CustomLcdDisplay *)lv_event_get_user_data(e);
        if (!display) return;
        
        // Cycle through backgrounds
        display->bg_index = (display->bg_index % 4) + 1;
        
        // Set the new background image
        switch (display->bg_index) {
            case 1: lv_img_set_src(display->bg_img, &bg1); break;
            case 2: lv_img_set_src(display->bg_img, &bg2); break;
            case 3: lv_img_set_src(display->bg_img, &bg3); break;
            case 4: lv_img_set_src(display->bg_img, &bg4); break;
        }
    }, LV_EVENT_CLICKED, this);
    
    /* Status bar */
    status_bar_ = lv_obj_create(container_);
    lv_obj_set_size(status_bar_, LV_HOR_RES * 0.6, fonts_.emoji_font->line_height);
    lv_obj_set_style_radius(status_bar_, 0, 0);
    lv_obj_set_style_bg_color(status_bar_, current_theme.background, 0);
    lv_obj_set_style_text_color(status_bar_, current_theme.text, 0);
    //lv_obj_set_style_bg_opa(status_bar_, LV_OPA_TRANSP, 0);
    // Set status bar background to semi-transparent
    lv_obj_set_style_bg_opa(status_bar_, LV_OPA_50, 0);

    /* Content - Chat area */
    content_ = lv_obj_create(container_);
    lv_obj_set_style_radius(content_, 0, 0);
    lv_obj_set_width(content_, LV_HOR_RES * 0.6);
    lv_obj_set_flex_grow(content_, 1);
    lv_obj_set_style_pad_all(content_, 5, 0);
    lv_obj_set_style_bg_color(content_, current_theme.chat_background, 0); // Background for chat area
    lv_obj_set_style_border_color(content_, current_theme.border, 0); // Border color for chat area

    // Enable scrolling for chat content
    lv_obj_set_scrollbar_mode(content_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(content_, LV_DIR_VER);
    
    // Create a flex container for chat messages
    lv_obj_set_flex_flow(content_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(content_, 10, 0); // Space between messages

    lv_obj_set_style_bg_opa(content_, LV_OPA_TRANSP, 0);

    // We'll create chat messages dynamically in SetChatMessage
    chat_message_label_ = nullptr;

    /* Status bar */
    lv_obj_set_flex_flow(status_bar_, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(status_bar_, 0, 0);
    lv_obj_set_style_border_width(status_bar_, 0, 0);
    lv_obj_set_style_pad_column(status_bar_, 0, 0);
    lv_obj_set_style_pad_left(status_bar_, 2, 0);
    lv_obj_set_style_pad_right(status_bar_, 2, 0);
    lv_obj_set_scrollbar_mode(status_bar_, LV_SCROLLBAR_MODE_OFF);
    // 设置状态栏的内容垂直居中
    lv_obj_set_flex_align(status_bar_, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // 创建emotion_label_在状态栏最左侧
    emotion_label_ = lv_label_create(status_bar_);
    lv_obj_set_style_text_font(emotion_label_, &font_awesome_30_4, 0);
    lv_obj_set_style_text_color(emotion_label_, current_theme.text, 0);
    lv_label_set_text(emotion_label_, FONT_AWESOME_AI_CHIP);
    lv_obj_set_style_margin_right(emotion_label_, 5, 0); // 添加右边距，与后面的元素分隔

    notification_label_ = lv_label_create(status_bar_);
    lv_obj_set_flex_grow(notification_label_, 1);
    lv_obj_set_style_text_align(notification_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(notification_label_, current_theme.text, 0);
    lv_label_set_text(notification_label_, "");
    lv_obj_add_flag(notification_label_, LV_OBJ_FLAG_HIDDEN);

    status_label_ = lv_label_create(status_bar_);
    lv_obj_set_flex_grow(status_label_, 1);
    lv_label_set_long_mode(status_label_, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(status_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(status_label_, current_theme.text, 0);
    lv_label_set_text(status_label_, Lang::Strings::INITIALIZING);
    
    mute_label_ = lv_label_create(status_bar_);
    lv_label_set_text(mute_label_, "");
    lv_obj_set_style_text_font(mute_label_, fonts_.icon_font, 0);
    lv_obj_set_style_text_color(mute_label_, current_theme.text, 0);

    network_label_ = lv_label_create(status_bar_);
    lv_label_set_text(network_label_, "");
    lv_obj_set_style_text_font(network_label_, fonts_.icon_font, 0);
    lv_obj_set_style_text_color(network_label_, current_theme.text, 0);
    lv_obj_set_style_margin_left(network_label_, 5, 0); // 添加左边距，与前面的元素分隔

    battery_label_ = lv_label_create(status_bar_);
    lv_label_set_text(battery_label_, "");
    lv_obj_set_style_text_font(battery_label_, fonts_.icon_font, 0);
    lv_obj_set_style_text_color(battery_label_, current_theme.text, 0);
    lv_obj_set_style_margin_left(battery_label_, 5, 0); // 添加左边距，与前面的元素分隔

    low_battery_popup_ = lv_obj_create(tab1);
    lv_obj_set_scrollbar_mode(low_battery_popup_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(low_battery_popup_, LV_HOR_RES * 0.54, fonts_.text_font->line_height * 2);
    lv_obj_align(low_battery_popup_, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(low_battery_popup_, current_theme.low_battery, 0);
    lv_obj_set_style_radius(low_battery_popup_, 10, 0);
    lv_obj_t* low_battery_label = lv_label_create(low_battery_popup_);
    lv_label_set_text(low_battery_label, Lang::Strings::BATTERY_NEED_CHARGE);
    lv_obj_set_style_text_color(low_battery_label, lv_color_white(), 0);
    lv_obj_center(low_battery_label);
    lv_obj_add_flag(low_battery_popup_, LV_OBJ_FLAG_HIDDEN);
}

#define  MAX_MESSAGES 50
void SetChatMessage(const char* role, const char* content) override{
    DisplayLockGuard lock(this);
    if (content_ == nullptr) {
        return;
    }
    
    //避免出现空的消息框
    if(strlen(content) == 0) return;
    
    // Create a message bubble
    lv_obj_t* msg_bubble = lv_obj_create(content_);
    lv_obj_set_style_radius(msg_bubble, 8, 0);
    lv_obj_set_scrollbar_mode(msg_bubble, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_width(msg_bubble, 1, 0);
    lv_obj_set_style_border_color(msg_bubble, current_theme.border, 0);
    lv_obj_set_style_pad_all(msg_bubble, 8, 0);

    // Create the message text
    lv_obj_t* msg_text = lv_label_create(msg_bubble);
    lv_label_set_text(msg_text, content);
    
    // 计算文本实际宽度
    lv_coord_t text_width = lv_txt_get_width(content, strlen(content), fonts_.text_font, 0);

    // 计算气泡宽度
    lv_coord_t max_width = LV_HOR_RES * 0.6 * 85 / 100 - 16;  // 屏幕宽度的85%
    lv_coord_t min_width = 20;  
    lv_coord_t bubble_width;
    
    // 确保文本宽度不小于最小宽度
    if (text_width < min_width) {
        text_width = min_width;
    }

    // 如果文本宽度小于最大宽度，使用文本宽度
    if (text_width < max_width) {
        bubble_width = text_width; 
    } else {
        bubble_width = max_width;
    }
    
    // 设置消息文本的宽度
    lv_obj_set_width(msg_text, bubble_width);  // 减去padding
    lv_label_set_long_mode(msg_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(msg_text, fonts_.text_font, 0);

    // 设置气泡宽度
    lv_obj_set_width(msg_bubble, bubble_width);
    lv_obj_set_height(msg_bubble, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(msg_bubble, LV_OPA_50, 0);
    // Set alignment and style based on message role
    if (strcmp(role, "user") == 0) {
        // User messages are right-aligned with green background
        lv_obj_set_style_bg_color(msg_bubble, current_theme.user_bubble, 0);
        // Set text color for contrast
        lv_obj_set_style_text_color(msg_text, current_theme.text, 0);
        
        // 设置自定义属性标记气泡类型
        lv_obj_set_user_data(msg_bubble, (void*)"user");
        
        // Set appropriate width for content
        lv_obj_set_width(msg_bubble, LV_SIZE_CONTENT);
        lv_obj_set_height(msg_bubble, LV_SIZE_CONTENT);
        
        // Add some margin 
        lv_obj_set_style_margin_right(msg_bubble, 10, 0);
        
        // Don't grow
        lv_obj_set_style_flex_grow(msg_bubble, 0, 0);
    } else if (strcmp(role, "assistant") == 0) {
        // Assistant messages are left-aligned with white background
        lv_obj_set_style_bg_color(msg_bubble, current_theme.assistant_bubble, 0);
        // Set text color for contrast
        lv_obj_set_style_text_color(msg_text, current_theme.text, 0);
        
        // 设置自定义属性标记气泡类型
        lv_obj_set_user_data(msg_bubble, (void*)"assistant");
        
        // Set appropriate width for content
        lv_obj_set_width(msg_bubble, LV_SIZE_CONTENT);
        lv_obj_set_height(msg_bubble, LV_SIZE_CONTENT);
        
        // Add some margin
        lv_obj_set_style_margin_left(msg_bubble, -4, 0);
        
        // Don't grow
        lv_obj_set_style_flex_grow(msg_bubble, 0, 0);
    } else if (strcmp(role, "system") == 0) {
        // System messages are center-aligned with light gray background
        lv_obj_set_style_bg_color(msg_bubble, current_theme.system_bubble, 0);
        // Set text color for contrast
        lv_obj_set_style_text_color(msg_text, current_theme.system_text, 0);
        
        // 设置自定义属性标记气泡类型
        lv_obj_set_user_data(msg_bubble, (void*)"system");
        
        // Set appropriate width for content
        lv_obj_set_width(msg_bubble, LV_SIZE_CONTENT);
        lv_obj_set_height(msg_bubble, LV_SIZE_CONTENT);
        
        // Don't grow
        lv_obj_set_style_flex_grow(msg_bubble, 0, 0);
    }
    
    // Create a full-width container for user messages to ensure right alignment
    if (strcmp(role, "user") == 0) {
        // Create a full-width container
        lv_obj_t* container = lv_obj_create(content_);
        lv_obj_set_width(container, LV_HOR_RES * 0.6);
        lv_obj_set_height(container, LV_SIZE_CONTENT);
        
        // Make container transparent and borderless
        lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(container, 0, 0);
        lv_obj_set_style_pad_all(container, 0, 0);
        
        // Move the message bubble into this container
        lv_obj_set_parent(msg_bubble, container);
        
        // Right align the bubble in the container
        lv_obj_align(msg_bubble, LV_ALIGN_RIGHT_MID, -10, 0);
        
        // Auto-scroll to this container
        lv_obj_scroll_to_view_recursive(container, LV_ANIM_ON);
    } else if (strcmp(role, "system") == 0) {
        // 为系统消息创建全宽容器以确保居中对齐
        lv_obj_t* container = lv_obj_create(content_);
        lv_obj_set_width(container, LV_HOR_RES * 0.6);
        lv_obj_set_height(container, LV_SIZE_CONTENT);
        
        // 使容器透明且无边框
        lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(container, 0, 0);
        lv_obj_set_style_pad_all(container, 0, 0);
        
        // 将消息气泡移入此容器
        lv_obj_set_parent(msg_bubble, container);
        
        // 将气泡居中对齐在容器中
        lv_obj_align(msg_bubble, LV_ALIGN_CENTER, 0, 0);
        
        // 自动滚动底部
        lv_obj_scroll_to_view_recursive(container, LV_ANIM_ON);
    } else {
        // For assistant messages
        // Left align assistant messages
        lv_obj_align(msg_bubble, LV_ALIGN_LEFT_MID, 0, 0);

        // Auto-scroll to the message bubble
        lv_obj_scroll_to_view_recursive(msg_bubble, LV_ANIM_ON);
    }
    
    // Store reference to the latest message label
    chat_message_label_ = msg_text;

    // 检查消息数量是否超过限制
    uint32_t msg_count = lv_obj_get_child_cnt(content_);
    while (msg_count >= MAX_MESSAGES) {
        // 删除最早的消息（第一个子节点）
        lv_obj_t* oldest_msg = lv_obj_get_child(content_, 0);
        if (oldest_msg != nullptr) {
            lv_obj_del(oldest_msg);
            msg_count--;
        }else{
            break;
        }
    }
}
#else
void SetupTab1() {
    DisplayLockGuard lock(this);

     
    lv_obj_set_style_text_font(tab1, fonts_.text_font, 0);
    lv_obj_set_style_text_color(tab1, current_theme.text, 0);
    lv_obj_set_style_bg_color(tab1, current_theme.background, 0);

    /* Container */
    container_ = lv_obj_create(tab1);
    lv_obj_set_size(container_, LV_HOR_RES, LV_VER_RES);
    lv_obj_center(container_);
    lv_obj_set_flex_flow(container_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(container_, 0, 0);
    lv_obj_set_style_border_width(container_, 0, 0);

    /* Status bar */
    status_bar_ = lv_obj_create(container_);
    lv_obj_set_size(status_bar_, LV_HOR_RES, fonts_.text_font->line_height);
    lv_obj_set_style_radius(status_bar_, 0, 0);
    lv_obj_set_style_bg_color(status_bar_, current_theme.background, 0);
    lv_obj_set_style_text_color(status_bar_, current_theme.text, 0);
    
    /* Content */
    content_ = lv_obj_create(container_);
    lv_obj_set_scrollbar_mode(content_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(content_, 0, 0);
    lv_obj_set_width(content_, LV_HOR_RES);
    lv_obj_set_flex_grow(content_, 1);
    lv_obj_set_style_pad_all(content_, 5, 0);
    lv_obj_set_style_bg_color(content_, current_theme.chat_background, 0);
    lv_obj_set_style_border_color(content_, current_theme.border, 0); // Border color for content

    lv_obj_set_flex_flow(content_, LV_FLEX_FLOW_COLUMN); // 垂直布局（从上到下）
    lv_obj_set_flex_align(content_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_EVENLY); // 子对象居中对齐，等距分布

    emotion_label_ = lv_label_create(content_);
    lv_obj_set_style_text_font(emotion_label_, &font_awesome_30_4, 0);
    lv_obj_set_style_text_color(emotion_label_, current_theme.text, 0);
    lv_label_set_text(emotion_label_, FONT_AWESOME_AI_CHIP);

    chat_message_label_ = lv_label_create(content_);
    lv_label_set_text(chat_message_label_, "");
    lv_obj_set_width(chat_message_label_, LV_HOR_RES * 0.9); // 限制宽度为屏幕宽度的 90%
    lv_label_set_long_mode(chat_message_label_, LV_LABEL_LONG_WRAP); // 设置为自动换行模式
    lv_obj_set_style_text_align(chat_message_label_, LV_TEXT_ALIGN_CENTER, 0); // 设置文本居中对齐
    lv_obj_set_style_text_color(chat_message_label_, current_theme.text, 0);

    /* Status bar */
    lv_obj_set_flex_flow(status_bar_, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(status_bar_, 0, 0);
    lv_obj_set_style_border_width(status_bar_, 0, 0);
    lv_obj_set_style_pad_column(status_bar_, 0, 0);
    lv_obj_set_style_pad_left(status_bar_, 2, 0);
    lv_obj_set_style_pad_right(status_bar_, 2, 0);

    network_label_ = lv_label_create(status_bar_);
    lv_label_set_text(network_label_, "");
    lv_obj_set_style_text_font(network_label_, fonts_.icon_font, 0);
    lv_obj_set_style_text_color(network_label_, current_theme.text, 0);

    notification_label_ = lv_label_create(status_bar_);
    lv_obj_set_flex_grow(notification_label_, 1);
    lv_obj_set_style_text_align(notification_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(notification_label_, current_theme.text, 0);
    lv_label_set_text(notification_label_, "");
    lv_obj_add_flag(notification_label_, LV_OBJ_FLAG_HIDDEN);

    status_label_ = lv_label_create(status_bar_);
    lv_obj_set_flex_grow(status_label_, 1);
    lv_label_set_long_mode(status_label_, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(status_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(status_label_, current_theme.text, 0);
    lv_label_set_text(status_label_, Lang::Strings::INITIALIZING);
    mute_label_ = lv_label_create(status_bar_);
    lv_label_set_text(mute_label_, "");
    lv_obj_set_style_text_font(mute_label_, fonts_.icon_font, 0);
    lv_obj_set_style_text_color(mute_label_, current_theme.text, 0);

    battery_label_ = lv_label_create(status_bar_);
    lv_label_set_text(battery_label_, "");
    lv_obj_set_style_text_font(battery_label_, fonts_.icon_font, 0);
    lv_obj_set_style_text_color(battery_label_, current_theme.text, 0);

    low_battery_popup_ = lv_obj_create(tab1);
    lv_obj_set_scrollbar_mode(low_battery_popup_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(low_battery_popup_, LV_HOR_RES * 0.9, fonts_.text_font->line_height * 2);
    lv_obj_align(low_battery_popup_, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(low_battery_popup_, current_theme.low_battery, 0);
    lv_obj_set_style_radius(low_battery_popup_, 10, 0);
    lv_obj_t* low_battery_label = lv_label_create(low_battery_popup_);
    lv_label_set_text(low_battery_label, Lang::Strings::BATTERY_NEED_CHARGE);
    lv_obj_set_style_text_color(low_battery_label, lv_color_white(), 0);
    lv_obj_center(low_battery_label);
    lv_obj_add_flag(low_battery_popup_, LV_OBJ_FLAG_HIDDEN);
}
#endif

    // Array of fortune messages
    const char* fortune_messages[100] = {
        "保持好奇心，拥抱终身学习的旅程。",
        "今天的善良将带来明天的意外之喜。",
        "勇气不是没有恐惧，而是战胜恐惧。",
        "微笑是善良的通用语言。",
        "种树最好的时间是二十年前，其次是现在。",
        "错误证明你正在尝试。",
        "每一项成就都始于尝试的决定。",
        "健康是最大的财富。",
        "困难之中蕴藏着机遇。",
        "幸福不是偶然，而是选择。",
        "真正的智慧是承认自己的无知。",
        "一个人的态度决定他的高度。",
        "坚持是通往成功的唯一道路。",
        "知识就是力量，学习永无止境。",
        "人生就像骑自行车，为了保持平衡，你必须保持前进。",
        "成功不是偶然的，而是来自于正确的习惯。",
        "宁静致远，淡泊明志。",
        "与其用泪水悔恨昨天，不如用汗水拼搏今天。",
        "一滴水只有放进大海才不会干涸。",
        "千里之行，始于足下。",
        "读万卷书，行万里路。",
        "没有人可以回到过去重新开始，但谁都可以从今天开始，书写一个全然不同的结局。",
        "不要等待机会，而要创造机会。",
        "人生如梦，岁月如歌。",
        "宁可慢一点，也不要退一步。",
        "生活不是等待风暴过去，而是学会在雨中翩翩起舞。",
        "心有多大，舞台就有多大。",
        "行动是治愈恐惧的良药。",
        "优秀的人，不是比别人更有能力，而是对自己要求更严格。",
        "积极思考造就积极人生。",
        "内心的强大，才是真正的强大。",
        "不要把时间浪费在拒绝上，把时间用在寻找机会上。",
        "你的选择比你的能力更重要。",
        "长风破浪会有时，直挂云帆济沧海。",
        "世上无难事，只要肯登攀。",
        "水滴石穿，非一日之功。",
        "感恩的心，感谢有你。",
        "永远年轻，永远热泪盈眶。",
        "人生不售来回票，一旦动身，绝不能复返。",
        "生命不息，奋斗不止。",
        "真正的快乐源于内心的平静。",
        "路是脚踏出来的，历史是人写出来的。",
        "每天进步一点点，成功不会远。",
        "态度决定高度，思路决定出路。",
        "日日行，不怕千万里；常常做，不怕千万事。",
        "少一点预设的期待，多一点真实的关怀。",
        "天行健，君子以自强不息。",
        "山不在高，有仙则名；水不在深，有龙则灵。",
        "一勤天下无难事。",
        "欲速则不达，见小利则大事不成。",
        "凡事预则立，不预则废。",
        "尽人事，听天命。",
        "人无远虑，必有近忧。",
        "修身齐家治国平天下。",
        "万事开头难。",
        "知己知彼，百战不殆。",
        "己所不欲，勿施于人。",
        "三思而后行。",
        "学无止境。",
        "温故而知新。",
        "言必信，行必果。",
        "勿以恶小而为之，勿以善小而不为。",
        "古之立大事者，不惟有超世之才，亦必有坚忍不拔之志。",
        "生活中最美好的事情，都是免费的。",
        "真正的财富是健康的身体和充实的精神。",
        "成功的秘诀在于坚持自己的目标不动摇。",
        "成长的过程就是不断发现自己的无知。",
        "世间最宝贵的是今天，最易丧失的也是今天。",
        "不要让过去的阴影遮住了现在的阳光。",
        "人生路上，每一步都算数。",
        "脚踏实地地走，比一味空想要好。",
        "人生就像一杯茶，不会苦一辈子，但总会苦一阵子。",
        "当你能够微笑面对人生时，你就战胜了人生。",
        "把每一个挑战当作成长的机会。",
        "人生没有彩排，每一天都是现场直播。",
        "别让昨天占据了太多的今天。",
        "每一个今天都是未来回忆里最美好的昨天。",
        "珍惜所有的不期而遇，看淡所有的不辞而别。",
        "生活的真谛不在于拥有的多少，而在于感受的深度。",
        "成功不是将来才有的，而是从决定去做的那一刻起，持续累积而成。",
        "别因为结果未知而害怕开始。",
        "简单生活，简单心境，简单欢乐。",
        "心若向阳，无谓悲伤。",
        "岁月静好，现世安稳。",
        "心若不动，风又奈何。",
        "生活不是只有温暖，还有风雨。",
        "适当放松，才能更好前行。",
        "和气致祥，忍气生财。",
        "吾日三省吾身。",
        "活在当下，珍惜眼前。",
        "人生如棋，落子无悔。",
        "人生不过三万天，何必贪恋人间烟火。",
        "心若计较，处处都有怨言；心若放宽，时时都是春天。",
        "厚德载物，自强不息。",
        "静以修身，俭以养德。",
        "书山有路勤为径，学海无涯苦作舟。",
        "谦虚是通往真理之门。",
        "内不欺己，外不欺人。",
        "善始善终，善做善成。",
        "事常与人违，事总在人为。"
    };

    void SetupTab2() {
        lv_obj_set_style_text_font(tab2, fonts_.text_font, 0);
        lv_obj_set_style_text_color(tab2, current_theme.text, 0);
        lv_obj_set_style_bg_color(tab2, current_theme.background, 0);

        /* Background image for tab2 */
        bg_img2 = lv_img_create(tab2);
    
        /* Set the image descriptor */
        lv_img_set_src(bg_img2, &bg1);
    
        /* Position the image to cover the entire tab */
        lv_obj_set_size(bg_img2, LV_HOR_RES, LV_VER_RES);
        lv_obj_set_pos(bg_img2, -16, -16);
    
        /* Send to background */
        lv_obj_move_background(bg_img2);
        
        // Create temperature display area - 放在顶部位置
        lv_obj_t *temp_container = lv_obj_create(tab2);
        lv_obj_set_size(temp_container, LV_HOR_RES * 0.8 - 80, 60);  // 修改高度为60
        // 调整天气容器位置，放在时间标签上方10像素
        lv_obj_align(temp_container, LV_ALIGN_BOTTOM_MID, 0, -20 - 60 - 10 - 60 - 10);  // 格言容器高度60，下边距20，间隔10，时间高度60，间隔10
        lv_obj_set_style_radius(temp_container, 10, 0);
        lv_obj_set_style_bg_opa(temp_container, LV_OPA_30, 0);
        lv_obj_set_style_border_width(temp_container, 0, 0);
        // 禁用滚动
        lv_obj_clear_flag(temp_container, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(temp_container, LV_SCROLLBAR_MODE_OFF);
        
        lv_obj_t *temp_label = lv_label_create(temp_container);
        lv_obj_set_style_text_font(temp_label, fonts_.text_font, 0);
        lv_label_set_text(temp_label, "正在获取天气信息...");
        lv_obj_set_width(temp_label, LV_HOR_RES * 0.8 - 40 + 60 - 20);  // 设置标签宽度
        lv_label_set_long_mode(temp_label, LV_LABEL_LONG_WRAP);  // 允许文本换行
        lv_obj_set_style_text_align(temp_label, LV_TEXT_ALIGN_CENTER, 0);  // 文本居中对齐
        lv_obj_center(temp_label);
        
        // 保存到全局静态变量，以便任务函数访问
        s_weather_temp_label = temp_label;
        
        // 创建并启动天气任务
        create_weather_task();
        
        // 立即执行一次获取天气信息
        fetch_weather();
        
        // Create time display area with large font - 放在中间位置
        lv_obj_t *time_container = lv_obj_create(tab2);
        lv_obj_set_size(time_container, LV_HOR_RES * 0.8 - 80, 60);
        // 调整时间容器位置，放在格言标签上方10像素
        lv_obj_align(time_container, LV_ALIGN_BOTTOM_MID, 0, -20 - 60 - 10);  // 格言容器高度60，下边距20，间隔10
        lv_obj_set_style_radius(time_container, 10, 0);
        lv_obj_set_style_bg_opa(time_container, LV_OPA_30, 0);
        lv_obj_set_style_border_width(time_container, 0, 0);
        // 禁用滚动
        lv_obj_clear_flag(time_container, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(time_container, LV_SCROLLBAR_MODE_OFF);
        
        lv_obj_t *time_label = lv_label_create(time_container);
        lv_obj_set_style_text_font(time_label, &time40 , 0); // Using larger font for time
        lv_label_set_text(time_label, "00:00:00");
        lv_obj_center(time_label);
        
        // Create fortune message box at the bottom
        lv_obj_t *fortune_container = lv_obj_create(tab2);
        lv_obj_set_size(fortune_container, LV_HOR_RES * 0.8 - 80, 60);  // 修改高度为60
        lv_obj_align(fortune_container, LV_ALIGN_BOTTOM_MID, 0, -20);
        lv_obj_set_style_radius(fortune_container, 10, 0);
        lv_obj_set_style_bg_opa(fortune_container, LV_OPA_30, 0);
        lv_obj_set_style_border_width(fortune_container, 0, 0);
        // 禁用滚动
        lv_obj_clear_flag(fortune_container, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(fortune_container, LV_SCROLLBAR_MODE_OFF);
        
        lv_obj_t *fortune_label = lv_label_create(fortune_container);
        lv_obj_set_style_text_font(fortune_label, fonts_.text_font, 0);
        lv_obj_set_width(fortune_label, LV_HOR_RES * 0.7 - 40);
        lv_label_set_long_mode(fortune_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_align(fortune_label, LV_TEXT_ALIGN_CENTER, 0);
        
        // Select a random fortune message
        srand(time(NULL)); // Initialize random seed with current time
        int random_index = rand() % 100;
        lv_label_set_text(fortune_label, fortune_messages[random_index]);
        lv_obj_center(fortune_label);
        
        /* Create background switch button (left side) */
        lv_obj_t *bg_switch_btn2 = lv_btn_create(tab2);
        lv_obj_set_size(bg_switch_btn2, 60, 60);  // Same size as tab1 button
        // 位置调整为垂直居中，与tab1的按钮Y轴位置一致
        lv_obj_align(bg_switch_btn2, LV_ALIGN_LEFT_MID, 2, 0);  // 左侧中间位置
        lv_obj_set_style_bg_opa(bg_switch_btn2, LV_OPA_50, 0);    // Semi-transparent
        lv_obj_set_style_radius(bg_switch_btn2, 30, 0);           // Round corners
        
        lv_obj_t *btn_label = lv_label_create(bg_switch_btn2);
        lv_label_set_text(btn_label, FONT_AWESOME_ARROW_RIGHT);  // Use arrow icon
        lv_obj_set_style_text_font(btn_label, fonts_.icon_font, 0);
        lv_obj_center(btn_label);
        
        // Add click event handler for the background switch button
        lv_obj_add_event_cb(bg_switch_btn2, [](lv_event_t *e) {
            CustomLcdDisplay *display = (CustomLcdDisplay *)lv_event_get_user_data(e);
            if (!display) return;
            
            // Cycle through backgrounds
            display->bg_index = (display->bg_index % 4) + 1;
            
            // Set the new background image
            switch (display->bg_index) {
                case 1: lv_img_set_src(display->bg_img2, &bg1); break;
                case 2: lv_img_set_src(display->bg_img2, &bg2); break;
                case 3: lv_img_set_src(display->bg_img2, &bg3); break;
                case 4: lv_img_set_src(display->bg_img2, &bg4); break;
            }
        }, LV_EVENT_CLICKED, this);
        
        /* Create return button (right side) */
        lv_obj_t *return_btn = lv_btn_create(tab2);
        lv_obj_set_size(return_btn, 60, 60);  // Same size as tab1 button
        // 位置调整为垂直居中，与tab1的按钮Y轴位置一致
        lv_obj_align(return_btn, LV_ALIGN_RIGHT_MID, -2, 0);  // 右侧中间位置
        lv_obj_set_style_bg_opa(return_btn, LV_OPA_50, 0);    // Semi-transparent
        lv_obj_set_style_radius(return_btn, 30, 0);           // Round corners
        
        lv_obj_t *return_btn_label = lv_label_create(return_btn);
        lv_label_set_text(return_btn_label, "<");  // Use simple left arrow text
        lv_obj_set_style_text_font(return_btn_label, fonts_.text_font, 0);
        lv_obj_center(return_btn_label);
        
        // Add click event handler for the return button
        lv_obj_add_event_cb(return_btn, [](lv_event_t *e) {
            CustomLcdDisplay *display = (CustomLcdDisplay *)lv_event_get_user_data(e);
            if (!display) return;
            
            // Find the tabview and switch to tab1
            lv_obj_t *tabview = lv_obj_get_parent(lv_obj_get_parent(display->tab2));
            if (tabview) {
                lv_tabview_set_act(tabview, 0, LV_ANIM_ON);
            }
        }, LV_EVENT_CLICKED, this);
        
        // Start timer to update time
        static lv_obj_t* time_cont = time_container; // Store container in static variable
        lv_timer_create([](lv_timer_t *t) {
            if (!time_cont) return;
            
            // Get current time
            time_t now;
            struct tm timeinfo;
            time(&now);
            localtime_r(&now, &timeinfo);
            
            // Format time as HH:MM:SS
            char time_str[9]; // Increased buffer size for HH:MM:SS format
            sprintf(time_str, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            
            // Update the time label
            lv_obj_t *time_label = lv_obj_get_child(time_cont, 0);
            if (time_label) {
                lv_label_set_text(time_label, time_str);
            }
        }, 1000, NULL);  // No need for user_data now
    }

    virtual void SetupUI() override {
        DisplayLockGuard lock(this);
        ESP_LOGI(TAG, "SetupUI --------------------------------------");
        // 创建tabview，填充整个屏幕
        lv_obj_t * tabview = lv_tabview_create(lv_scr_act());
        lv_obj_set_size(tabview, lv_pct(100), lv_pct(100));

        
        // 隐藏标签栏
        lv_tabview_set_tab_bar_position(tabview, LV_DIR_TOP);
        lv_tabview_set_tab_bar_size(tabview, 0);
        lv_obj_t * tab_btns = lv_tabview_get_tab_btns(tabview);
        lv_obj_add_flag(tab_btns, LV_OBJ_FLAG_HIDDEN);

         // 设置tabview的滚动捕捉模式，确保滑动后停留在固定位置
        lv_obj_t * content = lv_tabview_get_content(tabview);
        lv_obj_set_scroll_snap_x(content, LV_SCROLL_SNAP_CENTER);
        
        // 创建两个页面
        tab1 = lv_tabview_add_tab(tabview, "Tab1");
        tab2 = lv_tabview_add_tab(tabview, "Tab2");

        // Disable scrolling for tab1
        lv_obj_clear_flag(tab1, LV_OBJ_FLAG_SCROLLABLE);
        // Hide scrollbar for tab1
        lv_obj_set_scrollbar_mode(tab1, LV_SCROLLBAR_MODE_OFF);
        
        // Disable scrolling for tab2
        lv_obj_clear_flag(tab2, LV_OBJ_FLAG_SCROLLABLE);
        // Hide scrollbar for tab2
        lv_obj_set_scrollbar_mode(tab2, LV_SCROLLBAR_MODE_OFF);

        // Add click event handlers for both tabs
        lv_obj_add_event_cb(tab1, [](lv_event_t *e) {
            CustomLcdDisplay *display = (CustomLcdDisplay *)lv_event_get_user_data(e);
            if (!display) return;
            
            // If there's an active timer, delete it
            if (display->idle_timer_ != nullptr) {
                lv_timer_del(display->idle_timer_);
                display->idle_timer_ = nullptr;
            }
        }, LV_EVENT_CLICKED, this);

        lv_obj_add_event_cb(tab2, [](lv_event_t *e) {
            CustomLcdDisplay *display = (CustomLcdDisplay *)lv_event_get_user_data(e);
            if (!display) return;
            
            // If there's an active timer, delete it
            if (display->idle_timer_ != nullptr) {
                lv_timer_del(display->idle_timer_);
                display->idle_timer_ = nullptr;
            }
        }, LV_EVENT_CLICKED, this);

        // 在第一个页面中保持原有内容
        SetupTab1();
        SetupTab2();
    }

    virtual void SetTheme(const std::string& theme_name) override {
    DisplayLockGuard lock(this);
    
    if (theme_name == "dark" || theme_name == "DARK") {
        current_theme = DARK_THEME;
    } else if (theme_name == "light" || theme_name == "LIGHT") {
        current_theme = LIGHT_THEME;
    } else {
        // Invalid theme name, return false
        ESP_LOGE(TAG, "Invalid theme name: %s", theme_name.c_str());
        return;
    }
    
    // Get the active screen
    lv_obj_t* screen = lv_screen_active();
    
    // Update the screen colors
    lv_obj_set_style_bg_color(screen, current_theme.background, 0);
    lv_obj_set_style_text_color(screen, current_theme.text, 0);
    
    // Update container colors
    if (container_ != nullptr) {
        lv_obj_set_style_bg_color(container_, current_theme.background, 0);
        lv_obj_set_style_border_color(container_, current_theme.border, 0);
    }
    
    // Update status bar colors
    if (status_bar_ != nullptr) {
        lv_obj_set_style_bg_color(status_bar_, current_theme.background, 0);
        lv_obj_set_style_text_color(status_bar_, current_theme.text, 0);
        
        // Update status bar elements
        if (network_label_ != nullptr) {
            lv_obj_set_style_text_color(network_label_, current_theme.text, 0);
        }
        if (status_label_ != nullptr) {
            lv_obj_set_style_text_color(status_label_, current_theme.text, 0);
        }
        if (notification_label_ != nullptr) {
            lv_obj_set_style_text_color(notification_label_, current_theme.text, 0);
        }
        if (mute_label_ != nullptr) {
            lv_obj_set_style_text_color(mute_label_, current_theme.text, 0);
        }
        if (battery_label_ != nullptr) {
            lv_obj_set_style_text_color(battery_label_, current_theme.text, 0);
        }
        if (emotion_label_ != nullptr) {
            lv_obj_set_style_text_color(emotion_label_, current_theme.text, 0);
        }
    }
    
    // Update content area colors
    if (content_ != nullptr) {
        lv_obj_set_style_bg_color(content_, current_theme.chat_background, 0);
        lv_obj_set_style_border_color(content_, current_theme.border, 0);
        
        // If we have the chat message style, update all message bubbles
#if CONFIG_USE_WECHAT_MESSAGE_STYLE
        // Iterate through all children of content (message containers or bubbles)
        uint32_t child_count = lv_obj_get_child_cnt(content_);
        for (uint32_t i = 0; i < child_count; i++) {
            lv_obj_t* obj = lv_obj_get_child(content_, i);
            if (obj == nullptr) continue;
            
            lv_obj_t* bubble = nullptr;
            
            // 检查这个对象是容器还是气泡
            // 如果是容器（用户或系统消息），则获取其子对象作为气泡
            // 如果是气泡（助手消息），则直接使用
            if (lv_obj_get_child_cnt(obj) > 0) {
                // 可能是容器，检查它是否为用户或系统消息容器
                // 用户和系统消息容器是透明的
                lv_opa_t bg_opa = lv_obj_get_style_bg_opa(obj, 0);
                if (bg_opa == LV_OPA_TRANSP) {
                    // 这是用户或系统消息的容器
                    bubble = lv_obj_get_child(obj, 0);
                } else {
                    // 这可能是助手消息的气泡自身
                    bubble = obj;
                }
            } else {
                // 没有子元素，可能是其他UI元素，跳过
                continue;
            }
            
            if (bubble == nullptr) continue;
            
            // 使用保存的用户数据来识别气泡类型
            void* bubble_type_ptr = lv_obj_get_user_data(bubble);
            if (bubble_type_ptr != nullptr) {
                const char* bubble_type = static_cast<const char*>(bubble_type_ptr);
                
                // 根据气泡类型应用正确的颜色
                if (strcmp(bubble_type, "user") == 0) {
                    lv_obj_set_style_bg_color(bubble, current_theme.user_bubble, 0);
                } else if (strcmp(bubble_type, "assistant") == 0) {
                    lv_obj_set_style_bg_color(bubble, current_theme.assistant_bubble, 0); 
                } else if (strcmp(bubble_type, "system") == 0) {
                    lv_obj_set_style_bg_color(bubble, current_theme.system_bubble, 0);
                }
                
                // Update border color
                lv_obj_set_style_border_color(bubble, current_theme.border, 0);
                
                // Update text color for the message
                if (lv_obj_get_child_cnt(bubble) > 0) {
                    lv_obj_t* text = lv_obj_get_child(bubble, 0);
                    if (text != nullptr) {
                        // 根据气泡类型设置文本颜色
                        if (strcmp(bubble_type, "system") == 0) {
                            lv_obj_set_style_text_color(text, current_theme.system_text, 0);
                        } else {
                            lv_obj_set_style_text_color(text, current_theme.text, 0);
                        }
                    }
                }
            } else {
                // 如果没有标记，回退到之前的逻辑（颜色比较）
                // ...保留原有的回退逻辑...
                lv_color_t bg_color = lv_obj_get_style_bg_color(bubble, 0);
            
                // 改进bubble类型检测逻辑，不仅使用颜色比较
                bool is_user_bubble = false;
                bool is_assistant_bubble = false;
                bool is_system_bubble = false;
            
                // 检查用户bubble
                if (lv_color_eq(bg_color, DARK_USER_BUBBLE_COLOR) || 
                    lv_color_eq(bg_color, LIGHT_USER_BUBBLE_COLOR) ||
                    lv_color_eq(bg_color, current_theme.user_bubble)) {
                    is_user_bubble = true;
                }
                // 检查系统bubble
                else if (lv_color_eq(bg_color, DARK_SYSTEM_BUBBLE_COLOR) || 
                         lv_color_eq(bg_color, LIGHT_SYSTEM_BUBBLE_COLOR) ||
                         lv_color_eq(bg_color, current_theme.system_bubble)) {
                    is_system_bubble = true;
                }
                // 剩余的都当作助手bubble处理
                else {
                    is_assistant_bubble = true;
                }
            
                // 根据bubble类型应用正确的颜色
                if (is_user_bubble) {
                    lv_obj_set_style_bg_color(bubble, current_theme.user_bubble, 0);
                } else if (is_assistant_bubble) {
                    lv_obj_set_style_bg_color(bubble, current_theme.assistant_bubble, 0);
                } else if (is_system_bubble) {
                    lv_obj_set_style_bg_color(bubble, current_theme.system_bubble, 0);
                }
                
                // Update border color
                lv_obj_set_style_border_color(bubble, current_theme.border, 0);
                
                // Update text color for the message
                if (lv_obj_get_child_cnt(bubble) > 0) {
                    lv_obj_t* text = lv_obj_get_child(bubble, 0);
                    if (text != nullptr) {
                        // 回退到颜色检测逻辑
                        if (lv_color_eq(bg_color, current_theme.system_bubble) ||
                            lv_color_eq(bg_color, DARK_SYSTEM_BUBBLE_COLOR) || 
                            lv_color_eq(bg_color, LIGHT_SYSTEM_BUBBLE_COLOR)) {
                            lv_obj_set_style_text_color(text, current_theme.system_text, 0);
                        } else {
                            lv_obj_set_style_text_color(text, current_theme.text, 0);
                        }
                    }
                }
            }
        }
#else
        // Simple UI mode - just update the main chat message
        if (chat_message_label_ != nullptr) {
            lv_obj_set_style_text_color(chat_message_label_, current_theme.text, 0);
        }
        
        if (emotion_label_ != nullptr) {
            lv_obj_set_style_text_color(emotion_label_, current_theme.text, 0);
        }
#endif
    }
    
    // Update low battery popup
    if (low_battery_popup_ != nullptr) {
        lv_obj_set_style_bg_color(low_battery_popup_, current_theme.low_battery, 0);
    }

    // No errors occurred. Save theme to settings
    Display::SetTheme(theme_name);
}
    
    // 注册触摸输入 - 接受触摸数据结构
    void register_touch(SPD2010TouchDriver *touch_driver, int width, int height, bool swap_xy, bool invert_x, bool invert_y) {
        DisplayLockGuard lock(this);
        touch_driver_ = touch_driver;
        // 创建LVGL输入设备
        lv_indev_t *indev = lv_indev_create();
        if (indev == nullptr) {
            ESP_LOGE(TAG, "创建触摸输入设备失败");
            return;
        }
        
        // 设置输入设备属性
        lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_disp(indev, display_);
        
        // 使用静态函数作为回调
        lv_indev_set_read_cb(indev, read_touch_cb);
        
        // 存储this指针作为用户数据，以便静态回调函数访问
        lv_indev_set_user_data(indev, this);
        
        ESP_LOGI(TAG, "触摸屏注册成功");
    }

private:
    // 静态回调函数
    static void read_touch_cb(lv_indev_t *indev, lv_indev_data_t *data) {
        // 从用户数据获取this指针
        CustomLcdDisplay *self = (CustomLcdDisplay *)lv_indev_get_user_data(indev);
        if (!self || !self->touch_driver_) return;
        
        auto touch_data = self->touch_driver_->get_touch_data();
        if (!touch_data) return;
        
        if (touch_data->touch_num > 0) {
            data->point.x = touch_data->points[0].x;
            data->point.y = touch_data->points[0].y;
            data->state = LV_INDEV_STATE_PRESSED;
        } else {
            data->state = LV_INDEV_STATE_RELEASED;
        }
    }
};

class CustomBoard : public WifiBoard {
private:
    i2c_master_bus_handle_t i2c_bus_;
    esp_io_expander_handle_t io_expander = NULL;
    CustomLcdDisplay* display_;
    Button boot_btn, pwr_btn;
    PowerManager* power_manager_;
    PowerSaveTimer* power_save_timer_;
    esp_lcd_panel_io_handle_t panel_io_ = nullptr;
    esp_lcd_panel_handle_t panel_ = nullptr;
    // 触摸驱动
    SPD2010TouchDriver* touch_driver_ = nullptr;

    void InitializePowerManager() {
        power_manager_ = new PowerManager(GPIO_NUM_8);
        power_manager_->OnChargingStatusChanged([this](bool is_charging) {  
            if (is_charging) {
                power_save_timer_->SetEnabled(false);
            } else {
                power_save_timer_->SetEnabled(true);
            }
        });
    }

    void InitializePowerSaveTimer() {
        power_save_timer_ = new PowerSaveTimer(-1, 60, 300);
        power_save_timer_->OnEnterSleepMode([this]() {
            ESP_LOGI(TAG, "Enabling sleep mode");
            display_->SetChatMessage("system", "");
            display_->SetEmotion("sleepy");
            GetBacklight()->SetBrightness(0);
        });
        power_save_timer_->OnExitSleepMode([this]() {
            display_->SetChatMessage("system", "");
            display_->SetEmotion("neutral");
            GetBacklight()->RestoreBrightness();
        });
        power_save_timer_->OnShutdownRequest([this]() {
            ESP_LOGI(TAG, "Shutting down");
            esp_lcd_panel_disp_on_off(panel_, false); //关闭显示
            esp_deep_sleep_start();
        });
        power_save_timer_->SetEnabled(true);
    }

    void InitializeI2c() {
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = (i2c_port_t)0,
            .sda_io_num = I2C_SDA_IO,
            .scl_io_num = I2C_SCL_IO,
            .clk_source = I2C_CLK_SRC_DEFAULT,
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &i2c_bus_));
    }
    
    void InitializeTca9554(void) {
        esp_err_t ret = esp_io_expander_new_i2c_tca9554(i2c_bus_, I2C_ADDRESS, &io_expander);
        if(ret != ESP_OK)
            ESP_LOGE(TAG, "TCA9554 create returned error");        

        ret = esp_io_expander_set_dir(io_expander, IO_EXPANDER_PIN_NUM_0 | IO_EXPANDER_PIN_NUM_1, IO_EXPANDER_OUTPUT);                
        ESP_ERROR_CHECK(ret);
        ret = esp_io_expander_set_level(io_expander, IO_EXPANDER_PIN_NUM_0 | IO_EXPANDER_PIN_NUM_1, 1);                                
        ESP_ERROR_CHECK(ret);
        vTaskDelay(pdMS_TO_TICKS(300));
        ret = esp_io_expander_set_level(io_expander, IO_EXPANDER_PIN_NUM_0 | IO_EXPANDER_PIN_NUM_1, 0);                                
        ESP_ERROR_CHECK(ret);
        vTaskDelay(pdMS_TO_TICKS(300));
        ret = esp_io_expander_set_level(io_expander, IO_EXPANDER_PIN_NUM_0 | IO_EXPANDER_PIN_NUM_1, 1);                                
        ESP_ERROR_CHECK(ret);
    }

    void InitializeSpi() {
        ESP_LOGI(TAG, "Initialize QSPI bus");

        const spi_bus_config_t bus_config = TAIJIPI_SPD2010_PANEL_BUS_QSPI_CONFIG(QSPI_PIN_NUM_LCD_PCLK,
                                                                        QSPI_PIN_NUM_LCD_DATA0,
                                                                        QSPI_PIN_NUM_LCD_DATA1,
                                                                        QSPI_PIN_NUM_LCD_DATA2,
                                                                        QSPI_PIN_NUM_LCD_DATA3,
                                                                        QSPI_LCD_H_RES * 80 * sizeof(uint16_t));
        ESP_ERROR_CHECK(spi_bus_initialize(QSPI_LCD_HOST, &bus_config, SPI_DMA_CH_AUTO));
    }

    void Initializespd2010Display() {
        //esp_lcd_panel_io_handle_t panel_io = nullptr;
        // esp_lcd_panel_handle_t panel = nullptr;

        ESP_LOGI(TAG, "Install panel IO");
        
        const esp_lcd_panel_io_spi_config_t io_config = SPD2010_PANEL_IO_QSPI_CONFIG(QSPI_PIN_NUM_LCD_CS, NULL, NULL);
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)QSPI_LCD_HOST, &io_config, &panel_io_));

        ESP_LOGI(TAG, "Install SPD2010 panel driver");
        
        spd2010_vendor_config_t vendor_config = {
            .flags = {
                .use_qspi_interface = 1,
            },
        };
        const esp_lcd_panel_dev_config_t panel_config = {
            .reset_gpio_num = QSPI_PIN_NUM_LCD_RST,
            .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,     
            .bits_per_pixel = QSPI_LCD_BIT_PER_PIXEL,    
            .vendor_config = &vendor_config,
        };
        ESP_ERROR_CHECK(esp_lcd_new_panel_spd2010(panel_io_, &panel_config, &panel_));

        esp_lcd_panel_reset(panel_);
        esp_lcd_panel_init(panel_);
        esp_lcd_panel_disp_on_off(panel_, true);
        esp_lcd_panel_swap_xy(panel_, DISPLAY_SWAP_XY);
        esp_lcd_panel_mirror(panel_, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);
        display_ = new CustomLcdDisplay(panel_io_, panel_,
                                    DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
    }
    
    // 初始化触摸屏 - 使用新的触摸驱动
    void InitializeTouch() {
        ESP_LOGI(TAG, "初始化SPD2010触摸屏");
        
        // 创建触摸驱动实例
        touch_driver_ = new SPD2010TouchDriver(i2c_bus_);
        
        // 初始化触摸驱动
        esp_err_t ret = touch_driver_->initialize();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "触摸屏初始化失败: %d", ret);
            delete touch_driver_;
            touch_driver_ = nullptr;
            return;
        }
        
        // 将触摸屏注册到显示器
        if (display_ != nullptr && touch_driver_->is_initialized()) {
            display_->register_touch(touch_driver_, DISPLAY_WIDTH, DISPLAY_HEIGHT, 
                                    DISPLAY_SWAP_XY, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);
            
            // 启动触摸轮询任务
            //touch_driver_->start_touch_poll_task(this);
        }
    }
 
    void InitializeButtonsCustom() {
        gpio_reset_pin(BOOT_BUTTON_GPIO);                                     
        gpio_set_direction(BOOT_BUTTON_GPIO, GPIO_MODE_INPUT);   
        gpio_reset_pin(PWR_BUTTON_GPIO);                                     
        gpio_set_direction(PWR_BUTTON_GPIO, GPIO_MODE_INPUT);   
        gpio_reset_pin(PWR_Control_PIN);                                     
        gpio_set_direction(PWR_Control_PIN, GPIO_MODE_OUTPUT);     
        gpio_set_level(PWR_Control_PIN, true);
    }
    void InitializeButtons() {
        boot_btn.OnClick([this]() {
            power_save_timer_->WakeUp();
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting && !WifiStation::GetInstance().IsConnected()) {
                ResetWifiConfiguration();
            }
            app.ToggleChatState();
        });

        pwr_btn.OnClick([this]() {  
            power_save_timer_->WakeUp();
        });

        pwr_btn.OnLongPress([this]() {  
            if(GetBacklight()->brightness() > 0) {
                GetBacklight()->SetBrightness(0);
                gpio_set_level(PWR_Control_PIN, false);
            }
            else {
                GetBacklight()->RestoreBrightness();
                gpio_set_level(PWR_Control_PIN, true);
            }
        });
#if 0

        InitializeButtonsCustom();
        button_config_t btns_config = {
            .type = BUTTON_TYPE_CUSTOM,
            .long_press_time = 2000,
            .short_press_time = 50,
            .custom_button_config = {
                .active_level = 0,
                .button_custom_init = nullptr,
                .button_custom_get_key_value = [](void *param) -> uint8_t {
                    return gpio_get_level(BOOT_BUTTON_GPIO);
                },
                .button_custom_deinit = nullptr,
                .priv = this,
            },
        };
        boot_btn = iot_button_create(&btns_config);
        iot_button_register_cb(boot_btn, BUTTON_SINGLE_CLICK, [](void* button_handle, void* usr_data) {
            power_save_timer_->WakeUp();
            auto self = static_cast<CustomBoard*>(usr_data);
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting && !WifiStation::GetInstance().IsConnected()) {
                self->ResetWifiConfiguration();
            }
            app.ToggleChatState();
        }, this);
        iot_button_register_cb(boot_btn, BUTTON_LONG_PRESS_START, [](void* button_handle, void* usr_data) {
        }, this);

        btns_config.long_press_time = 5000;
        btns_config.custom_button_config.button_custom_get_key_value = [](void *param) -> uint8_t {
            return gpio_get_level(PWR_BUTTON_GPIO);
        };
        pwr_btn = iot_button_create(&btns_config);
        iot_button_register_cb(pwr_btn, BUTTON_SINGLE_CLICK, [](void* button_handle, void* usr_data) {
            power_save_timer_->WakeUp();
        }, this);
        iot_button_register_cb(pwr_btn, BUTTON_LONG_PRESS_START, [](void* button_handle, void* usr_data) {
            auto self = static_cast<CustomBoard*>(usr_data);
            if(self->GetBacklight()->brightness() > 0) {
                self->GetBacklight()->SetBrightness(0);
                gpio_set_level(PWR_Control_PIN, false);
            }
            else {
                self->GetBacklight()->RestoreBrightness();
                gpio_set_level(PWR_Control_PIN, true);
            }
        }, this);
#endif
    }

    void InitializeIot() {
        auto& thing_manager = iot::ThingManager::GetInstance();
        thing_manager.AddThing(iot::CreateThing("Speaker"));
        thing_manager.AddThing(iot::CreateThing("Screen"));
    }

public:
    CustomBoard() : boot_btn(BOOT_BUTTON_GPIO), pwr_btn(PWR_BUTTON_GPIO) { 
        InitializeI2c();
        InitializeTca9554();
        InitializePowerManager();
        InitializePowerSaveTimer();
        InitializeSpi();
        Initializespd2010Display();
        InitializeTouch();
        InitializeButtons();
        InitializeIot();
        GetBacklight()->RestoreBrightness();
    }

    virtual AudioCodec* GetAudioCodec() override {
        static NoAudioCodecSimplex audio_codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_SPK_GPIO_BCLK, AUDIO_I2S_SPK_GPIO_LRCK, AUDIO_I2S_SPK_GPIO_DOUT, I2S_STD_SLOT_LEFT, AUDIO_I2S_MIC_GPIO_SCK, AUDIO_I2S_MIC_GPIO_WS, AUDIO_I2S_MIC_GPIO_DIN, I2S_STD_SLOT_RIGHT);

        return &audio_codec;
    }

    virtual Display* GetDisplay() override {
        return display_;
    }
    
    virtual Backlight* GetBacklight() override {
        static PwmBacklight backlight(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT);
        return &backlight;
    }

    virtual bool GetBatteryLevel(int& level, bool& charging, bool& discharging) override {
        static bool last_discharging = false;
        charging = power_manager_->IsCharging();
        discharging = power_manager_->IsDischarging();
        if (discharging != last_discharging) {
            power_save_timer_->SetEnabled(discharging);
            last_discharging = discharging;
        }
        level = power_manager_->GetBatteryLevel();
        return true;
    }

    virtual void SetPowerSaveMode(bool enabled) override {
        if (!enabled) {
            power_save_timer_->WakeUp();
        }
        WifiBoard::SetPowerSaveMode(enabled);
    }

    ~CustomBoard() {
        // 释放触摸驱动资源
        if (touch_driver_ != nullptr) {
            delete touch_driver_;
            touch_driver_ = nullptr;
        }
    }
};

DECLARE_BOARD(CustomBoard);
