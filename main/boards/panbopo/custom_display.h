/*
 * @Date: 2025-08-07 20:53:34
 * @LastEditors: zhouke
 * @LastEditTime: 2025-08-07 21:39:43
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
    lv_obj_t* emotion_image_;
    lv_anim_t* emotion_anim_;

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
