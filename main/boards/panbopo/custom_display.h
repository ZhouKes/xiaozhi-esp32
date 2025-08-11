/*
 * @Date: 2025-08-07 20:53:34
 * @LastEditors: zhouke
 * @LastEditTime: 2025-08-10 23:17:45
 * @FilePath: \xiaozhi-esp32\main\boards\panbopo\custom_display.h
 */
#pragma once

// 说明：本头文件可能需要根据实际项目结构调整包含路径

// 标准库包含
#include <string>

// ESP-IDF 相关包含
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>

// LVGL相关包含
#include "lvgl.h"
#include "core/lv_obj_pos.h"
#include "font/lv_font.h"
#include "misc/lv_types.h"

// 项目特定包含
#include "display/lcd_display.h"  // SpiLcdDisplay类定义

 
class CustomLcdDisplay : public SpiLcdDisplay {
public:
    lv_obj_t* emotion_bg;
    lv_obj_t* custom_bg;
    lv_obj_t* emotion_image_;
    lv_anim_t* emotion_anim_;
    lv_obj_t* weather_image_;
    lv_anim_t* weather_anim_;
    bool is_weather_animation_paused = false;
    bool is_emotion_animation_paused = false;

    lv_obj_t* label_sensor_value1;
    lv_obj_t* label_sensor_value2;
    lv_obj_t* label_sensor_value3;
    lv_obj_t* label_sensor_value4;
 

    // 表情动画相关变量
    std::string current_emotion;
    std::string current_weather;
    // 动画相关变量
    static const int ANGRY_GIF_FRAMES = 10;  // angry_gif数组的帧数
    int current_frame_ = 0;                  // 当前帧索引

    /**
     * 启动表情动画
     */
    void StartEmotionAnimation();

    /**
     * 停止表情动画
     */
    void StopEmotionAnimation();

    /**
     * 暂停表情动画 
     */
    void PauseEmotionAnimation();

    /**
     * 恢复表情动画
     */
    void ResumeEmotionAnimation();
    /**
     * 启动天气动画
     */
    void StartWeatherAnimation();

    /**
     * 停止天气动画
     */
    void StopWeatherAnimation();

    /**
     * 暂停天气动画
     */
    void PauseWeatherAnimation();

    /**
     * 恢复天气动画
     */
    void ResumeWeatherAnimation();
    /**
     * 隐藏自定义背景
     */
    void HideCustomBG();

    /**
     * 显示自定义背景
     */
    void ShowCustomBG();

    /**
     * 显示表情背景
     */
    void ShowEmotionBG();   

    /**
     * 隐藏表情背景
     */
    void HideEmotionBG();   


    /**
     * 设置传感器数据
     */
    void SetSensorData1(const char* value1);
    void SetSensorData2(const char* value2);
    void SetSensorData3(const char* value3);
    void SetSensorData4(const char* value4);


    static CustomLcdDisplay* GetInstance();

    /**
     * 析构函数
     */
    ~CustomLcdDisplay();


    /**
     * 构造函数
     */
    CustomLcdDisplay(esp_lcd_panel_io_handle_t io_handle, 
                    esp_lcd_panel_handle_t panel_handle,
                    int width,
                    int height,
                    int offset_x,
                    int offset_y,
                    bool mirror_x,
                    bool mirror_y,
                    bool swap_xy);
    
    /**
     * 设置是否进入闲置状态
     * 闲置状态会自动切换到时钟界面
     */
    void SetIdle(bool status);
    
    /**
     * 设置UI布局
     */
    void SetupUI() override;
    
    /**
     * 设置聊天消息
     * @param role 角色("user", "assistant", "system")
     * @param content 消息内容
     */
    void SetChatMessage(const char* role, const char* content) override;
    
    /**
     * 设置主题
     * @param theme_name 主题名称("dark", "light")
     */
    void SetTheme(const std::string& theme_name) override;

 
private:
 
 
}; 
