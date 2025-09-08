// HARDWARE/usart/usart1.h
#ifndef USART1_H
#define USART1_H
#include "stm32f10x.h"
#include <string.h>
#include <stdio.h>

// 函数声明
void USART1_Init(uint32_t baudrate);
void USART1_SendByte(uint8_t data);
void USART1_SendString(uint8_t *str);
extern uint8_t usart1_rx_complete; // 接收完成标志，供主循环判断
// 4. 指令解析函数（接收完成后调用）
void USART1_ParseCmd(void);

void USART1_SendRealData(void);
#endif