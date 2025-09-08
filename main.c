#include "stm32f10x.h"                  // Device header
#include "sys_data.h"
#include "delay.h"
#include "printf.h"
#include "tim.h"

#include "dht11/dht11.h"
#include "adc/adc_ldr.h"
#include "oled/OLED.h"
#include "led_rgb/led_rgb.h"
#include "beep/beep.h"
#include "Servo/Servo.h"
#include "alarm.h"
#include "flash/w25qxx.h" 
#include "log.h"
//#include "esp8266.h"
#include "usart/usart1.h"
#include "led.h"


uint8_t prev_alarm_state = SYS_NORMAL;//避免无效日志

//// 实现一个简单的毫秒中断服务程序（例如使用 SysTick）
//void SysTick_Handler(void) {
//    millis_tick++;
//}


int main(void) {
    // 1. 初始化基础外设
    delay_init(72);       // 延时初始化
    TIM3_Init();        // 定时器3初始化（100ms中断）
//    printf_init();
	USART1_Init(115200);
	
	ADC1_Init();
	dht11_init();
	
	OLED_Init();
	OLED_Clear();
	delay_ms(1000);
	
	LED_RGB_Init();
	BEEP_Init();
	Servo_init();
	W25QXX_Init();


	
//	//esp8266
//	 // 系统初始化
//    System_Init();
//	delay_init(72);       // 延时初始化
//    LED_Init();
//    USART_MyInit();
//    
//    	
//    // 连接到WiFi - 设置你的WiFi名称和密码
//    //ESP_ConnectWiFi(myaccount, mypasswd);
//		
//		
//     if (SysTick_Config(SystemCoreClock / 1000)) {
//        while (1); // 初始化 SysTick 失败
//    } 

//		
//    // 连接到TCP服务器 - 设置上位机的IP和端口
//    ESP_ConnectServer(myip, myport);
	
	// 2. 主循环（只做“标志位检测+功能执行”）
    while (1) {
        // 光照采集（100ms一次）
        if (sys_data.flag_adc) {

			
			// 修改后（转换为与光照成正比的数值）
			uint16_t adc_raw = ADC_GetLight();  // 读取原始ADC值
			sys_data.prev_light = sys_data.light;  // 保存上一次转换后的值
			sys_data.light = 1000 - adc_raw;    // 转换：反转原始值，与光照成正比
			
//            printf("Light: %d lux\r\n", sys_data.light);
            sys_data.flag_adc = 0;
			
			Log_TriggerSave();
			Alarm_Check();
			LED_Alarm_Control();
			BEEP_Alarm_Control();
			Servo_Link_Alarm();
			
        }

        // OLED刷新（200ms一次）
        if (sys_data.flag_oled) {


			// ---------- 1. 标题区域：局部清屏 + 重绘 ----------
            OLED_Clear_Area(0, 0, 0, 64);   // 只清“页0，列0~64”
            OLED_ShowString(0, 0, "Env Monitor");

            // ---------- 2. 温度区域：局部清屏 + 重绘 ----------
            OLED_Clear_Area(1, 1, 0, 68);   // 只清“页1~2，列0~68”
            OLED_ShowString(1, 0, "Temp:"); // 6x8 字体显示标签
            OLED_ShowNum_size(1, 40, sys_data.temp, 2, 6); // 2位6x8字体显示温度
            OLED_ShowChar(1, 60, 'C'); // 8x16 字体显示“C”（若要“℃”需字库支持）

            // ---------- 3. 湿度区域：局部清屏 + 重绘 ----------
            OLED_Clear_Area(2, 2, 0, 68);   // 只清“页3~4，列0~68”
            OLED_ShowString(2, 0, "Hum:");  // 6x8 字体显示标签
            OLED_ShowNum_size(2, 40, sys_data.hum, 2, 6); // 2位6x8字体显示湿度
            OLED_ShowChar(2, 60, '%'); // 8x16 字体显示“%”

            // ---------- 4. 光照区域：局部清屏 + 重绘 ----------
            OLED_Clear_Area(3, 3, 0, 100);  // 只清“页5~6，列0~100”
            OLED_ShowString(3, 0, "Light:");// 6x8 字体显示标签
            OLED_ShowNum_size(3, 50, sys_data.light, 4, 6); // 4位6x8字体显示光照
            OLED_ShowString(3, 90, "lux");  // 6x8 字体显示单位
			
			// 显示舵机角度
			OLED_Clear_Area(4, 4, 0, 100);
            OLED_ShowString(4, 0, "Servo:");
            OLED_ShowNum_size(4, 50, sys_data.servo_angle, 3, 6);
            OLED_ShowChar(4, 80, 'o');       // 用“o”代替“°”
			OLED_ShowChar(4, 90, 'C');       // 显示“C”表示摄氏度
			
			
			// 4. 在OLED上显示报警状态
			OLED_Clear_Area(5, 5, 0, 127);
			OLED_ShowString(5, 0, "State: ");
			switch (sys_data.alarm_state) {
				case SYS_NORMAL: OLED_ShowString(5, 60, "Normal"); break;
				case SYS_ENV_ALARM: OLED_ShowString(5, 60, "Env Alarm"); break;
				case SYS_INTRUDE_ALARM: OLED_ShowString(5, 60, "Intrude!"); break;
			}
			
			
			
            sys_data.flag_oled = 0; // 重置刷新标志，避免重复刷新

        }
		// 温湿度采集
        if (sys_data.flag_dht11) 
		{
            if (DHT11_ReadData(&sys_data.temp, &sys_data.hum) == true) 
			{
//                printf("Temp: %d℃, Hum: %d%%\r\n", sys_data.temp, sys_data.hum);
            } else 
			{
//                printf("DHT11 Read Fail\r\n");
            }
			
			
            sys_data.flag_dht11 = 0;
        }
		
//		//esp8266
//		 // 如果有数据接收完成，处理数据
//        if(esp_cmd_complete) {
//            ESP_ProcessReceivedData();
//            esp_cmd_complete = 0;
//            esp_rx_index = 0;
//            memset((char*)esp_rx_buffer, 0, MAX_DATA_LEN);
//        }
//        
//		ESP_SendData("111\r\n");
		// 串口指令解析（接收完成后处理）
		USART1_SendRealData();
//		led_on();
        if (usart1_rx_complete) {
			delay_ms(100);
            USART1_ParseCmd(); // 调用解析函数
        }
    }
} 