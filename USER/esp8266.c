#include "esp8266.h"


// 全局变量
char esp_rx_buffer[MAX_DATA_LEN];
volatile uint16_t esp_rx_index = 0;
volatile uint8_t esp_cmd_complete = 0;
volatile uint8_t esp_error = 0;


// 调试LED初始化
void LED_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 配置PC13为推挽输出 - 调试LED
    GPIO_InitStructure.GPIO_Pin = DEBUG_LED_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DEBUG_LED_PORT, &GPIO_InitStructure);
    
    // 初始化为高电平 - LED灭
    GPIO_SetBits(DEBUG_LED_PORT, DEBUG_LED_PIN);
}



// 系统初始化
void System_Init(void) {
    // 配置系统时钟
    SystemInit();
    SystemCoreClockUpdate();
    
    // 开启外设时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
}



// USART初始化
void USART_MyInit(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // 配置USART2 TX (PA2) 复用推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 配置USART2 RX (PA3) 浮空输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // USART配置: 115200, 8N1
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(ESP_USART, &USART_InitStructure);
    
    // 配置USART接收中断
    NVIC_InitStructure.NVIC_IRQChannel = ESP_USART_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // 使能USART接收中断
    USART_ITConfig(ESP_USART, USART_IT_RXNE, ENABLE);
    
    // 使能USART
    USART_Cmd(ESP_USART, ENABLE);
}

// 串口发送字符串
void USART_SendString(USART_TypeDef* USARTx, const char* str) {
    while(*str) {
        // 等待发送数据寄存器空
        while(USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET);
        USART_SendData(USARTx, *str++);
    }
}

// 向ESP发送AT命令
void ESP_SendCommand(const char* cmd) {
    // 清除缓冲区
    esp_rx_index = 0;
    esp_cmd_complete = 0;
    esp_error = 0;
    memset((char*)esp_rx_buffer, 0, MAX_DATA_LEN);
    
    // 发送命令
    USART_SendString(ESP_USART, cmd);
    USART_SendString(ESP_USART, "\r\n");
}

// 等待ESP响应
uint8_t ESP_WaitResponse(const char* response, uint16_t timeout) {

		delay_ms(10);
    uint32_t start_time = millis_tick;
    while(!esp_cmd_complete && !esp_error) {
        // 检查超时: 当前时间 - 开始时间 > 超时时间
        if((millis_tick - start_time) > timeout) {
            esp_error = 1;
            return 0;
        }
    }
    
    // 检查响应中是否包含指定字符串
    if(esp_cmd_complete && strstr((char*)esp_rx_buffer, response) != NULL) {
        return 1;
    }
    
    // 响应错误LED闪烁2次
//    LED_Blink(1, 10000);


    return 0;
}

// 连接WiFi
void ESP_ConnectWiFi(const char* ssid, const char* password) {
    // 设置WiFi模式为STA
    ESP_SendCommand("AT+CWMODE=1");
    if(!ESP_WaitResponse("OK", 2000)) {
        // 持续尝试直到成功
        while(!ESP_WaitResponse("OK", 2000)) {
            ESP_SendCommand("AT+CWMODE=1");
        }
    }
    
    // 连接WiFi
    char cmd[128];
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"", ssid, password);
    ESP_SendCommand(cmd);
    
    // 等待连接成功
    if(!ESP_WaitResponse("OK", 10000)) {
        // 连接失败，尝试重新连接
        while(!ESP_WaitResponse("OK", 10000)) {
            ESP_SendCommand(cmd);
        }
    }
    
    // WiFi连接成功LED常亮2秒
    GPIO_ResetBits(DEBUG_LED_PORT, DEBUG_LED_PIN);
    delay_ms(2000);
    GPIO_SetBits(DEBUG_LED_PORT, DEBUG_LED_PIN);
}

// 连接到TCP服务器
void ESP_ConnectServer(const char* ip, uint16_t port) {
    // 设置单连接模式
    ESP_SendCommand("AT+CIPMUX=0");
    if(!ESP_WaitResponse("OK", 2000)) {
        // 持续尝试直到成功
        while(!ESP_WaitResponse("OK", 2000)) {
            ESP_SendCommand("AT+CIPMUX=0");
        }
    }
    
    // 连接TCP服务器
    char cmd[64];
    sprintf(cmd, "AT+CIPSTART=\"TCP\",\"%s\",%d", ip, port);
    ESP_SendCommand(cmd);
    
    // 等待连接成功
    if(!ESP_WaitResponse("OK", 5000) && !ESP_WaitResponse("CONNECT", 5000)) {
        // 连接失败，尝试重新连接
        while(!(ESP_WaitResponse("OK", 5000) || ESP_WaitResponse("CONNECT", 5000))) {
            ESP_SendCommand(cmd);
        }
    }

    // 服务器连接成功LED闪烁2次
//    LED_Blink(2, 100);
}

// 向服务器发送数据
void ESP_SendData(const char* data) {
    // 计算实际数据长度（包含结束符\r\n）
    uint16_t data_len = strlen(data);  // +2 for \r\n
    
    // 准备发送指令
    char cmd[32];
    sprintf(cmd, "AT+CIPSEND=%d", data_len);
    ESP_SendCommand(cmd);
		
    if(!ESP_WaitResponse(">", 2000)) {
        // 持续尝试直到成功
        while(!ESP_WaitResponse(">", 2000)) {
            ESP_SendCommand(cmd);
        }
    }
    // 等待发送提示（>符号）
 
        // 发送实际数据 + 结束符
      USART_SendString(ESP_USART, data);
      USART_SendString(ESP_USART, "\r\n");  // 关键：添加结束符
			
			while (!ESP_WaitResponse("SEND OK", 2000));       
}




