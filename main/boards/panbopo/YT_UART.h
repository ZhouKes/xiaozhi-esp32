#ifndef YT_UART_H
#define YT_UART_H

#include <string>
#include <nvs_flash.h>
#define UART_YT_PORT UART_NUM_1
#define RX_PIN GPIO_NUM_13
#define TX_PIN GPIO_NUM_12
#define BUF_SIZE (512) // 缓冲区大小
enum{
    Wakeup_Xiaozhi =1,          //小志唤醒
    Distribution_network_mode,  //进入配网
    Wake_word_pattern,          //唤醒词模式
    Wake_word_ended,            //唤醒词结束
    Wake_word_false_ended,      //唤醒词中止
    Bluetooth_mode,             //蓝牙模式
    Bluetooth_off,              //关闭蓝牙
    Bluetooth_pause,            //蓝牙音乐播放暂停
    Bluetooth_playing,          //蓝牙音乐播放继续
    Bluetooth_connected,        //蓝牙已连接
    Bluetooth_disconnected,     //蓝牙已断开
    Increase_volume,            //增大音量
    Decrease_volume,            //减少音量
    Maximum_volume,             //最大音量
    Minimum_volume,             //最小音量
    Last_song,                  //上一首
    Next_song,                  //下一首
    BT_Wakeup_Xiaozhi,           //蓝牙模式凝听
};

enum {
    C3_OFF =0,
    C3_ON   , 
    YT_ON   ,
    YT_OFF  ,
};
// extern uint8_t flag_sound; 
extern uint8_t yt_command_flag;
void YT_init();

#endif
