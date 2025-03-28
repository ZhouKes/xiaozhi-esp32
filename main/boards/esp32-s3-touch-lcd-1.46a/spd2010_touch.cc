#include "spd2010_touch.h"
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TAG "SPD2010_TOUCH"

SPD2010TouchDriver::SPD2010TouchDriver(i2c_master_bus_handle_t i2c_bus)
    : i2c_bus_(i2c_bus), 
      touch_dev_handle_(NULL), 
      initialized_(false) {
    memset(&touch_data_, 0, sizeof(touch_data_));
}

SPD2010TouchDriver::~SPD2010TouchDriver() {
    if (touch_dev_handle_ != NULL) {
        i2c_master_bus_rm_device(touch_dev_handle_);
        touch_dev_handle_ = NULL;
    }
    initialized_ = false;
}

esp_err_t SPD2010TouchDriver::initialize() {
    ESP_LOGI(TAG, "初始化SPD2010触摸屏");
    
    // 创建I2C设备
    const i2c_device_config_t dev_cfg = {
        .device_address = SPD2010_ADDR,
        .scl_speed_hz = I2C_CLK_SPEED,
    };
    
    // 创建I2C设备句柄
    esp_err_t ret = i2c_master_bus_add_device(i2c_bus_, &dev_cfg, &touch_dev_handle_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "创建触摸设备句柄失败: %d", ret);
        return ret;
    }
    
    // 读取固件版本
    ret = touch_read_fw_version();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "读取触摸屏固件版本失败: %d", ret);
        i2c_master_bus_rm_device(touch_dev_handle_); // 失败时释放设备
        touch_dev_handle_ = NULL;
        return ret;
    }
    
    // 标记触摸屏已初始化
    initialized_ = true;
    ESP_LOGI(TAG, "触摸屏初始化成功");
    
    return ESP_OK;
}

// I2C辅助函数 - 读取触摸寄存器数据
esp_err_t SPD2010TouchDriver::touch_i2c_read(uint16_t reg_addr, uint8_t *data, uint32_t len) {
    uint8_t buf_addr[2];
    buf_addr[0] = (uint8_t)(reg_addr >> 8);
    buf_addr[1] = (uint8_t)reg_addr;
    
    return i2c_master_transmit_receive(touch_dev_handle_, buf_addr, 2, data, len, I2C_TIMEOUT_MS);
}

// I2C辅助函数 - 写入触摸寄存器数据
esp_err_t SPD2010TouchDriver::touch_i2c_write(uint16_t reg_addr, uint8_t *data, uint32_t len) {
    uint8_t buf[len+2];
    buf[0] = (uint8_t)(reg_addr >> 8);
    buf[1] = (uint8_t)reg_addr;
    memcpy(&buf[2], data, len);
    
    return i2c_master_transmit(touch_dev_handle_, buf, len+2, I2C_TIMEOUT_MS);
}

// SPD2010触摸屏命令函数
esp_err_t SPD2010TouchDriver::touch_write_point_mode_cmd() {
    uint8_t sample_data[4];
    sample_data[0] = 0x50;
    sample_data[1] = 0x00;
    sample_data[2] = 0x00;
    sample_data[3] = 0x00;
    
    esp_err_t ret = touch_i2c_write((((uint16_t)sample_data[0] << 8) | sample_data[1]), 
                                   &sample_data[2], 2);
    esp_rom_delay_us(200);
    return ret;
}

esp_err_t SPD2010TouchDriver::touch_write_start_cmd() {
    uint8_t sample_data[4];
    sample_data[0] = 0x46;
    sample_data[1] = 0x00;
    sample_data[2] = 0x00;
    sample_data[3] = 0x00;
    
    esp_err_t ret = touch_i2c_write((((uint16_t)sample_data[0] << 8) | sample_data[1]), 
                                   &sample_data[2], 2);
    esp_rom_delay_us(200);
    return ret;
}

esp_err_t SPD2010TouchDriver::touch_write_cpu_start_cmd() {
    uint8_t sample_data[4];
    sample_data[0] = 0x04;
    sample_data[1] = 0x00;
    sample_data[2] = 0x01;
    sample_data[3] = 0x00;
    
    esp_err_t ret = touch_i2c_write((((uint16_t)sample_data[0] << 8) | sample_data[1]), 
                                   &sample_data[2], 2);
    esp_rom_delay_us(200);
    return ret;
}

esp_err_t SPD2010TouchDriver::touch_write_clear_int_cmd() {
    uint8_t sample_data[4];
    sample_data[0] = 0x02;
    sample_data[1] = 0x00;
    sample_data[2] = 0x01;
    sample_data[3] = 0x00;
    
    esp_err_t ret = touch_i2c_write((((uint16_t)sample_data[0] << 8) | sample_data[1]), 
                                   &sample_data[2], 2);
    esp_rom_delay_us(200);
    return ret;
}

