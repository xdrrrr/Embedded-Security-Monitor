#include "esp8266.h"


// 全局变量
char esp_rx_buffer[MAX_DATA_LEN];
volatile uint16_t esp_rx_index = 0;
volatile uint8_t esp_cmd_complete = 0;
volatile uint8_t esp_error = 0;
volatile uint32_t millis_tick = 0;





// 系统初始化
void System_Init(void) {
    // 配置系统时钟
    SystemInit();
    SystemCoreClockUpdate();
    
    // 开启外设时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
}

// LED初始化
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
		
		
    // 检查是否有IPD数据 (+IPD,<len>:<data>)
		
		
		if(strstr((char*)esp_rx_buffer, "#GET_LOG*"))
		GPIO_ResetBits(DEBUG_LED_PORT, DEBUG_LED_PIN);	
  	else if(strstr((char*)esp_rx_buffer, "#SET_ANGLE,91*"))
		GPIO_SetBits(DEBUG_LED_PORT, DEBUG_LED_PIN);

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

