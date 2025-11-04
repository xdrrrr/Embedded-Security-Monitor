#ifndef __FLASH_H__
#define __FLASH_H__

#include "SPI.h"
#include <stdio.h>
#include <string.h>


void Flash_Init(void);

void Flash_WriteData(unsigned char addr[], unsigned char data[], int len);


void FLash_ReadData(unsigned char addr[], unsigned char data[], int len);

#endif
