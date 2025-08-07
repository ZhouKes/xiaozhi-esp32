#include "lvgl.h"
#include "custom_display.h"
#include "core/lv_obj_pos.h"
#include "font/lv_font.h"
#include "misc/lv_types.h"
#include "misc/lv_timer.h"
#include <string>
#include <esp_log.h>
#include <font_awesome_symbols.h>
#include "assets/lang_config.h"
#include "settings.h"
#include <string.h>
#define TAG "CustomLcdDisplay"



LV_IMG_DECLARE(angry_00);
LV_IMG_DECLARE(angry_01);
LV_IMG_DECLARE(angry_02);
LV_IMG_DECLARE(angry_03);
LV_IMG_DECLARE(angry_04);
LV_IMG_DECLARE(angry_05);
LV_IMG_DECLARE(angry_06);
LV_IMG_DECLARE(angry_07);
LV_IMG_DECLARE(angry_08);
LV_IMG_DECLARE(angry_09);



const lv_image_dsc_t* angry_gif[] = {
    &angry_00,
    &angry_01,
    &angry_02,
    &angry_03,
    &angry_04,
    &angry_05,
    &angry_06,
    &angry_07,
    &angry_08,
    &angry_09,
};

 
LV_FONT_DECLARE(font_puhui_20_4);
LV_FONT_DECLARE(font_awesome_20_4);
LV_FONT_DECLARE(font_awesome_30_4);
 



// Color definitions for dark theme
#define DARK_BACKGROUND_COLOR       lv_color_hex(0)     // Dark background
#define DARK_TEXT_COLOR             lv_color_white()           // White text
#define DARK_CHAT_BACKGROUND_COLOR  lv_color_hex(0)     // Slightly lighter than background
#define DARK_USER_BUBBLE_COLOR      lv_color_hex(0x1A6C37)     // Dark green
#define DARK_ASSISTANT_BUBBLE_COLOR lv_color_hex(0x333333)     // Dark gray
#define DARK_SYSTEM_BUBBLE_COLOR    lv_color_hex(0x2A2A2A)     // Medium gray
#define DARK_SYSTEM_TEXT_COLOR      lv_color_hex(0xAAAAAA)     // Light gray text
#define DARK_BORDER_COLOR           lv_color_hex(0)     // Dark gray border
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
 
// Current theme - initialize based on default config
static ThemeColors current_theme = DARK_THEME;

// 构造函数实现
static CustomLcdDisplay* s_instance = nullptr;
CustomLcdDisplay::CustomLcdDisplay(esp_lcd_panel_io_handle_t io_handle, 
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
                    .text_font = &font_puhui_20_4,
                    .icon_font = &font_awesome_20_4,
                    .emoji_font = font_emoji_32_init(),
                }) {
    DisplayLockGuard lock(this);
    s_instance = this;
    SetupUI();
}

