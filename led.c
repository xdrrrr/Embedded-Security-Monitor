#include "led.h"
#include "stm32f10x.h"   

void led_init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_SetBits(GPIOC, GPIO_Pin_13);
}

void led_off(void)
{
	GPIO_SetBits(GPIOC, GPIO_Pin_13);
}

void led_on(void)
{
	GPIO_ResetBits(GPIOC, GPIO_Pin_13);
}