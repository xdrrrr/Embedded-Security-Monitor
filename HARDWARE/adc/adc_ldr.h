#ifndef ADC_H
#define ADC_H

#include "stm32f10x.h"

void ADC1_Init(void);

uint16_t ADC1_Read(void);

uint16_t ADC_GetLight(void);
#endif