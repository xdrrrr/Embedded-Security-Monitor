#ifndef __W25QXX_H
#define __W25QXX_H			    
#include "stm32f10x.h"
#include "delay.h"

/*
    余浩W25Q64驱动程序  模拟SPI驱动
		微信：YH_Memory
		版本：21.01.06
*/

	  
//W25X系列/Q系列芯片列表	   
#define W25Q80 	0XEF13 	 //61203
#define W25Q16 	0XEF14   //61204
#define W25Q32 	0XEF15   //61205
#define W25Q64 	0XEF16   //61206
#define W25Q128	0XEF17   //61207
   

/************管脚配置**************/
//SCK 配置
#define W25QXX_SCK_RCC		RCC_APB2Periph_GPIOB
#define W25QXX_SCK_PORT		GPIOB			  	    													  
#define W25QXX_SCK_PIN		GPIO_Pin_13
//MOSI 配置
#define W25QXX_MOSI_RCC		RCC_APB2Periph_GPIOB
#define W25QXX_MOSI_PORT	GPIOB				  	    													  
#define W25QXX_MOSI_PIN		GPIO_Pin_14
//MISO 配置	
#define W25QXX_MISO_RCC		RCC_APB2Periph_GPIOB
#define W25QXX_MISO_PORT	GPIOB				  	    													  
#define W25QXX_MISO_PIN		GPIO_Pin_15
//CS 配置	
#define W25QXX_CS_RCC		  RCC_APB2Periph_GPIOB
#define W25QXX_CS_PORT	  	GPIOB				  	    													  
#define W25QXX_CS_PIN		  GPIO_Pin_12



#define W25QXX_SCK_1  		GPIO_SetBits(W25QXX_SCK_PORT,W25QXX_SCK_PIN)
#define W25QXX_SCK_0  		GPIO_ResetBits(W25QXX_SCK_PORT,W25QXX_SCK_PIN)
#define W25QXX_MOSI_1  		GPIO_SetBits(W25QXX_MOSI_PORT,W25QXX_MOSI_PIN)
#define W25QXX_MOSI_0  		GPIO_ResetBits(W25QXX_MOSI_PORT,W25QXX_MOSI_PIN)
#define W25QXX_CS_1  			GPIO_SetBits(W25QXX_CS_PORT,W25QXX_CS_PIN)
#define W25QXX_CS_0  			GPIO_ResetBits(W25QXX_CS_PORT,W25QXX_CS_PIN)
#define W25QXX_READ_MISO 	GPIO_ReadInputDataBit(W25QXX_MISO_PORT, W25QXX_MISO_PIN)
/**********************************/


 
//指令表
#define W25X_WriteEnable			0x06 
#define W25X_WriteDisable			0x04 
#define W25X_ReadStatusReg		0x05 
#define W25X_WriteStatusReg		0x01 
#define W25X_ReadData					0x03 
#define W25X_FastReadData			0x0B 
#define W25X_FastReadDual			0x3B 
#define W25X_PageProgram			0x02 
#define W25X_BlockErase				0xD8 
#define W25X_SectorErase			0x20 
#define W25X_ChipErase				0xC7 
#define W25X_PowerDown				0xB9 
#define W25X_ReleasePowerDown	0xAB 
#define W25X_DeviceID					0xAB 
#define W25X_ManufactDeviceID	0x90 
#define W25X_JedecDeviceID		0x9F 

//spi读写
uint8_t W25QXX_ReadWriteByte(uint8_t txData);

//W25QXX 操作函数
void      W25QXX_Init(void);             			 //初始化W25QXX
uint16_t  W25QXX_ReadID(void);  	 						 //读取FLASH ID
uint8_t	  W25QXX_ReadSR(void);        				 //读取状态寄存器 
void      W25QXX_Write_SR(uint8_t sr);  			 //写状态寄存器
void 			W25QXX_Write_Enable(void);  				 //写使能 
void 			W25QXX_Write_Disable(void);		       //写保护
void 			W25QXX_Write_NoCheck(uint8_t* pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite);  //无检验写FLASH 
void 			W25QXX_Read(uint8_t* pBuffer,uint32_t ReadAddr,uint16_t NumByteToRead);   					//读取FLASH
void 			W25QXX_Write(uint8_t* pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite);					//写入FLASH
void 			W25QXX_Erase_Chip(void);    	  			//整片擦除
void 			W25QXX_Erase_Sector(uint32_t Dst_Addr);		//扇区擦除
void 			W25QXX_Wait_Busy(void);           	  //等待空闲
void      W25QXX_PowerDown(void);        				//进入掉电模式
void      W25QXX_WAKEUP(void);									//唤醒
//SPI在一页(0~65535)内写入少于256个字节的数据
//在指定地址开始写入最大256字节的数据
//pBuffer:数据存储区
//WriteAddr:开始写入的地址(24bit)
//NumByteToWrite:要写入的字节数(最大256),该数不应该超过该页的剩余字节数!!!	 
void W25QXX_Write_Page(uint8_t* pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite);

#endif
















