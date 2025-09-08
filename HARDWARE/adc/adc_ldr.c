#include "adc/adc_ldr.h"

void ADC1_Init(void)
{
	// 1. 使能时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);
	// ADC 时钟分频（ADC 时钟不得超过 14MHz，这里 72MHz / 6 = 12MHz）
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	ADC_InitTypeDef ADC_InitStructure;
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv =
					ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADC1, &ADC_InitStructure);
	
	ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1,ADC_SampleTime_55Cycles5);
	
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC2));
	
	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC2));
	
	ADC_Cmd(ADC1, ENABLE);
}

uint16_t ADC1_Read(void)
{
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
	
	ADC_ClearFlag(ADC2, ADC_FLAG_EOC);
	
	return ADC_GetConversionValue(ADC1) & 0xfff;
}

// 转换为光照值（0-1000lux）
uint16_t ADC_GetLight(void) 
{
    uint16_t adc_val = ADC1_Read();
    return (adc_val * 1000) / 4095; // 0-4095 → 0-1000lux
}