// HARDWARE/usart/usart1.c
#include <stdlib.h>
#include "usart1.h"
#include "sys_data.h"
#include "servo/servo.h"
#include "log.h"
#include "led_rgb/led_rgb.h"
#include "beep/beep.h"
#include "alarm.h"
#include "delay.h"

extern volatile SystemData sys_data;
extern uint8_t alarm_off_flag; // 从beep_led.c引用（消警标志）

// 串口接收缓存（大小64字节，足够存指令）
#define USART1_BUF_LEN 64
uint8_t usart1_rx_buf[USART1_BUF_LEN];
uint16_t usart1_rx_cnt = 0; // 接收计数
uint8_t usart1_rx_complete = 0; // 接收完成标志（检测到'*'）

// 1. USART1初始化（115200波特率，带接收中断）
void USART1_Init(uint32_t baudrate) {
    GPIO_InitTypeDef GPIO_InitStru;
    USART_InitTypeDef USART_InitStru;
    NVIC_InitTypeDef NVIC_InitStru;

    // 使能时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

    // 配置TX（PA9，推挽复用）
    GPIO_InitStru.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStru.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStru.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStru);

    // 配置RX（PA10，浮空输入）
    GPIO_InitStru.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStru.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStru);

    // 配置USART1
    USART_InitStru.USART_BaudRate = baudrate;
    USART_InitStru.USART_WordLength = USART_WordLength_8b;
    USART_InitStru.USART_StopBits = USART_StopBits_1;
    USART_InitStru.USART_Parity = USART_Parity_No;
    USART_InitStru.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStru.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStru);

    // 使能接收中断（RXNE：接收非空）
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    // 配置NVIC
    NVIC_InitStru.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStru.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStru.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStru.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStru);

    USART_Cmd(USART1, ENABLE); // 使能USART1
}

// 2. USART1发送1字节
void USART1_SendByte(uint8_t data) {
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, data);
}

// 3. USART1发送字符串
void USART1_SendString(uint8_t *str) {
    while (*str != '\0') {
        USART1_SendByte(*str++);
    }
}

// 4. 指令解析函数（接收完成后调用）
void USART1_ParseCmd(void) {
    // 示例：解析“#SET_ANGLE,90*”
    if (strstr((char*)usart1_rx_buf, "#SET_ANGLE") != NULL) {
        uint8_t angle = 0;
        // 提取参数（逗号后，星号前）
        char *param = strchr((char*)usart1_rx_buf, ',');
        if (param != NULL) {
            param++; // 跳过逗号
            angle = atoi(param); // 转为数字
			sys_data.servo_manual_flag = 1;
            Servo_SetAngle(angle); // 控制舵机
            // 回复上位机：指令执行成功
            char resp[32];
            sprintf(resp, "$ACK,SET_ANGLE,OK*");
            USART1_SendString((uint8_t*)resp);
            printf("Set Servo Angle: %d°\r\n", angle);
        }
    }

    // 解析“#ALARM_OFF*”（远程消警）
    else if (strstr((char*)usart1_rx_buf, "#ALARM_OFF") != NULL) {
        Alarm_RemoteOff();
        char resp[32];
        sprintf(resp, "$ACK,ALARM_OFF,OK*");
        USART1_SendString((uint8_t*)resp);
        printf("Alarm Off\r\n");
    }

    // 解析“#GET_LOG,N*”（读N条日志）
    else if (strstr((char*)usart1_rx_buf, "#GET_LOG") != NULL) {
        uint16_t read_cnt = 1; // 默认读10条
//        char *param = strchr((char*)usart1_rx_buf, ',');
//        if (param != NULL) {
//            param++;
//            read_cnt = atoi(param);
//            if (read_cnt > 20) read_cnt = 20; // 最多读20条，避免数据量过大
//        }

        // 读取日志（从当前地址往前读，即最近的日志）
        AlarmLog logs[20];
        uint32_t start_addr = log_current_addr > LOG_START_ADDR ? 
            log_current_addr - read_cnt * LOG_SIZE : LOG_START_ADDR;
        uint16_t actual_cnt = Log_Read(start_addr, logs, read_cnt);



        // 回复日志数据（每条日志一条$LOG帧）
        char resp[64];
//        sprintf(resp, "$ACK,GET_LOG,CNT=%d*", actual_cnt);
//        USART1_SendString((uint8_t*)resp);
        for (uint16_t i = 0; i < actual_cnt; i++) {
//            sprintf(resp, "LOG:20230901103001,%d,%d,%d,%d", 
//                 logs[i].alarm_type,
//                logs[i].temp, logs[i].hum, logs[i].light);
			
			     sprintf(resp, "LOG:20230901103001,%d,%d,%d,%d", 
                 sys_data.alarm_state,
                sys_data.temp, sys_data.hum, sys_data.light);
            USART1_SendString((uint8_t*)resp);
            delay_ms(10); // 间隔10ms，避免上位机接收拥堵
        }
//        printf("Read Log: %d条\r\n", actual_cnt);
    }

    // 清空缓存和标志
    memset(usart1_rx_buf, 0, USART1_BUF_LEN);
    usart1_rx_cnt = 0;
    usart1_rx_complete = 0;
}

// 5. USART1中断服务函数（接收数据）
void USART1_IRQHandler(void) {
    if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET) {
        uint8_t data = USART_ReceiveData(USART1);
        // 存储数据到缓存，直到检测到'*'或缓存满
        if (usart1_rx_cnt < USART1_BUF_LEN - 1) {
            usart1_rx_buf[usart1_rx_cnt++] = data;
            if (data == '*') { // 检测到指令结束符'*'
                usart1_rx_buf[usart1_rx_cnt] = '\0'; // 字符串结尾
                usart1_rx_complete = 1;
            }
        } else {
            // 缓存满，重置
            usart1_rx_cnt = 0;
            memset(usart1_rx_buf, 0, USART1_BUF_LEN);
        }
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

// 实时数据上传函数（打包sys_data）
void USART1_SendRealData(void) {
    char data_buf[64];
    // 格式：$DATA,温度,湿度,光照,舵机角度,报警状态*
    sprintf(data_buf, "$DATA,%d,%d,%d,%d,%d*",
        sys_data.temp, sys_data.hum, sys_data.light,
        sys_data.servo_angle, sys_data.alarm_state);
    USART1_SendString((uint8_t*)data_buf);
	
//    // 同时串口打印（调试用）
//    printf("Send Data: %s\r\n", data_buf);
}