// 读取触摸状态和数据长度
esp_err_t SPD2010TouchDriver::touch_read_status_length(touch_status_t *tp_status) {
    uint8_t sample_data[4];
    sample_data[0] = 0x20;
    sample_data[1] = 0x00;
    
    esp_err_t ret = touch_i2c_read((((uint16_t)sample_data[0] << 8) | sample_data[1]), 
                                  sample_data, 4);
    if (ret != ESP_OK) {
        return ret;
    }
    
    esp_rom_delay_us(200);
    tp_status->status_low.pt_exist = (sample_data[0] & 0x01);
    tp_status->status_low.gesture = (sample_data[0] & 0x02);
    tp_status->status_low.aux = ((sample_data[0] & 0x08));
    tp_status->status_high.tic_busy = ((sample_data[1] & 0x80) >> 7);
    tp_status->status_high.tic_in_bios = ((sample_data[1] & 0x40) >> 6);
    tp_status->status_high.tic_in_cpu = ((sample_data[1] & 0x20) >> 5);
    tp_status->status_high.tint_low = ((sample_data[1] & 0x10) >> 4);
    tp_status->status_high.cpu_run = ((sample_data[1] & 0x08) >> 3);
    tp_status->read_len = (sample_data[3] << 8 | sample_data[2]);
    
    return ESP_OK;
}

// 读取触摸点数据
esp_err_t SPD2010TouchDriver::touch_read_hdp(touch_status_t *tp_status, touch_data_t *touch) {
    if (tp_status->read_len <= 0) {
        return ESP_OK;
    }
    
    uint8_t sample_data[4+(10*6)]; // 4 Bytes Header + 10 Finger * 6 Bytes
    sample_data[0] = 0x00;
    sample_data[1] = 0x03;
    
    esp_err_t ret = touch_i2c_read((((uint16_t)sample_data[0] << 8) | sample_data[1]), 
                                  sample_data, tp_status->read_len);
    if (ret != ESP_OK) {
        return ret;
    }
    
    uint8_t check_id = sample_data[4];
    if ((check_id <= 0x0A) && tp_status->status_low.pt_exist) {
        touch->touch_num = ((tp_status->read_len - 4)/6);
        if (touch->touch_num > 10) {
            touch->touch_num = 10; // 限制最大点数
        }
        
        touch->gesture = 0x00;
        for (int i = 0; i < touch->touch_num; i++) {
            uint8_t offset = i*6;
            touch->points[i].id = sample_data[4 + offset];
            touch->points[i].x = (((sample_data[7 + offset] & 0xF0) << 4) | sample_data[5 + offset]);
            touch->points[i].y = (((sample_data[7 + offset] & 0x0F) << 8) | sample_data[6 + offset]);
            touch->points[i].weight = sample_data[8 + offset];
        }
        
        // 手势识别逻辑
        if ((touch->points[0].weight != 0) && (touch->down != 1)) {
            touch->down = 1;
            touch->up = 0;
            touch->down_x = touch->points[0].x;
            touch->down_y = touch->points[0].y;
        } else if ((touch->points[0].weight == 0) && (touch->down == 1)) {
            touch->up = 1;
            touch->down = 0;
            touch->up_x = touch->points[0].x;
            touch->up_y = touch->points[0].y;
        }
    } else if ((check_id == 0xF6) && tp_status->status_low.gesture) {
        touch->touch_num = 0x00;
        touch->gesture = sample_data[6] & 0x07;
    } else {
        touch->touch_num = 0x00;
        touch->gesture = 0x00;
    }
    
    return ESP_OK;
}

// 读取HDP状态
esp_err_t SPD2010TouchDriver::touch_read_hdp_status(tp_hdp_status_t *tp_hdp_status) {
    uint8_t sample_data[8];
    sample_data[0] = 0xFC;
    sample_data[1] = 0x02;
    
    esp_err_t ret = touch_i2c_read((((uint16_t)sample_data[0] << 8) | sample_data[1]), 
                                  sample_data, 8);
    if (ret != ESP_OK) {
        return ret;
    }
    
    tp_hdp_status->status = sample_data[5];
    tp_hdp_status->next_packet_len = (sample_data[2] | sample_data[3] << 8);
    
    return ESP_OK;
}

// 读取HDP剩余数据
esp_err_t SPD2010TouchDriver::touch_read_hdp_remain_data(tp_hdp_status_t *tp_hdp_status) {
    uint8_t sample_data[32];
    sample_data[0] = 0x00;
    sample_data[1] = 0x03;
    
    return touch_i2c_read((((uint16_t)sample_data[0] << 8) | sample_data[1]), 
                         sample_data, tp_hdp_status->next_packet_len);
}

