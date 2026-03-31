#include "wifi_board.h"
#include "codecs/es8311_audio_codec.h"
#include "application.h"
#include "display/lcd_display.h"
// #include "display/no_display.h"
#include "button.h"

#include "esp_video.h"
#include "esp_video_init.h"
#include "esp_cam_sensor_xclk.h"

#include "esp_lcd_panel_ops.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_ldo_regulator.h"

#include "esp_lcd_mipi_dsi.h"
#include "config.h"

#include <esp_log.h>
#include <driver/i2c_master.h>
#include <esp_lvgl_port.h>
#include "esp_lcd_lt8912b.h"
#define TAG "WaveshareEsp32p4nanoHdmi"

typedef enum {
    BSP_HDMI_RES_NONE = 0,
    BSP_HDMI_RES_800x600,   /*!< 800x600@60HZ   */
    BSP_HDMI_RES_1024x768,  /*!< 1024x768@60HZ  */
    BSP_HDMI_RES_1280x720,  /*!< 1280x720@60HZ  */
    BSP_HDMI_RES_1280x800,  /*!< 1280x800@60HZ  */
    BSP_HDMI_RES_1920x1080  /*!< 1920x1080@30HZ */
} bsp_hdmi_resolution_t;


class CustomBacklight : public Backlight {
public:
    CustomBacklight(i2c_master_bus_handle_t i2c_handle)
        : Backlight(), i2c_handle_(i2c_handle) {}

protected:
    i2c_master_bus_handle_t i2c_handle_;

    virtual void SetBrightnessImpl(uint8_t brightness) override {
        uint8_t i2c_address = 0x45;     // 7-bit address
        uint8_t reg = 0x96;
        uint8_t data[2] = {reg, brightness};

        i2c_master_dev_handle_t dev_handle;
        i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = i2c_address,
            .scl_speed_hz = 100000,
        };

        esp_err_t err = i2c_master_bus_add_device(i2c_handle_, &dev_cfg, &dev_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to add I2C device: %s", esp_err_to_name(err));
            return;
        }

        err = i2c_master_transmit(dev_handle, data, sizeof(data), -1);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to transmit brightness: %s", esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "Backlight brightness set to %u", brightness);
        }

        // i2c_master_bus_rm_device(dev_handle);
    }
};

class WaveshareEsp32p4nano : public WifiBoard {
private:
    i2c_master_bus_handle_t codec_i2c_bus_;
    Button boot_button_;
    LcdDisplay *display__;
 

