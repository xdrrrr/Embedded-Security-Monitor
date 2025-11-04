#include "flash.h"

void Flash_Init(void)
{
	SPI_Init();
}


void Flash_WriteData(unsigned char addr[], unsigned char data[], int len)
{
	printf("%s\r\n", data);
	unsigned char temp[260] = {0};
//	printf("----%d----\r\n", __LINE__);
	//1、写启用
	temp[0] = 0x06;
	SPI_Write_Read_Data(temp, 1, NULL, 0);
	
	//2、擦除
	temp[0] = 0x20;
	memcpy(temp+1, addr, 3);
	SPI_Write_Read_Data(temp, 4, NULL, 0);
	
	//3、等待擦除完成
	unsigned char status = 0;
	temp[0] = 0x05;
	do
	{
		status = 0;
		//printf("----%d----\r\n", __LINE__);
		SPI_Write_Read_Data(temp, 1, &status, 1);
		printf("status = %d\r\n", status);
	}while((status&0x01) == 1);
	
	//4、写启用
	temp[0] = 0x06;
	SPI_Write_Read_Data(temp, 1, NULL, 0);
	//5、写数据
	temp[0] = 0x02;
	memcpy(temp+1, addr, 3);
	memcpy(temp+4, data, len);
	SPI_Write_Read_Data(temp, len+4, NULL, 0);
	
//	printf("----%d----\r\n", __LINE__);
	temp[0] = 0x05;
	//6、等待写入完成
	do
	{
		status = 0;
		SPI_Write_Read_Data(temp, 1, &status, 1);
	}while((status&0x01) == 1);
//	printf("----%d----\r\n", __LINE__);
}


void FLash_ReadData(unsigned char addr[], unsigned char data[], int len)
{
	unsigned char temp[4] = {0};
	temp[0] = 0x03;
	memcpy(temp+1, addr, 3);
	SPI_Write_Read_Data(temp, 4, data, len);
}