// 读取固件版本
esp_err_t SPD2010TouchDriver::touch_read_fw_version() {
    uint8_t sample_data[18];
    sample_data[0] = 0x26;
    sample_data[1] = 0x00;
    
    esp_err_t ret = touch_i2c_read((((uint16_t)sample_data[0] << 8) | sample_data[1]), 
                                  sample_data, 18);
    if (ret != ESP_OK) {
        return ret;
    }
    
    uint32_t Dummy = ((sample_data[0] << 24) | (sample_data[1] << 16) | 
                      (sample_data[3] << 8) | (sample_data[0]));
    uint16_t DVer = ((sample_data[5] << 8) | (sample_data[4]));
    uint32_t PID = ((sample_data[9] << 24) | (sample_data[8] << 16) | 
                    (sample_data[7] << 8) | (sample_data[6]));
    uint32_t ICName_L = ((sample_data[13] << 24) | (sample_data[12] << 16) | 
                         (sample_data[11] << 8) | (sample_data[10]));    // "2010"
    uint32_t ICName_H = ((sample_data[17] << 24) | (sample_data[16] << 16) | 
                         (sample_data[15] << 8) | (sample_data[14]));    // "SPD"
    
    ESP_LOGI(TAG, "触摸屏固件: Dummy[%lu], DVer[%u], PID[%lu], Name[%lu-%lu]", 
            Dummy, DVer, PID, ICName_H, ICName_L);
            
    return ESP_OK;
}
touch_data_t* SPD2010TouchDriver::get_touch_data() { 
    touch_data_.touch_num = 0;
    read_touch_data();
    
    return &touch_data_; 
}


// 读取触摸数据主函数
esp_err_t SPD2010TouchDriver::read_touch_data() {
    if (!initialized_) {
        return ESP_ERR_INVALID_STATE;
    }
    
    touch_status_t tp_status = {0};
    tp_hdp_status_t tp_hdp_status = {0};
    
    // 读取状态和数据长度
    esp_err_t ret = touch_read_status_length(&tp_status);
    if (ret != ESP_OK) {
        return ret;
    }
    
    if (tp_status.status_high.tic_in_bios) {
        // 清除中断命令
        ret = touch_write_clear_int_cmd();
        if (ret != ESP_OK) return ret;
       
        // CPU启动命令
        ret = touch_write_cpu_start_cmd();
        if (ret != ESP_OK) return ret;
    } 
    else if (tp_status.status_high.tic_in_cpu) {
        // 触摸模式修改命令
        ret = touch_write_point_mode_cmd();
        if (ret != ESP_OK) return ret;
        
        // 触摸启动命令
        ret = touch_write_start_cmd();
        if (ret != ESP_OK) return ret;
        
        // 清除中断命令
        ret = touch_write_clear_int_cmd();
        if (ret != ESP_OK) return ret;
    } 
    else if (tp_status.status_high.cpu_run && tp_status.read_len == 0) {
        ret = touch_write_clear_int_cmd();
        if (ret != ESP_OK) return ret;
       
    } 
    else if (tp_status.status_low.pt_exist || tp_status.status_low.gesture) {
        // 读取HDP数据
        ret = touch_read_hdp(&tp_status, &touch_data_);
        if (ret != ESP_OK) return ret;
        
        
 hdp_done_check:
        // 读取HDP状态
        ret = touch_read_hdp_status(&tp_hdp_status);
        if (ret != ESP_OK) return ret;
       

        if (tp_hdp_status.status == 0x82) {
            // 清除中断
            ret = touch_write_clear_int_cmd();
            if (ret != ESP_OK) return ret;
        }
        else if (tp_hdp_status.status == 0x00) {
            // 读取HDP剩余数据
            ret = touch_read_hdp_remain_data(&tp_hdp_status);
            if (ret != ESP_OK) return ret;
            
            goto hdp_done_check;
        }
    } 
    else if (tp_status.status_high.cpu_run && tp_status.status_low.aux) {
        ret = touch_write_clear_int_cmd();
        if (ret != ESP_OK) return ret;
    }
    
    return ESP_OK;
}


#if 0
// 触摸屏轮询任务
void SPD2010TouchDriver::touch_poll_task(void *arg) {
    SPD2010TouchDriver *driver = static_cast<SPD2010TouchDriver*>(arg);
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (true) {
        // 读取触摸数据
        esp_err_t ret = driver->read_touch_data();
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "读取触摸数据失败: %d", ret);
        }
        
        // 20ms周期轮询
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(20));
    }
}

esp_err_t SPD2010TouchDriver::start_touch_poll_task(void* task_arg) {
    if (!initialized_) {
        return ESP_ERR_INVALID_STATE;
    }
    
    BaseType_t ret = xTaskCreate(touch_poll_task, "touch_poll", 8192, this, 15, NULL);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "创建触摸轮询任务失败");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "触摸轮询任务启动成功");
    return ESP_OK;
} 
#endif