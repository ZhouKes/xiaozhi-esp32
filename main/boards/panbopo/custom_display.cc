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

//字体
LV_FONT_DECLARE(time70);
LV_FONT_DECLARE(time60);
LV_FONT_DECLARE(time50);
LV_FONT_DECLARE(time40);
LV_FONT_DECLARE(time30);
LV_FONT_DECLARE(time20);
 

//表情图标
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

LV_IMG_DECLARE(smile_00);
LV_IMG_DECLARE(smile_01);

LV_IMG_DECLARE(turnon_00);
LV_IMG_DECLARE(turnon_01);
LV_IMG_DECLARE(turnon_02);
LV_IMG_DECLARE(turnon_03);
LV_IMG_DECLARE(turnon_04);
LV_IMG_DECLARE(turnon_05);
 
 


//传感器图标
LV_IMG_DECLARE(thermometr_sun);
LV_IMG_DECLARE(Tornado);
LV_IMG_DECLARE(Thermometr);
LV_IMG_DECLARE(Umbrella_rain);


//天气动画
LV_IMG_DECLARE(sunny_00);
LV_IMG_DECLARE(sunny_01);
LV_IMG_DECLARE(sunny_02);
LV_IMG_DECLARE(sunny_03);
LV_IMG_DECLARE(sunny_04);
LV_IMG_DECLARE(sunny_05);
LV_IMG_DECLARE(sunny_06);  
LV_IMG_DECLARE(sunny_07);
LV_IMG_DECLARE(sunny_08);
LV_IMG_DECLARE(sunny_09);



 



#define TURNON_GIF_FRAMES 5
const lv_image_dsc_t* turnon_gif[] = {
    &turnon_00,
    &turnon_01,
    &turnon_02,
    &turnon_03,
    &turnon_04,
    &turnon_05,
};

#define ANGRY_GIF_FRAMES 9
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

#define SMILE_GIF_FRAMES 2
const lv_image_dsc_t* smile_gif[] = {
    &smile_00,
    &smile_01,
};




#define SUNNY_GIF_FRAMES 10
const lv_image_dsc_t* sunny_gif[] = {
    &sunny_00,
    &sunny_01,
    &sunny_02,
    &sunny_03,
    &sunny_04,
    &sunny_05,
    &sunny_06,
    &sunny_07,
    &sunny_08,
    &sunny_09,  
};


