// HARDWARE/timer/timer3.c
#include "tim.h"
#include "sys_data.h" // 引用全局数据


uint32_t timer_100ms_cnt = 0; 

// 定时器3初始化（100ms中断一次）
void TIM3_Init(void) {
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStru;
    NVIC_InitTypeDef NVIC_InitStru;

    // 1. 使能时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    // 2. 配置定时器（72MHz系统时钟）
    TIM_TimeBaseStru.TIM_Period = 9999;    // 自动重装载值（10000-1）
    TIM_TimeBaseStru.TIM_Prescaler = 719;  // 预分频系数（720-1）
    // 计算：72MHz/(719+1)/(9999+1) = 72000000/720/10000 = 10Hz → 100ms一次中断
    TIM_TimeBaseStru.TIM_ClockDivision = 0;
    TIM_TimeBaseStru.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStru);

    // 3. 使能中断
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

    // 4. 配置NVIC（中断优先级）
    NVIC_InitStru.NVIC_IRQChannel = TIM3_IRQn;
    NVIC_InitStru.NVIC_IRQChannelPreemptionPriority = 1; // 抢占优先级1
    NVIC_InitStru.NVIC_IRQChannelSubPriority = 2;        // 子优先级1
    NVIC_InitStru.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStru);

    // 5. 启动定时器
    TIM_Cmd(TIM3, ENABLE);
}

// 定时器3中断服务函数（设置各模块标志位）
void TIM3_IRQHandler(void) {
    static uint16_t cnt_100ms = 0; // 100ms计数器

    if (TIM_GetITStatus(TIM3, TIM_IT_Update) == SET) {
        cnt_100ms++;
		
		timer_100ms_cnt++;

        // 1. 100ms一次：光照采集、报警判断
        sys_data.flag_adc = 1;
        sys_data.flag_alarm = 1;

        // 2. 500ms一次：OLED刷新（5个100ms）
        if (cnt_100ms % 5 == 0) {
            sys_data.flag_oled = 1;
        }

        // 3. 1000ms一次：温湿度采集（10个100ms）
        if (cnt_100ms % 10 == 0) {
            sys_data.flag_dht11 = 1;
            cnt_100ms = 0; // 重置计数器，避免溢出
        }

        TIM_ClearITPendingBit(TIM3, TIM_IT_Update); // 清除中断标志
    }
}