// 处理接收到的数据
void ESP_ProcessReceivedData(void) {
		
    // 1. 查找 "+IPD,"
    char *p = strstr(esp_rx_buffer, "+IPD,");
    while (p) {
        // p 指向 "+IPD,"
        // 格式： +IPD,<len>:<data>
        char *len_start = p + 5;
        int len = atoi(len_start); // 取出 len
        char *colon = strchr(len_start, ':');
        if (colon) {
            char *data_start = colon + 1;
            // 确保数据以 '\0' 结束（临时）
            char payload[256];
            int copy_len = len;
            if (copy_len > (int)sizeof(payload)-1) copy_len = sizeof(payload)-1;
            memcpy(payload, data_start, copy_len);
            payload[copy_len] = '\0';

            // 调用命令处理：payload 里可能包含多条命令或带换行
            ESP_ProcessTCPCommand(payload);

            // 清除已处理部分（把esp_rx_buffer重置或移除已处理段）
            // 为简单：清空整个缓冲区 （你也可以实现滑动窗口保留未处理的尾部）
            memset(esp_rx_buffer, 0, MAX_DATA_LEN);
            esp_rx_index = 0;
            esp_cmd_complete = 0;
            return;
        } else {
            break; // 等待完整数据
        }
    }
    // 也可以处理其它状态或原生AT反馈，例如 "OK", "ERROR"
}




//void delay_ms(uint32_t ms)
//{
//    // 8MHz时钟下的经验值系数，需要根据实际情况校准！
//    // 大约需要约2000次循环来实现1ms延时（在8MHz下）
//    #define LOOPS_PER_MS 2000
//    
//    for(uint32_t i = 0; i < ms; i++) {
//        // 内部循环实现约1ms的延时
//        volatile uint32_t j = LOOPS_PER_MS;
//        while(j--);
//    }
//}




// USART2中断处理函数
void USART2_IRQHandler(void) {
    // 检查并清除溢出错误标志（ORE）
    if (USART_GetFlagStatus(ESP_USART, USART_FLAG_ORE) == SET) {
        USART_ClearFlag(ESP_USART, USART_FLAG_ORE);
        volatile uint8_t temp = USART_ReceiveData(ESP_USART); // 读取DR以复位
    }
    
    if(USART_GetITStatus(ESP_USART, USART_IT_RXNE) != RESET) {
        uint8_t byte = USART_ReceiveData(ESP_USART);
        USART_ClearITPendingBit(ESP_USART, USART_IT_RXNE); // 立即清除标志
        
        // 保存到缓冲区（如果还有空间）
        if(esp_rx_index < MAX_DATA_LEN - 1) {
            esp_rx_buffer[esp_rx_index++] = byte;
            
            // 检查是否收到完整的命令（以换行符结束）
            if(byte == '\n') {
                esp_cmd_complete = 1;
            }
        } else {
            // 缓冲区溢出，重新开始
            esp_rx_index = 0;
            esp_error = 1;
            memset((char*)esp_rx_buffer, 0, MAX_DATA_LEN);
        }
    }
}

// 4. 指令解析函数（接收完成后调用）
void ESP_ProcessTCPCommand(const char* cmd_str)
{
	char buf[128];
    strncpy(buf, cmd_str, sizeof(buf)-1);
    buf[sizeof(buf)-1] = '\0';

    // #SET_ANGLE,90*
    if (strstr(buf, "#SET_ANGLE") != NULL) {
        char *param = strchr(buf, ',');
        if (param) {
            uint8_t angle = (uint8_t)atoi(param+1);
            sys_data.servo_manual_flag = 1;
            Servo_SetAngle(angle);
            char resp[64];
            sprintf(resp, "$ACK,SET_ANGLE,OK*");
            ESP_SendData(resp);
        }
    }
    else if (strstr(buf, "#ALARM_OFF") != NULL) {
        Alarm_RemoteOff();
        ESP_SendData("$ACK,ALARM_OFF,OK*");
    }
    else if (strstr(buf, "#GET_LOG") != NULL) {
        // 这里我把原来读取日志的伪代码做简单改写示例
        uint16_t read_cnt = 1; // 或从buf解析
        AlarmLog logs[20];
        uint32_t start_addr = log_current_addr > LOG_START_ADDR ? 
            log_current_addr - read_cnt * LOG_SIZE : LOG_START_ADDR;
        uint16_t actual_cnt = Log_Read(start_addr, logs, read_cnt);

        char resp[128];
        for (uint16_t i=0;i<actual_cnt;i++) {
            // 这里示例只发当前状态，按你原来的格式组织
            sprintf(resp, "LOG:20230901103001,%d,%d,%d,%d",
                sys_data.alarm_state, sys_data.temp, sys_data.hum, sys_data.light);
            ESP_SendData(resp);
            delay_ms(10);
        }
    }
}

void ESP_SendRealData(void) {
    char data_buf[64];
	// 格式：$DATA,温度,湿度,光照,舵机角度,报警状态*
    sprintf(data_buf, "$DATA,%d,%d,%d,%d,%d*",
            sys_data.temp, sys_data.hum, sys_data.light,
            sys_data.servo_angle, sys_data.alarm_state);
    ESP_SendData(data_buf);
}