    void InitializeCodecI2c() {
        // Initialize I2C peripheral
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = I2C_NUM_1,
            .sda_io_num = AUDIO_CODEC_I2C_SDA_PIN,
            .scl_io_num = AUDIO_CODEC_I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &codec_i2c_bus_));
    }

    static esp_err_t bsp_enable_dsi_phy_power(void) {
#if MIPI_DSI_PHY_PWR_LDO_CHAN > 0
        // Turn on the power for MIPI DSI PHY, so it can go from "No Power" state to "Shutdown" state
        static esp_ldo_channel_handle_t phy_pwr_chan = NULL;
        esp_ldo_channel_config_t ldo_cfg = {
            .chan_id = MIPI_DSI_PHY_PWR_LDO_CHAN,
            .voltage_mv = MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV,
        };
        esp_ldo_acquire_channel(&ldo_cfg, &phy_pwr_chan);
        ESP_LOGI(TAG, "MIPI DSI PHY Powered on");
#endif // BSP_MIPI_DSI_PHY_PWR_LDO_CHAN > 0

        return ESP_OK;
    }

    void InitializeLCD() {
        uint8_t chip_addr = 0x45;
        uint8_t write_cmds[4][2] = {{0x95, 0x11}, {0x95, 0x17}, {0x96, 0x00}, {0x96, 0xFF}};
        i2c_device_config_t i2c_dev_conf = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = chip_addr,
            .scl_speed_hz = 100000,
        };
        i2c_master_dev_handle_t dev_handle = NULL;
        if (i2c_master_bus_add_device(codec_i2c_bus_, &i2c_dev_conf, &dev_handle) == ESP_OK)
        {
            for (uint8_t i = 0; i < 4; i++)
            {
                i2c_master_transmit(dev_handle, write_cmds[i], 2, 50);
            }
            i2c_master_bus_rm_device(dev_handle);
        }

        bsp_enable_dsi_phy_power();
        esp_lcd_panel_io_handle_t panel_io = NULL;
        esp_lcd_panel_handle_t disp_panel = NULL;

        esp_lcd_dsi_bus_handle_t mipi_dsi_bus = NULL;
        esp_lcd_dsi_bus_config_t bus_config = {
            .bus_id = 0,
            .num_data_lanes = 2,
            .phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT,
            .lane_bit_rate_mbps = 1000,
        };
        esp_lcd_new_dsi_bus(&bus_config, &mipi_dsi_bus);

        ESP_LOGI(TAG, "Install MIPI DSI LCD control panel");
        /* Main IO */
        esp_lcd_panel_io_i2c_config_t io_config = LT8912B_IO_CFG(400000, LT8912B_IO_I2C_MAIN_ADDRESS);
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(codec_i2c_bus_, &io_config, &panel_io));
    
        /* CEC DSI IO */
        esp_lcd_panel_io_handle_t io_cec_dsi = NULL;
        esp_lcd_panel_io_i2c_config_t io_config_cec = LT8912B_IO_CFG(400000, LT8912B_IO_I2C_CEC_ADDRESS);
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(codec_i2c_bus_, &io_config_cec, &io_cec_dsi));
    
        /* AVI IO */
        esp_lcd_panel_io_handle_t io_avi = NULL;
        esp_lcd_panel_io_i2c_config_t io_config_avi = LT8912B_IO_CFG(400000, LT8912B_IO_I2C_AVI_ADDRESS);
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(codec_i2c_bus_, &io_config_avi, &io_avi));
    
        const esp_lcd_dpi_panel_config_t dpi_configs[] = {
            LT8912B_800x600_PANEL_60HZ_DPI_CONFIG_WITH_FBS(1),
            LT8912B_1024x768_PANEL_60HZ_DPI_CONFIG_WITH_FBS(1),
            LT8912B_1280x720_PANEL_60HZ_DPI_CONFIG_WITH_FBS(1),
            LT8912B_1280x800_PANEL_60HZ_DPI_CONFIG_WITH_FBS(1),
            LT8912B_1920x1080_PANEL_30HZ_DPI_CONFIG_WITH_FBS(1)
        };
    
        const esp_lcd_panel_lt8912b_video_timing_t video_timings[] = {
            ESP_LCD_LT8912B_VIDEO_TIMING_800x600_60Hz(),
            ESP_LCD_LT8912B_VIDEO_TIMING_1024x768_60Hz(),
            ESP_LCD_LT8912B_VIDEO_TIMING_1280x720_60Hz(),
            ESP_LCD_LT8912B_VIDEO_TIMING_1280x800_60Hz(),
            ESP_LCD_LT8912B_VIDEO_TIMING_1920x1080_30Hz()
        };

        lt8912b_vendor_config_t vendor_config = {
            .mipi_config = {
                .dsi_bus = mipi_dsi_bus,
                .lane_num = LCD_MIPI_DSI_LANE_NUM,
            },
        };

        /* DPI config */
        bsp_hdmi_resolution_t resolution = BSP_HDMI_RES_1280x720;
        /* DPI config */
        switch (resolution) {
        case BSP_HDMI_RES_800x600:
            ESP_LOGI(TAG, "HDMI configuration for 800x600@60HZ");
            vendor_config.mipi_config.dpi_config = &dpi_configs[0];
            memcpy(&vendor_config.video_timing, &video_timings[0], sizeof(esp_lcd_panel_lt8912b_video_timing_t));
            break;
        case BSP_HDMI_RES_1024x768:
            ESP_LOGI(TAG, "HDMI configuration for 1024x768@60HZ");
            vendor_config.mipi_config.dpi_config = &dpi_configs[1];
            memcpy(&vendor_config.video_timing, &video_timings[1], sizeof(esp_lcd_panel_lt8912b_video_timing_t));
            break;
        case BSP_HDMI_RES_1280x720:
            ESP_LOGI(TAG, "HDMI configuration for 1280x720@60HZ");
            vendor_config.mipi_config.dpi_config = &dpi_configs[2];
            memcpy(&vendor_config.video_timing, &video_timings[2], sizeof(esp_lcd_panel_lt8912b_video_timing_t));
            break;
        case BSP_HDMI_RES_1280x800:
            ESP_LOGI(TAG, "HDMI configuration for 1280x800@60HZ");
            vendor_config.mipi_config.dpi_config = &dpi_configs[3];
            memcpy(&vendor_config.video_timing, &video_timings[3], sizeof(esp_lcd_panel_lt8912b_video_timing_t));
            break;
        case BSP_HDMI_RES_1920x1080:
            ESP_LOGI(TAG, "HDMI configuration for 1920x1080@30HZ");
            vendor_config.mipi_config.dpi_config = &dpi_configs[4];
            memcpy(&vendor_config.video_timing, &video_timings[4], sizeof(esp_lcd_panel_lt8912b_video_timing_t));
            break;
        default:
            ESP_LOGE(TAG, "Unsupported display type (%d)", resolution);
        }

        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = PIN_NUM_LCD_RST;
        panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
        panel_config.bits_per_pixel = 24;
        panel_config.vendor_config = &vendor_config;
        const esp_lcd_panel_lt8912b_io_t io_all = {
            .main = panel_io,
            .cec_dsi = io_cec_dsi,
            .avi = io_avi,
        };
        ESP_ERROR_CHECK(esp_lcd_new_panel_lt8912b(&io_all, &panel_config, &disp_panel));
        ESP_ERROR_CHECK(esp_lcd_panel_reset(disp_panel));
        ESP_ERROR_CHECK(esp_lcd_panel_init(disp_panel));
        ESP_LOGI(TAG, "Display initialized");


        display__ = new MipiLcdDisplay(panel_io, disp_panel, DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                       DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
    
    }
 
    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            // During startup (before connected), pressing BOOT button enters Wi-Fi config mode without reboot
            if (app.GetDeviceState() == kDeviceStateStarting) {
                EnterWifiConfigMode();
                return;
            }
            app.ToggleChatState();
        });
    }

public:
    WaveshareEsp32p4nano() :
        boot_button_(BOOT_BUTTON_GPIO) {
        InitializeCodecI2c();
        InitializeLCD();
        InitializeButtons();
    }

    virtual AudioCodec *GetAudioCodec() override {
        static Es8311AudioCodec audio_codec(codec_i2c_bus_, I2C_NUM_1, AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
                                            AUDIO_I2S_GPIO_MCLK, AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS, AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN,
                                            AUDIO_CODEC_PA_PIN, AUDIO_CODEC_ES8311_ADDR);
        return &audio_codec;
    }

    virtual Display *GetDisplay() override {
        return display__;
    }
 
};

DECLARE_BOARD(WaveshareEsp32p4nano);
