#include "Servo.h"
#include "stm32f10x.h" 
#include "sys_data.h"

void Servo_init(void)
{
	//使能GPIOA和TIM2
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);
	
	//配置GPIOA1
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	//配置TIM时基
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_TimeBaseStructure.TIM_Period = 199;
	TIM_TimeBaseStructure.TIM_Prescaler = 7199;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, & TIM_TimeBaseStructure);
	
	//配置TIM2通道2pwm
	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_Pulse = 15;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OC2Init(TIM2, & TIM_OCInitStructure);
	
	// 使能TIM2在CCR2上的预装载寄存器
	TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);
	// 使能TIM2在CCR2上的预装载寄存器
	TIM_ARRPreloadConfig(TIM2, ENABLE);
	
	TIM_ClearFlag(TIM2, TIM_FLAG_Update);
	TIM_ClearITPendingBit(TIM2, TIM_FLAG_Update);
	TIM_Cmd(TIM2, ENABLE);
}

// 设置舵机角度（0-180°）
void Servo_SetAngle(uint8_t angle) {
    // 角度限位（避免超范围）
    if (angle > 180) angle = 180;
    if (angle < 0) angle = 0;
    
    // 更新全局变量（供其他模块读取当前角度）
    sys_data.servo_angle = angle;
    
    // 角度→PWM比较值映射：
    // 舵机特性：0.5ms脉冲→0°，2.5ms脉冲→180°（周期20ms）
    // 计数器频率10kHz→1个计数单位=0.1ms，故：
    // 0.5ms → 5个单位，2.5ms → 25个单位
    // 公式：PWM值 = 5 + (angle × 20) / 180 （20=25-5，对应180°的差值）
    uint16_t pwm_val = 5 + (angle * 20) / 180;
    
    // 设置通道2的比较值（与初始化的TIM2_CH2匹配）
    TIM_SetCompare2(TIM2, pwm_val);
}

// 舵机与报警联动（入侵时转向45°，正常归中90°）
void Servo_Link_Alarm(void) {
    // 与报警模块的枚举ALARM_INTRUDE保持一致
	if (sys_data.servo_manual_flag == 1)
	{
		return ;
	}
    if (sys_data.alarm_state == SYS_INTRUDE_ALARM) {
        Servo_SetAngle(45);  // 入侵时转向45°
    } else {
        Servo_SetAngle(90);  // 正常时归中
    }
}