LV_FONT_DECLARE(font_puhui_20_4);
LV_FONT_DECLARE(font_puhui_16_4);
LV_FONT_DECLARE(font_puhui_30_4);
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
                    .text_font = &font_puhui_16_4,
                    .icon_font = &font_awesome_30_4,
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

    
    //创建自定义UI
    custom_bg = lv_obj_create(screen);
    lv_obj_set_size(custom_bg, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(custom_bg, current_theme_.background, 0);
    lv_obj_set_style_border_width(custom_bg, 0, 0);
    lv_obj_set_style_border_color(custom_bg, current_theme_.border, 0);
    lv_obj_center(custom_bg);
    lv_obj_set_flex_flow(custom_bg, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(custom_bg, 0, 0);
    lv_obj_set_style_pad_row(custom_bg, 0, 0);
    // 设置flex对齐属性，让子对象在垂直和水平方向都居中
    lv_obj_set_flex_align(custom_bg, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(custom_bg, LV_OBJ_FLAG_HIDDEN);
    
    /* Custom Status bar */
    lv_obj_t* custom_status_bar_ = lv_obj_create(custom_bg);
    lv_obj_set_size(custom_status_bar_, LV_HOR_RES / 2, 40);
    lv_obj_set_style_radius(custom_status_bar_, 0, 0);
    lv_obj_set_style_bg_color(custom_status_bar_, current_theme_.background, 0);
    lv_obj_set_style_text_color(custom_status_bar_, current_theme_.text, 0);
    lv_obj_set_flex_flow(custom_status_bar_, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(custom_status_bar_, 0, 0);
    lv_obj_set_style_border_width(custom_status_bar_, 0, 0);
    lv_obj_set_style_pad_column(custom_status_bar_, 0, 0);
    lv_obj_set_style_pad_left(custom_status_bar_, 2, 0);
    lv_obj_set_style_pad_right(custom_status_bar_, 2, 0);
    lv_obj_set_flex_align(custom_status_bar_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);


    battery_label_ = lv_label_create(custom_status_bar_);
    lv_label_set_text(battery_label_, "");
    lv_obj_set_style_text_font(battery_label_, fonts_.icon_font, 0);
    lv_obj_set_style_text_color(battery_label_, current_theme_.text, 0);
    lv_obj_set_style_pad_left(battery_label_, 10, 0);
    lv_obj_set_style_pad_right(battery_label_, 10, 0);

    network_label_ = lv_label_create(custom_status_bar_);
    lv_label_set_text(network_label_, "");
    lv_obj_set_style_text_font(network_label_, fonts_.icon_font, 0);
    lv_obj_set_style_text_color(network_label_, current_theme_.text, 0);
    lv_obj_set_style_pad_left(network_label_, 10, 0);
    lv_obj_set_style_pad_right(network_label_, 10, 0);


    lv_obj_t* time_label = lv_label_create(custom_bg);
    lv_obj_set_size(time_label, LV_HOR_RES, 50);
    lv_obj_set_style_text_font(time_label, &time60, 0);
    lv_label_set_text(time_label, "14:13");
    lv_obj_set_style_text_align(time_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(time_label);
    
 
 
    lv_obj_t* weather_bar = lv_obj_create(custom_bg);
    lv_obj_set_size(weather_bar, 280, 60);
    lv_obj_set_style_border_width(weather_bar, 0, 0);
    
    // 设置weather_bar为水平方向灵活布局
    lv_obj_set_flex_flow(weather_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(weather_bar, 0, 0);
    lv_obj_set_style_pad_column(weather_bar, 0, 0);
    // 设置子组件在垂直方向居中对齐
    lv_obj_set_flex_align(weather_bar, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    
    // 创建天气图片组件
    weather_image_ = lv_image_create(weather_bar);
    lv_obj_set_size(weather_image_, 60, 60);
    lv_obj_set_flex_grow(weather_image_, 1);
    lv_obj_set_style_border_width(weather_image_, 0, 0);
    lv_image_set_src(weather_image_, &sunny_00); // 设置初始天气图片
    
    // 创建警告消息标签组件
    lv_obj_t* warning_label = lv_label_create(weather_bar);
    lv_obj_set_flex_grow(warning_label, 1);
    lv_obj_set_style_text_align(warning_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(warning_label, lv_color_hex(0xFF0000), 0);
    lv_label_set_text(warning_label, "电池电量低");
    lv_obj_add_flag(warning_label, LV_OBJ_FLAG_HIDDEN);
    
    // 设置初始天气状态并启动天气动画
    current_weather = "sunny";
    weather_anim_ = new lv_anim_t();
    lv_anim_init(weather_anim_);
    StartWeatherAnimation();

    lv_obj_t* sensor_bar = lv_obj_create(custom_bg);
    lv_obj_set_size(sensor_bar, LV_HOR_RES, 160);
    lv_obj_set_style_border_width(sensor_bar, 0, 0);
    lv_obj_set_scrollbar_mode(sensor_bar, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t* empty_bar = lv_obj_create(custom_bg);
    lv_obj_set_size(empty_bar, LV_HOR_RES, 50);
    lv_obj_set_style_border_width(empty_bar, 0, 0);


    //Write codes screen_label_sensor_name4
    lv_obj_t* label_sensor_name4 = lv_label_create(sensor_bar);
    lv_obj_set_pos(label_sensor_name4, 253, 120);
    lv_obj_set_size(label_sensor_name4, 100, 22);
    lv_label_set_text(label_sensor_name4, "传感器数据4");
    lv_label_set_long_mode(label_sensor_name4, LV_LABEL_LONG_DOT);
 

    //Write codes screen_label_sensor_value4
    lv_obj_t* label_sensor_value4 = lv_label_create(sensor_bar);
    lv_obj_set_pos(label_sensor_value4, 251, 89);
    lv_obj_set_size(label_sensor_value4, 100, 35);
    lv_label_set_text(label_sensor_value4, "35%");
    lv_label_set_long_mode(label_sensor_value4, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(label_sensor_value4, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

 

    //Write codes screen_sensor_img4
    lv_obj_t* sensor_img4 = lv_image_create(sensor_bar);
    lv_obj_set_pos(sensor_img4, 193, 99);
    lv_obj_set_size(sensor_img4, 50, 50);
    lv_image_set_src(sensor_img4, &Tornado);
    

    //Write codes screen_label_sensor_name3
    lv_obj_t* label_sensor_name3 = lv_label_create(sensor_bar);
    lv_obj_set_pos(label_sensor_name3, 77, 120);
    lv_obj_set_size(label_sensor_name3, 100, 22);
    lv_label_set_text(label_sensor_name3, "传感器数据3");
    lv_label_set_long_mode(label_sensor_name3, LV_LABEL_LONG_DOT);
 

    //Write codes screen_label_sensor_value3
    label_sensor_value3 = lv_label_create(sensor_bar);
    lv_obj_set_pos(label_sensor_value3, 76, 89);
    lv_obj_set_size(label_sensor_value3, 100, 35);
    lv_label_set_text(label_sensor_value3, "2000lux");
    lv_label_set_long_mode(label_sensor_value3, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(label_sensor_value3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_sensor_img3
    lv_obj_t* sensor_img3 = lv_image_create(sensor_bar);
    lv_obj_set_pos(sensor_img3, 20, 99);
    lv_obj_set_size(sensor_img3, 50, 50);
    lv_image_set_src(sensor_img3, &Thermometr);
 

    //Write codes screen_label_sensor_name2
    lv_obj_t* label_sensor_name2 = lv_label_create(sensor_bar);
    lv_obj_set_pos(label_sensor_name2, 253, 55);
    lv_obj_set_size(label_sensor_name2, 100, 22);
    lv_label_set_text(label_sensor_name2, "传感器数据2");
    lv_label_set_long_mode(label_sensor_name2, LV_LABEL_LONG_DOT);


    //Write codes screen_label_sensorvalue2
    label_sensor_value2 = lv_label_create(sensor_bar);
    lv_obj_set_pos(label_sensor_value2, 251, 15);
    lv_obj_set_size(label_sensor_value2, 100, 35);
    lv_label_set_text(label_sensor_value2, "64%");
    lv_label_set_long_mode(label_sensor_value2, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(label_sensor_value2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

 

    //Write codes screen_sensor_img2
    lv_obj_t* sensor_img2 = lv_image_create(sensor_bar);
    lv_obj_set_pos(sensor_img2, 193, 16);
    lv_obj_set_size(sensor_img2, 50, 50);
    lv_image_set_src(sensor_img2, &Umbrella_rain);
 

    //Write codes screen_label_sensor_name1
    lv_obj_t* label_sensor_name1 = lv_label_create(sensor_bar);
    lv_obj_set_pos(label_sensor_name1, 77, 55);
    lv_obj_set_size(label_sensor_name1, 100, 22);
    lv_label_set_text(label_sensor_name1, "传感器数据1");
    lv_label_set_long_mode(label_sensor_name1, LV_LABEL_LONG_DOT);

    
    //Write codes screen_label_sensor_value1
    label_sensor_value1 = lv_label_create(sensor_bar);
    lv_obj_set_pos(label_sensor_value1, 76, 15);
    lv_obj_set_size(label_sensor_value1, 100, 35);
    lv_label_set_text(label_sensor_value1, "35°C");
    lv_label_set_long_mode(label_sensor_value1, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(label_sensor_value1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

 

    //Write codes screen_senser_img1
    lv_obj_t* sensor_img1 = lv_image_create(sensor_bar);
    lv_obj_set_pos(sensor_img1, 20, 16);
    lv_obj_set_size(sensor_img1, 50, 50);
    lv_obj_add_flag(sensor_img1, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(sensor_img1, &thermometr_sun);
 

    //Write codes screen_line_2
    lv_obj_t* screen_line_2 = lv_line_create(sensor_bar);
    lv_obj_set_pos(screen_line_2, 30, 80);
    lv_obj_set_size(screen_line_2, 300, 1);
    static lv_point_precise_t line_2[] = {{0, 0},{300, 0}};
    lv_line_set_points(screen_line_2, line_2, 2);

    //Write style for screen_line_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_line_width(screen_line_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(screen_line_2, lv_color_hex(0x757575), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(screen_line_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_rounded(screen_line_2, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_line_1
    lv_obj_t* screen_line_1 = lv_line_create(sensor_bar);
    lv_obj_set_pos(screen_line_1, 180, 10);
    lv_obj_set_size(screen_line_1, 1, 140);
    static lv_point_precise_t line_1[] = {{0, 0},{0, 140}};
    lv_line_set_points(screen_line_1, line_1, 2);

    //Write style for screen_line_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_line_width(screen_line_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(screen_line_1, lv_color_hex(0x757575), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(screen_line_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_rounded(screen_line_1, true, LV_PART_MAIN|LV_STATE_DEFAULT);


    ESP_LOGI(TAG, "emotion_container");
    emotion_bg = lv_obj_create(screen);
    lv_obj_set_size(emotion_bg, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(emotion_bg, current_theme_.background, 0);
    lv_obj_set_style_border_width(emotion_bg, 0, 0);
    lv_obj_set_style_border_color(emotion_bg, current_theme_.border, 0);
    lv_obj_center(emotion_bg);


    emotion_image_ = lv_image_create(emotion_bg);
    current_emotion = "turnon";
    lv_obj_set_size(emotion_image_, 200, 200);
    lv_obj_set_style_border_width(emotion_image_, 0, 0);
    lv_obj_center(emotion_image_);
    lv_image_set_src(emotion_image_, &turnon_05);
    
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

void CustomLcdDisplay::SetEmotion(const char* emotion) {
    struct Emotion {
        const char* icon;
        const char* text;
    };

    static const std::vector<Emotion> emotions = {
        {"smile", "neutral"},
        {"smile", "happy"},
        {"smile", "laughing"},
        {"smile", "funny"},
        {"smile", "sad"},
        {"smile", "angry"},
        {"smile", "crying"},
        {"smile", "loving"},
        {"smile", "embarrassed"},
        {"smile", "surprised"},
        {"smile", "shocked"},
        {"smile", "thinking"},
        {"smile", "winking"},
        {"smile", "cool"},
        {"smile", "relaxed"},
        {"smile", "delicious"},
        {"smile", "kissy"},
        {"smile", "confident"},
        {"smile", "sleepy"},
        {"smile", "silly"},
        {"smile", "confused"}
    };
    
    // 查找匹配的表情
    std::string_view emotion_view(emotion);
    auto it = std::find_if(emotions.begin(), emotions.end(),
        [&emotion_view](const Emotion& e) { return e.text == emotion_view; });

    DisplayLockGuard lock(this);
 

    // 如果找到匹配的表情就显示对应图标，否则显示默认的neutral表情
    if (it != emotions.end()) {
        current_emotion = it->text;
    } else {
        current_emotion = "smile";
    }

}

void CustomLcdDisplay::SetStatus(const char* status) {
    DisplayLockGuard lock(this);

    if (strcmp(status, Lang::Strings::STANDBY) == 0) {
        lv_obj_clear_flag(custom_bg, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(emotion_bg, LV_OBJ_FLAG_HIDDEN);
        is_weather_animation_paused = false;
        is_emotion_animation_paused = true;
    } else if (strcmp(status, Lang::Strings::LISTENING) == 0) {
        lv_obj_add_flag(custom_bg, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(emotion_bg, LV_OBJ_FLAG_HIDDEN);
        is_weather_animation_paused = true;
        is_emotion_animation_paused = false;
    } else if (strcmp(status, Lang::Strings::SPEAKING) == 0) {
        lv_obj_add_flag(custom_bg, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(emotion_bg, LV_OBJ_FLAG_HIDDEN);
        is_weather_animation_paused = true;
        is_emotion_animation_paused = false;
    }else{
        lv_obj_clear_flag(custom_bg, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(emotion_bg, LV_OBJ_FLAG_HIDDEN);
        is_weather_animation_paused = false;
        is_emotion_animation_paused = true;
    }

    if (status_label_ == nullptr) {
        return;
    }
    lv_label_set_text(status_label_, status);
    lv_obj_clear_flag(status_label_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(notification_label_, LV_OBJ_FLAG_HIDDEN);

    last_status_update_time_ = std::chrono::system_clock::now();
    
}

/*
void CustomLcdDisplay::SetPreviewImage(const std::string& image_path) {
 
}


void CustomLcdDisplay::SetChatMessage(const std::string& message) 
{
 
}
*/

// LVGL动画回调函数 - 用于更新表情图片
static void emotion_anim_cb(void* obj, int32_t value) {
    CustomLcdDisplay* display = CustomLcdDisplay::GetInstance() ;

    if (display->is_emotion_animation_paused) {
        return;
    }
    lv_obj_t* image = (lv_obj_t*)obj;
    // 获取CustomLcdDisplay实例
    //根据当前动画类型展示不同动画
    if (display) {
        std::string current_emotion = display->current_emotion;
        if (current_emotion == "turnon") {
            int frame_index = ((value * TURNON_GIF_FRAMES) / 200) % TURNON_GIF_FRAMES;
            lv_image_set_src(image, turnon_gif[frame_index]);
        } else if (current_emotion == "angry") {
            int frame_index = (value * ANGRY_GIF_FRAMES) / 1000;
            lv_image_set_src(image, angry_gif[frame_index]);
        } else if (current_emotion == "smile") {
            int frame_index = (value * SMILE_GIF_FRAMES) / 1000;
            lv_image_set_src(image, smile_gif[frame_index]);
        }
    }
}

static void weather_anim_cb(void* obj, int32_t value) {

    CustomLcdDisplay* display = CustomLcdDisplay::GetInstance() ;
    if (display->is_weather_animation_paused) {
        return;
    }
    
    lv_obj_t* image = (lv_obj_t*)obj;
    if (display) {  
        std::string current_weather = display->current_weather;
        if (current_weather == "sunny") {
            int frame_index = (value * SUNNY_GIF_FRAMES) / 1000;
            lv_image_set_src(image, sunny_gif[frame_index]);
            ESP_LOGI(TAG, "sunny");
        } else if (current_weather == "cloudy") {
            int frame_index = (value * SUNNY_GIF_FRAMES) / 1000;
            lv_image_set_src(image, sunny_gif[frame_index]);
        } else if (current_weather == "rainy") {
            int frame_index = (value * SUNNY_GIF_FRAMES) / 1000;
            lv_image_set_src(image, sunny_gif[frame_index]);
        }else{
            int frame_index = (value * SUNNY_GIF_FRAMES) / 1000;
            lv_image_set_src(image, sunny_gif[frame_index]);
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


void CustomLcdDisplay::StartWeatherAnimation() {
    if (!weather_anim_ || !weather_image_) {
        return;
    }
    // 停止之前的动画（如果存在）
    lv_anim_del(weather_image_, nullptr);
    
    // 设置动画参数
    lv_anim_set_var(weather_anim_, weather_image_);           // 动画目标对象
    lv_anim_set_exec_cb(weather_anim_, weather_anim_cb);      // 动画执行回调
    lv_anim_set_values(weather_anim_, 0, 999);               // 动画值范围：0-999
    lv_anim_set_time(weather_anim_, 2000);                   // 动画周期：1秒
    lv_anim_set_repeat_count(weather_anim_, LV_ANIM_REPEAT_INFINITE); // 无限循环

    // 启动动画
    lv_anim_start(weather_anim_);
}


void CustomLcdDisplay::StopWeatherAnimation() {
    if (weather_image_) {
        lv_anim_del(weather_image_, nullptr);
        ESP_LOGI(TAG, "天气动画已停止");
    }
}
 
void CustomLcdDisplay::HideCustomBG() {
    DisplayLockGuard lock(this);
    lv_obj_add_flag(custom_bg, LV_OBJ_FLAG_HIDDEN);
    is_weather_animation_paused = true;
}

void CustomLcdDisplay::ShowCustomBG() {
    DisplayLockGuard lock(this);
    lv_obj_clear_flag(custom_bg, LV_OBJ_FLAG_HIDDEN);
    is_weather_animation_paused = false;
}

void CustomLcdDisplay::ShowEmotionBG() {
    DisplayLockGuard lock(this);
    lv_obj_clear_flag(emotion_bg, LV_OBJ_FLAG_HIDDEN);
    is_emotion_animation_paused = false;
}

void CustomLcdDisplay::HideEmotionBG() {
    DisplayLockGuard lock(this);
    lv_obj_add_flag(emotion_bg, LV_OBJ_FLAG_HIDDEN);
    is_emotion_animation_paused = true;
}

void CustomLcdDisplay::SetSensorData1(const char* value1) {
    DisplayLockGuard lock(this);
    lv_label_set_text(label_sensor_value1, value1);
}

void CustomLcdDisplay::SetSensorData2(const char* value2) {
    DisplayLockGuard lock(this);
    lv_label_set_text(label_sensor_value2, value2);
}

void CustomLcdDisplay::SetSensorData3(const char* value3) {
    DisplayLockGuard lock(this);
    lv_label_set_text(label_sensor_value3, value3);
}

void CustomLcdDisplay::SetSensorData4(const char* value4) {
    DisplayLockGuard lock(this);
    lv_label_set_text(label_sensor_value4, value4);
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
