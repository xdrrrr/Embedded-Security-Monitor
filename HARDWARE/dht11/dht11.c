#include "dht11.h"
#include "delay.h"

//PA6
#define DHT11_PORT GPIOA
#define DHT11_PIN GPIO_Pin_6
#define DHT11_RCC RCC_APB2Periph_GPIOA

void dht11_init(void)
{
		RCC_APB2PeriphClockCmd(DHT11_RCC, ENABLE); 
}

void DHT11_PinOutputModeConfig(void)
{
	GPIO_InitTypeDef GPIO_InitStructure; 
	GPIO_InitStructure.GPIO_Pin = DHT11_PIN; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
	GPIO_Init(DHT11_PORT, &GPIO_InitStructure);
}

void DHT11_PinInputModeConfig(void)
{
	GPIO_InitTypeDef GPIO_InitStructure; 
	GPIO_InitStructure.GPIO_Pin = DHT11_PIN; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 
	GPIO_Init(DHT11_PORT, &GPIO_InitStructure);
}

//向DHT11发送开始信号
void DHT11_SendStart(void)
{
	//1.配置引脚为输出模式
	DHT11_PinOutputModeConfig();
	
	//2.把引脚电平拉低并持续20ms
	GPIO_ResetBits(DHT11_PORT,DHT11_PIN);
	delay_ms(20);
	
	//3.把引脚电平拉高并持续30us
	GPIO_SetBits(DHT11_PORT,DHT11_PIN);
	delay_us(30);
}

//判断DHT11是否响应
bool DHT11_IsACK(void)
{
	uint32_t cnt = 0;  //作为计数器
	//1.配置引脚为输入模式
	DHT11_PinInputModeConfig();
	//2.判断PG9引脚是否检测到低电平  为了提高程序可靠性，所以人为添加超时机制，超时时间假设为100us
	while( GPIO_ReadInputDataBit(DHT11_PORT,DHT11_PIN) == 1 && cnt < 100)
	{
//		delay_us(1);
		cnt++;
	}
	if(cnt >= 100)
		return false;
	cnt = 0;
	
	//3.判断PG9引脚检测的低电平是否持续80us
	while( GPIO_ReadInputDataBit(DHT11_PORT,DHT11_PIN) == 0 && cnt < 100)
	{
		delay_us(1);
		cnt++;
	}
	if( cnt >= 100 )
		return false;
	else
		return true;
}

//判断DHT11发送的bit的值，并存储到一个字节的bit0位置中
uint8_t DHT11_ReadBit(void)
{
	//1.等待低电平出现   
	while( GPIO_ReadInputDataBit(DHT11_PORT,DHT11_PIN) == 1 );
	
	//1.等待低电平结束   
	while( GPIO_ReadInputDataBit(DHT11_PORT,DHT11_PIN) == 0 );
	
	//2.此时高电平出现，则延时 28us < n < 70us
	delay_us(40);
	
	//3.延时结束之后，判断PG9引脚的电平状态，如果电平还是高电平，则说明是bit = 1
	if( GPIO_ReadInputDataBit(DHT11_PORT,DHT11_PIN) == 1 )
		return 1;
	else 
		return 0;
}

//DHT11读取1字节  DHT11提供的40bit是以MSB
uint8_t DHT11_ReadByte(void)
{
	int i = 0;
	//1.定义变量并初始化
	uint8_t data = 0;  // 0000 0000
	
	//2.循环8次，接收一个字节
	for(i = 0;i < 8;i++)
	{
		data <<= 1;
		data |= DHT11_ReadBit(); 
	} 
	
	return data;
}

//读取DHT11温湿度传感器的数据
bool DHT11_ReadData(volatile uint8_t *temp,volatile uint8_t *hum)
{
	uint8_t buf[5] = {0};
	int i = 0;
	
	//1.MCU发送开始信号
	DHT11_SendStart();
	
	//2.MCU等待DHT进行响应
	if( true == DHT11_IsACK() )
	{
		//3.循环读取40bit
		for(i=0;i<5;i++)
		{
			buf[i] = DHT11_ReadByte();
		}
		
		//4.对数据进行校验
		if( buf[4] == buf[0] + buf[1] + buf[2] + buf[3] )
		{
			*hum = buf[0];
			*temp = buf[2];
			return true;
		}
		else
			return false; //说明读取数据失败，原因是校验未通过
	}
	else
	{
		return false; //说明读取数据失败，原因是DHT未响应
	}
}



































/*
void dht11_init(void)
{
		RCC_APB2PeriphClockCmd(DHT11_RCC, ENABLE); 
}

static void DHT11_OUT(int x)
{
	GPIO_InitTypeDef GPIO_InitStructure; 
	GPIO_InitStructure.GPIO_Pin = DHT11_PIN; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
	GPIO_Init(DHT11_PORT, &GPIO_InitStructure);
	
	if(x)
		GPIO_SetBits(DHT11_PORT, DHT11_PIN); 
	else
		GPIO_ResetBits(DHT11_PORT, DHT11_PIN); 
}

static int DHT11_IN(void)
{
	GPIO_InitTypeDef GPIO_InitStructure; 
	GPIO_InitStructure.GPIO_Pin = DHT11_PIN; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 
	GPIO_Init(DHT11_PORT, &GPIO_InitStructure);
	
	return GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN); 
}

static int read_bit()
{
	int i = 0;
	while(DHT11_IN());
	while(!DHT11_IN());//吃低电平
	
#if 0
	delay_us(40);

	if(DHT11_IN())
		return 1;
	else
		return 0;
#else
	while(GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == 1)
	{
		delay_us(1);
		i++;		
	}
	
	if(i>40)
		return 1;
	else
		return 0;
#endif
}

int dht11_read(unsigned char *w, unsigned char *h)
{
	//1. PA6输出//2. 握手
	DHT11_OUT(1);
	delay_ms(50);
	DHT11_OUT(0);
	delay_ms(20);
	DHT11_OUT(1);
	delay_us(30);

	//4. 理解握手
	int i = 0;
	while(DHT11_IN())//吃高电平
	{
		delay_us(1);
		i++;
		if(i > 40)//对方没回复
			return -1;
	}
	
	while(!DHT11_IN())//吃di电平
	{
		i = 0;
		delay_us(1);
		i++;
		if(i > 90)//对方没回复
			return -2;
	}
	
	while(DHT11_IN());//吃高电平
	
	//5. 读入40bit
	unsigned char buf[5] = {0};
	for(int i = 0; i<5; i++)
	{
		for(int j=0; j<8; j++)
		{
			buf[i] |= (read_bit() << (7-j));
		}
	}
	//6. 校验
	if(buf[4] == buf[0]+buf[1]+buf[2]+buf[3])
	//7. 返回
	{	
		*w = buf[2];
		*h = buf[0];
		return 0;
	}
	return -3;
}
*/
