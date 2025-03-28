#pragma once

#include <driver/i2c_master.h>
#include <esp_err.h>
#include <stdint.h>
#include "esp_log.h"

// SPD2010触摸屏常量定义
#define SPD2010_ADDR                0x53
#define MAX_TOUCH_POINTS            5
#define I2C_TIMEOUT_MS              1000
#define I2C_CLK_SPEED               400000

// 定义触摸点数据结构
typedef struct {
    uint8_t id;
    uint16_t x;
    uint16_t y;
    uint8_t weight;
} touch_point_t;

// 定义触摸数据结构
typedef struct {
    touch_point_t points[10];
    uint8_t touch_num;
    uint8_t gesture;
    uint8_t down;
    uint8_t up;
    uint16_t down_x;
    uint16_t down_y;
    uint16_t up_x;
    uint16_t up_y;
} touch_data_t;

// 状态高字节数据结构
typedef struct {
    uint8_t none0;
    uint8_t none1;
    uint8_t none2;
    uint8_t cpu_run;
    uint8_t tint_low;
    uint8_t tic_in_cpu;
    uint8_t tic_in_bios;
    uint8_t tic_busy;
} touch_status_high_t;

// 状态低字节数据结构
typedef struct {
    uint8_t pt_exist;
    uint8_t gesture;
    uint8_t key;
    uint8_t aux;
    uint8_t keep;
    uint8_t raw_or_pt;
    uint8_t none6;
    uint8_t none7;
} touch_status_low_t;

// 完整状态数据结构
typedef struct {
    touch_status_low_t status_low;
    touch_status_high_t status_high;
    uint16_t read_len;
} touch_status_t;

typedef struct {
    uint8_t status;
    uint16_t next_packet_len;
} tp_hdp_status_t;

class SPD2010TouchDriver {
public:
    // 触摸回调函数类型
    typedef void (*touch_callback_t)(touch_data_t* touch_data, void* user_data);
    
    SPD2010TouchDriver(i2c_master_bus_handle_t i2c_bus);
    ~SPD2010TouchDriver();
    
    // 初始化触摸屏
    esp_err_t initialize();
    
    // 读取触摸数据 - 主函数
    esp_err_t read_touch_data();
    
    // 获取触摸数据的引用
    touch_data_t* get_touch_data();
    
    // 注册触摸回调函数
    void register_touch_callback(touch_callback_t callback, void* user_data);
    
    // 周期性处理 - 需要在主循环中调用
    esp_err_t periodic_process();
    
    // 判断触摸屏是否已初始化
    bool is_initialized() const { return initialized_; }

private:
    // I2C相关
    i2c_master_bus_handle_t i2c_bus_;
    i2c_master_dev_handle_t touch_dev_handle_;
    
    // 触摸数据
    touch_data_t touch_data_;
    bool initialized_;
    
    // 回调相关
    touch_callback_t touch_callback_ = nullptr;
    void* user_data_ = nullptr;
    
    // I2C辅助函数
    esp_err_t touch_i2c_read(uint16_t reg_addr, uint8_t *data, uint32_t len);
    esp_err_t touch_i2c_write(uint16_t reg_addr, uint8_t *data, uint32_t len);
    
    // SPD2010触摸屏命令函数
    esp_err_t touch_write_point_mode_cmd();
    esp_err_t touch_write_start_cmd();
    esp_err_t touch_write_cpu_start_cmd();
    esp_err_t touch_write_clear_int_cmd();
    
    // 读取触摸状态和数据
    esp_err_t touch_read_status_length(touch_status_t *tp_status);
    esp_err_t touch_read_hdp(touch_status_t *tp_status, touch_data_t *touch);
    esp_err_t touch_read_hdp_status(tp_hdp_status_t *tp_hdp_status);
    esp_err_t touch_read_hdp_remain_data(tp_hdp_status_t *tp_hdp_status);
    esp_err_t touch_read_fw_version();
}; 