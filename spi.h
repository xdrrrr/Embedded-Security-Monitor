#ifndef __SPI_H__
#define __SPI_H__

#include <stm32f10x.h>

#include "printf.h"

void SPI_Init(void);

void SPI_Write_Read_Data(unsigned char data_w[], int len_w, unsigned char data_r[], int len_r);

#endif
