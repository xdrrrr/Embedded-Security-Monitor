#ifndef ESP8266_H
#define ESP8266_H

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include "misc.h"
#include <string.h>
#include <stdio.h>


#include "sys_data.h"
#include "servo/servo.h"
#include "log.h"
#include "alarm.h"
#include "delay.h"
// 定义调试LED引脚 - PC13
#define DEBUG_LED_PIN    GPIO_Pin_13
#define DEBUG_LED_PORT   GPIOC

// 定义ESP-01S串口 - USART2 (PA2-TX, PA3-RX)
#define ESP_USART        USART2
#define ESP_USART_IRQn   USART2_IRQn

// 协议定义
#define MAX_DATA_LEN     128
#define CMD_TIMEOUT      3000  // 3秒超时

#define myaccount "123"
#define mypasswd "12345678"

#define myip "192.168.233.209"
//#define myip "192.168.157.209"
#define myport 8080



// 全局变量
extern char esp_rx_buffer[MAX_DATA_LEN];
extern volatile uint16_t esp_rx_index;
extern volatile uint8_t esp_cmd_complete;
extern volatile uint8_t esp_error;


extern volatile uint32_t millis_tick;





// 函数原型
void System_Init(void);
void LED_Init(void);
void USART_MyInit(void);
void USART_SendString(USART_TypeDef* USARTx, const char* str);
void ESP_SendCommand(const char* cmd);
uint8_t ESP_WaitResponse(const char* response, uint16_t timeout);
void ESP_ConnectWiFi(const char* ssid, const char* password);
void ESP_ConnectServer(const char* ip, uint16_t port);
void ESP_SendData(const char* data);

void ESP_ProcessReceivedData(void);
//void delay_ms(uint32_t ms);



// 外部可调用的命令处理接口（把原 USART1_ParseCmd 的逻辑移到这里）
void ESP_ProcessTCPCommand(const char* cmd_str);

// 用于把实时数据发送到上位机（通过ESP）
void ESP_SendRealData(void);




#endif