// SetIdle方法实现
void CustomLcdDisplay::SetIdle(bool status) {
    if (status) {
 
    } else {
 
    }
}

 
void CustomLcdDisplay::SetChatMessage(const char* role, const char* content){
    DisplayLockGuard lock(this);
    if (chat_message_label_ == nullptr) {
        return;
    }
    lv_label_set_text(chat_message_label_, content);
 
}

 
void CustomLcdDisplay::SetupUI() {
    ESP_LOGI(TAG, "SetupUI IN CUSTOM LCD DISPLAY");
 DisplayLockGuard lock(this);

    auto screen = lv_screen_active();
    lv_obj_set_style_text_font(screen, fonts_.text_font, 0);
    lv_obj_set_style_text_color(screen, current_theme_.text, 0);
    lv_obj_set_style_bg_color(screen, current_theme_.background, 0);

    /* Container */
    container_ = lv_obj_create(screen);
    lv_obj_set_size(container_, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_flex_flow(container_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(container_, 0, 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_row(container_, 0, 0);
    lv_obj_set_style_bg_color(container_, current_theme_.background, 0);
    lv_obj_set_style_border_color(container_, current_theme_.border, 0);

    /* Status bar */
    status_bar_ = lv_obj_create(container_);
    lv_obj_set_size(status_bar_, LV_HOR_RES, fonts_.text_font->line_height);
    lv_obj_set_style_radius(status_bar_, 0, 0);
    lv_obj_set_style_bg_color(status_bar_, current_theme_.background, 0);
    lv_obj_set_style_text_color(status_bar_, current_theme_.text, 0);
    
    /* Content */
    content_ = lv_obj_create(container_);
    lv_obj_set_scrollbar_mode(content_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(content_, 0, 0);
    lv_obj_set_width(content_, LV_HOR_RES);
    lv_obj_set_flex_grow(content_, 1);
    lv_obj_set_style_pad_all(content_, 5, 0);
    lv_obj_set_style_bg_color(content_, current_theme_.chat_background, 0);
    lv_obj_set_style_border_color(content_, current_theme_.border, 0); // Border color for content

    lv_obj_set_flex_flow(content_, LV_FLEX_FLOW_COLUMN); // 垂直布局（从上到下）
    lv_obj_set_flex_align(content_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_EVENLY); // 子对象居中对齐，等距分布

    
    ESP_LOGI(TAG, "emotion_container");
    emotion_image_ = lv_image_create(screen);
    int gif_size = LV_HOR_RES;
    lv_obj_set_size(emotion_image_, 280, 280);
    lv_obj_set_style_border_width(emotion_image_, 0, 0);
    lv_obj_center(emotion_image_);
    lv_image_set_src(emotion_image_, &angry_00);
    
    // 设置用户数据，以便动画回调函数可以访问CustomLcdDisplay实例
    lv_obj_set_user_data(emotion_image_, this);
    
    // 初始化动画对象
    emotion_anim_ = new lv_anim_t();
    lv_anim_init(emotion_anim_);
    
    // 启动表情动画
    StartEmotionAnimation();

    emotion_label_ = lv_label_create(content_);
    lv_obj_set_style_text_font(emotion_label_, &font_awesome_30_4, 0);
    lv_obj_set_style_text_color(emotion_label_, current_theme_.text, 0);
    lv_label_set_text(emotion_label_, FONT_AWESOME_AI_CHIP);

    preview_image_ = lv_image_create(content_);
    lv_obj_set_size(preview_image_, width_ * 0.5, height_ * 0.5);
    lv_obj_align(preview_image_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(preview_image_, LV_OBJ_FLAG_HIDDEN);

    chat_message_label_ = lv_label_create(content_);
    lv_label_set_text(chat_message_label_, "");
    lv_obj_set_width(chat_message_label_, LV_HOR_RES * 0.9); // 限制宽度为屏幕宽度的 90%
    lv_label_set_long_mode(chat_message_label_, LV_LABEL_LONG_WRAP); // 设置为自动换行模式
    lv_obj_set_style_text_align(chat_message_label_, LV_TEXT_ALIGN_CENTER, 0); // 设置文本居中对齐
    lv_obj_set_style_text_color(chat_message_label_, current_theme_.text, 0);

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
    lv_obj_set_style_text_color(network_label_, current_theme_.text, 0);

    notification_label_ = lv_label_create(status_bar_);
    lv_obj_set_flex_grow(notification_label_, 1);
    lv_obj_set_style_text_align(notification_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(notification_label_, current_theme_.text, 0);
    lv_label_set_text(notification_label_, "");
    lv_obj_add_flag(notification_label_, LV_OBJ_FLAG_HIDDEN);

    status_label_ = lv_label_create(status_bar_);
    lv_obj_set_flex_grow(status_label_, 1);
    lv_label_set_long_mode(status_label_, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(status_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(status_label_, current_theme_.text, 0);
    lv_label_set_text(status_label_, Lang::Strings::INITIALIZING);
    mute_label_ = lv_label_create(status_bar_);
    lv_label_set_text(mute_label_, "");
    lv_obj_set_style_text_font(mute_label_, fonts_.icon_font, 0);
    lv_obj_set_style_text_color(mute_label_, current_theme_.text, 0);

    battery_label_ = lv_label_create(status_bar_);
    lv_label_set_text(battery_label_, "");
    lv_obj_set_style_text_font(battery_label_, fonts_.icon_font, 0);
    lv_obj_set_style_text_color(battery_label_, current_theme_.text, 0);

    low_battery_popup_ = lv_obj_create(screen);
    lv_obj_set_scrollbar_mode(low_battery_popup_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(low_battery_popup_, LV_HOR_RES * 0.9, fonts_.text_font->line_height * 2);
    lv_obj_align(low_battery_popup_, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(low_battery_popup_, current_theme_.low_battery, 0);
    lv_obj_set_style_radius(low_battery_popup_, 10, 0);
    low_battery_label_ = lv_label_create(low_battery_popup_);
    lv_label_set_text(low_battery_label_, Lang::Strings::BATTERY_NEED_CHARGE);
    lv_obj_set_style_text_color(low_battery_label_, lv_color_white(), 0);
    lv_obj_center(low_battery_label_);
    lv_obj_add_flag(low_battery_popup_, LV_OBJ_FLAG_HIDDEN);
}

void CustomLcdDisplay::SetTheme(const std::string& theme_name) {
 
}
/*
void CustomLcdDisplay::SetEmotion(const std::string& emotion) {
 
}

void CustomLcdDisplay::SetPreviewImage(const std::string& image_path) {
 
}


void CustomLcdDisplay::SetChatMessage(const std::string& message) 
{
 
}
*/

// LVGL动画回调函数 - 用于更新表情图片
static void emotion_anim_cb(void* obj, int32_t value) {
    lv_obj_t* image = (lv_obj_t*)obj;
    // 获取CustomLcdDisplay实例
    CustomLcdDisplay* display = CustomLcdDisplay::GetInstance() ;
    if (display) {
        // 计算当前帧索引（value从0到999，映射到0-9帧）
        int frame_index = (value * display->ANGRY_GIF_FRAMES) / 1000;
        if (frame_index != display->current_frame_) {
            display->current_frame_ = frame_index;
            // 设置对应的图片（angry_gif数组在本文件中已定义）
            lv_image_set_src(image, angry_gif[frame_index]);
        }
    }
}

// 启动表情动画
void CustomLcdDisplay::StartEmotionAnimation() {
    if (!emotion_anim_ || !emotion_image_) {
        return;
    }
    
    // 停止之前的动画（如果存在）
    lv_anim_del(emotion_image_, nullptr);
    
    // 设置动画参数
    lv_anim_set_var(emotion_anim_, emotion_image_);           // 动画目标对象
    lv_anim_set_exec_cb(emotion_anim_, emotion_anim_cb);      // 动画执行回调
    lv_anim_set_values(emotion_anim_, 0, 999);               // 动画值范围：0-999
    lv_anim_set_time(emotion_anim_, 5000);                   // 动画周期：1秒
    lv_anim_set_repeat_count(emotion_anim_, LV_ANIM_REPEAT_INFINITE); // 无限循环
    lv_anim_set_path_cb(emotion_anim_, lv_anim_path_linear);  // 线性动画路径
    
    // 启动动画
    lv_anim_start(emotion_anim_);
    
    ESP_LOGI(TAG, "表情动画已启动，周期1秒，循环播放");
}

// 停止表情动画
void CustomLcdDisplay::StopEmotionAnimation() {
    if (emotion_image_) {
        lv_anim_del(emotion_image_, nullptr);
        ESP_LOGI(TAG, "表情动画已停止");
    }
}

// 析构函数实现
CustomLcdDisplay::~CustomLcdDisplay() {

    // Clear the static instance pointer if this instance is being deleted
    if (s_instance == this) {
        s_instance = nullptr;
    }
    // 停止动画
    StopEmotionAnimation();
    
    // 清理动画对象
    if (emotion_anim_) {
        delete emotion_anim_;
        emotion_anim_ = nullptr;
    }
}

// Implementation of the GetInstance method
CustomLcdDisplay* CustomLcdDisplay::GetInstance() {
    return s_instance;
}
