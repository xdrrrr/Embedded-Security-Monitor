#ifndef __OLED_H
#define __OLED_H
#include "stm32f10x.h"  

void OLED_Init(void);
void OLED_Clear(void);

void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowNum_size(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length, uint8_t Size);



void OLED_Clear_Area(uint8_t Page_Start, uint8_t Page_End, uint8_t Col_Start, uint8_t Col_End);

#endif



/*
#ifndef __OLED_H__
#define __OLED_H__

#include "iic.h"
#include <string.h>
#include "stm32f10x.h"                  // Device header

void OLED_Init(void);


void OLED_Clear(void);

void OLED_Clear_Area(uint8_t page_start, uint8_t page_end, uint8_t col_start, uint8_t col_end);

void OLED_ShowChar(int x, int y, unsigned char ch);

void OLED_ShowChar_8x16(int x, int y, unsigned char ch);

void OLED_ShowString(int x, int y, char *str);

void OLED_ShowNum_6X8(uint8_t x, uint8_t y, uint32_t num, uint8_t len);

void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size);

#endif
*/