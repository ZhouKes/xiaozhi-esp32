#include "wifi_board.h"
#include "audio_codecs/no_audio_codec.h"
#include "display/lcd_display.h"
#include "system_reset.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "iot/thing_manager.h"
#include "led/single_led.h"

#include <wifi_station.h>
#include <esp_log.h>
#include <driver/i2c_master.h>
#include <esp_lcd_panel_vendor.h>
#include <driver/spi_common.h>

#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_gc9b71.h"
 
#define TAG "NV3401Board"

class NV3401Board : public WifiBoard {
private:
 
    Button boot_button_;
    LcdDisplay* display_;
    esp_lcd_panel_handle_t panel_handle = NULL;
    void InitializeSpi() {
        

    }

    void InitializeLcdDisplay() {
        const spi_bus_config_t buscfg = GC9B71_PANEL_BUS_QSPI_CONFIG(PIN_NUM_LCD_PCLK,
                                                               PIN_NUM_LCD_DATA0,
                                                               PIN_NUM_LCD_DATA1,
                                                               PIN_NUM_LCD_DATA2,
                                                               PIN_NUM_LCD_DATA3,
                                                               LCD_H_RES * LCD_V_RES * LCD_BIT_PER_PIXEL / 8);

        spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);

        ESP_LOGI(TAG, "载入面板引脚");
        esp_lcd_panel_io_handle_t io_handle = NULL;
        const esp_lcd_panel_io_spi_config_t io_config = GC9B71_PANEL_IO_QSPI_CONFIG(PIN_NUM_LCD_CS,
                                                                              nullptr,
                                                                              nullptr);
        gc9b71_vendor_config_t vendor_config = {
            .flags = {
                .use_qspi_interface = 1,
            },
        };

        // Attach the LCD to the SPI bus
        esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle);

        const esp_lcd_panel_dev_config_t panel_config = {
            .reset_gpio_num = PIN_NUM_LCD_RST,
            .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
            .bits_per_pixel = LCD_BIT_PER_PIXEL,
            .vendor_config = &vendor_config,
        };

        ESP_LOGI(TAG, "载入GC9B71面板驱动");
        esp_lcd_new_panel_gc9b71(io_handle, &panel_config, &panel_handle);
        esp_lcd_panel_reset(panel_handle);
        esp_lcd_panel_init(panel_handle);
        // user can flush pre-defined pattern to the screen before we turn on the screen or backlight
        esp_lcd_panel_disp_on_off(panel_handle, true);
 
        display_ = new LcdDisplay(io_handle, panel_handle, DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT,
                                    DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
    }


 
    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting && !WifiStation::GetInstance().IsConnected()) {
                ResetWifiConfiguration();
            }
            app.ToggleChatState();
        });
    }

    // 物联网初始化，添加对 AI 可见设备
    void InitializeIot() {
        auto& thing_manager = iot::ThingManager::GetInstance();
        thing_manager.AddThing(iot::CreateThing("Lamp"));
        //thing_manager.AddThing(iot::CreateThing("Lamp"));
    }

public:
    NV3401Board() :
        boot_button_(BOOT_BUTTON_GPIO) {
        InitializeSpi();
        InitializeLcdDisplay();
        InitializeButtons();
        InitializeIot();
    }

    virtual Led* GetLed() override {
        static SingleLed led(BUILTIN_LED_GPIO);
        return &led;
    }

    virtual AudioCodec* GetAudioCodec() override {
#ifdef AUDIO_I2S_METHOD_SIMPLEX
        static NoAudioCodecSimplex audio_codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_SPK_GPIO_BCLK, AUDIO_I2S_SPK_GPIO_LRCK, AUDIO_I2S_SPK_GPIO_DOUT, AUDIO_I2S_MIC_GPIO_SCK, AUDIO_I2S_MIC_GPIO_WS, AUDIO_I2S_MIC_GPIO_DIN);
#else
        static NoAudioCodecDuplex audio_codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS, AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN);
#endif
        return &audio_codec;
    }

    virtual Display* GetDisplay() override {
        return display_;
    }
};

DECLARE_BOARD(NV3401Board);
