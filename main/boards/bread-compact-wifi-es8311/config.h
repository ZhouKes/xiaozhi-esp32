/*
 * @Date: 2025-07-02 22:34:12
 * @LastEditors: zhouke
 * @LastEditTime: 2025-07-03 21:29:35
 * @FilePath: \xiaozhi-esp32\main\boards\bread-compact-wifi-es8311\config.h
 */
#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

 
#define AUDIO_INPUT_SAMPLE_RATE  24000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000
 

#define AUDIO_I2S_GPIO_MCLK GPIO_NUM_10
#define AUDIO_I2S_GPIO_WS GPIO_NUM_13
#define AUDIO_I2S_GPIO_BCLK GPIO_NUM_11
#define AUDIO_I2S_GPIO_DIN  GPIO_NUM_14
#define AUDIO_I2S_GPIO_DOUT GPIO_NUM_12

#define AUDIO_CODEC_PA_PIN       GPIO_NUM_4
#define AUDIO_CODEC_I2C_SDA_PIN  GPIO_NUM_41
#define AUDIO_CODEC_I2C_SCL_PIN  GPIO_NUM_42
#define AUDIO_CODEC_ES8311_ADDR  0x18
 

#define BUILTIN_LED_GPIO        GPIO_NUM_48
#define BOOT_BUTTON_GPIO        GPIO_NUM_0
#define TOUCH_BUTTON_GPIO       GPIO_NUM_47
#define VOLUME_UP_BUTTON_GPIO   GPIO_NUM_40
#define VOLUME_DOWN_BUTTON_GPIO GPIO_NUM_39

 

// A MCP Test: Control a lamp
#define LAMP_GPIO GPIO_NUM_18

#endif // _BOARD_CONFIG_H_
