#ifndef PFS123_H
#define PFS123_H

#include <string>
#include <nvs_flash.h>

#define UART_PORT_PFS123 UART_NUM_0
#define RX_PIN_FPS123 GPIO_NUM_9

#define BUF_SIZE (512) // 缓冲区大小
void PFS123_init();

#endif
