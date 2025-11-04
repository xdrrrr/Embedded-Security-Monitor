#include "spi.h"

void SPI_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	//CS  B12--CS,13--SCLK,14--MOSI,15--MISO
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	//MISO
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	//模式1空闲状态电平
	GPIO_WriteBit(GPIOB, GPIO_Pin_12, Bit_RESET);
	GPIO_WriteBit(GPIOB, GPIO_Pin_13, Bit_RESET);
}

void CS_OUT(int x)
{
	if(x == 1)
		GPIO_WriteBit(GPIOB, GPIO_Pin_12, Bit_SET);
	else
		GPIO_WriteBit(GPIOB, GPIO_Pin_12, Bit_RESET);
}


void SCLK_OUT(int x)
{
	if(x == 1)
		GPIO_WriteBit(GPIOB, GPIO_Pin_13, Bit_SET);
	else
		GPIO_WriteBit(GPIOB, GPIO_Pin_13, Bit_RESET);
}


void MOSI_OUT(int x)
{
	if(x == 1)
		GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_SET);
	else
		GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_RESET);
}

int MISO_IN(void)
{
	return GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_15);
}

//模式1
void SPI_Write_Read_Data(unsigned char data_w[], int len_w, unsigned char data_r[], int len_r)
{
//	if(len_w != 0)
//	{
//		for(int i = 0; i < len_w; i++)
//			printf("data_w = %X\r\n", data_w[i]);
//	}
	
	//选中设备
	CS_OUT(0);
	
	//准备发送的数据
	for(int i = 0; i < len_w; i++)
	{
		for(int j = 0; j < 8; j++)
		{
			if(((data_w[i] >> (7-j) )& 0x1) == 1)
			{
//				printf("1 ");
				MOSI_OUT(1);
			}
			else
			{
//				printf("0 ");
				MOSI_OUT(0);
			}
			//产生时钟边沿
			SCLK_OUT(0);
			SCLK_OUT(1);
		}
	}
//	printf("\r\n");
	for(int i = 0; i < len_r; i++)
	{
		for(int j = 0; j < 8; j++)
		{
			//产生接收的时钟边沿
			SCLK_OUT(1);
			SCLK_OUT(0);
			//接受数据
			if(MISO_IN() == 1)
			{
				data_r[i] |= 1 << (7-j);
//				printf("1 ");
			}
			else
			{
				data_r[i] |= 0 << (7-j);
//				printf("0 ");
			}
		}
	}
//	printf("\r\n");
	//取消选中
	CS_OUT(1);
	
//	if(len_r != 0)
//		for(int i = 0; i < len_r; i++)
//			printf("data_r = %X\r\n", data_r[i]);
}
