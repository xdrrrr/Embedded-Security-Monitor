#include "stm32f10x.h"
#include "oled.h"
#include "oledfont.h"  // 依赖6x8(F6x8)、8x16(F8X16) ASCII字库
#include <string.h>    // 用于字符串处理（数字转字符串）

/* -------------------------- 引脚定义（与原有代码保持一致） -------------------------- */
#define OLED_SCL_PIN GPIO_Pin_7
#define OLED_SDA_PIN GPIO_Pin_6
#define OLED_I2C_PORT GPIOB  // 引脚端口：GPIOB
// 引脚电平控制宏
#define OLED_W_SCL(x) GPIO_WriteBit(OLED_I2C_PORT, OLED_SCL_PIN, (BitAction)(x))
#define OLED_W_SDA(x) GPIO_WriteBit(OLED_I2C_PORT, OLED_SDA_PIN, (BitAction)(x))

/* -------------------------- 基础I2C驱动（原有代码保留，确保通信正常） -------------------------- */
/**
  * @brief  I2C引脚初始化（模拟I2C，开漏输出）
  * @param  无
  * @retval 无
  */
void OLED_I2C_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);  // 使能GPIOB时钟
	
	GPIO_InitTypeDef GPIO_InitStructure;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;        // 开漏输出（I2C标准）
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;       // 50MHz速率
	GPIO_InitStructure.GPIO_Pin = OLED_SCL_PIN;
 	GPIO_Init(OLED_I2C_PORT, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = OLED_SDA_PIN;
 	GPIO_Init(OLED_I2C_PORT, &GPIO_InitStructure);
	
	OLED_W_SCL(1);  // 初始电平：SCL高
	OLED_W_SDA(1);  // 初始电平：SDA高
}

/**
  * @brief  模拟I2C起始信号（SDA从高→低，SCL保持高）
  * @param  无
  * @retval 无
  */
void OLED_I2C_Start(void)
{
	OLED_W_SDA(1);
	OLED_W_SCL(1);
	OLED_W_SDA(0);  // SDA拉低，起始信号
	OLED_W_SCL(0);  // SCL拉低，准备发送数据
}

/**
  * @brief  模拟I2C停止信号（SDA从低→高，SCL保持高）
  * @param  无
  * @retval 无
  */
void OLED_I2C_Stop(void)
{
	OLED_W_SDA(0);
	OLED_W_SCL(1);  // SCL拉高，准备停止
	OLED_W_SDA(1);  // SDA拉高，停止信号
}

/**
  * @brief  模拟I2C发送1字节数据（不处理应答，简化版）
  * @param  Byte：要发送的字节（8位）
  * @retval 无
  */
void OLED_I2C_SendByte(uint8_t Byte)
{
	uint8_t i;
	for (i = 0; i < 8; i++)
	{
		OLED_W_SDA(Byte & (0x80 >> i));  // 从最高位(bit7)开始发送
		OLED_W_SCL(1);                    // SCL拉高，从机采样
		OLED_W_SCL(0);                    // SCL拉低，准备下一位
	}
	OLED_W_SCL(1);  // 额外时钟（跳过应答，兼容多数OLED）
	OLED_W_SCL(0);
}

/**
  * @brief  OLED写入命令（I2C从机地址：0x78，命令标志：0x00）
  * @param  Command：OLED控制命令（参考SSD1306手册）
  * @retval 无
  */
void OLED_WriteCommand(uint8_t Command)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(0x78);  // OLED从机地址（默认0x78，若硬件A0接GND则为0x3C）
	OLED_I2C_SendByte(0x00);  // 写命令标志（Co=0, D/C#=0）
	OLED_I2C_SendByte(Command); 
	OLED_I2C_Stop();
}

/**
  * @brief  OLED写入数据（I2C从机地址：0x78，数据标志：0x40）
  * @param  Data：要显示的像素数据（1字节对应8个垂直像素）
  * @retval 无
  */
void OLED_WriteData(uint8_t Data)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(0x78);  // OLED从机地址
	OLED_I2C_SendByte(0x40);  // 写数据标志（Co=0, D/C#=1）
	OLED_I2C_SendByte(Data);
	OLED_I2C_Stop();
}

/* -------------------------- 核心功能实现（补充.h中声明的函数） -------------------------- */
/**
  * @brief  OLED初始化（含I2C引脚+OLED控制器配置）
  * @param  无
  * @retval 无
  * @note   基于SSD1306控制器（多数128x64 OLED采用），命令可根据硬件调整
  */
void OLED_Init(void)
{
	OLED_I2C_Init();  // 先初始化I2C引脚
	
	// ------------ OLED控制器初始化命令（SSD1306标准序列）------------
	OLED_WriteCommand(0xAE);  // 0xAE：关闭显示（初始化先关显示）
	OLED_WriteCommand(0x20);  // 0x20：设置寻址模式
	OLED_WriteCommand(0x10);  // 0x10：页寻址模式（默认常用模式）
	OLED_WriteCommand(0xA1);  // 0xA1：段重映射（0xA0为默认，0xA1左右翻转，根据屏幕调整）
	OLED_WriteCommand(0xC8);  // 0xC8：COM扫描方向（0xC0为默认，0xC8上下翻转，根据屏幕调整）
	OLED_WriteCommand(0x00);  // 0x00：设置低列地址
	OLED_WriteCommand(0x10);  // 0x10：设置高列地址
	OLED_WriteCommand(0x40);  // 0x40：设置起始行地址（第0行）
	OLED_WriteCommand(0x8D);  // 0x8D：设置电荷泵
	OLED_WriteCommand(0x14);  // 0x14：开启电荷泵（0x94为旧版，0x14更通用）
	OLED_WriteCommand(0xAF);  // 0xAF：开启显示（初始化最后开显示）
}

/**
  * @brief  设置OLED光标位置（页+列）
  * @param  Line：起始页号（对应Y轴，0~7，1页=8行像素）
  * @param  Column：起始列号（对应X轴，0~127，1列=1个像素）
  * @retval 无
  */
void OLED_SetCursor(uint8_t Line, uint8_t Column)
{
	OLED_WriteCommand(0xB0 | Line);                  // 0xB0：设置页地址（Line范围0~7）
	OLED_WriteCommand(0x10 | ((Column & 0xF0) >> 4));// 高4位列地址（Column高4位）
	OLED_WriteCommand(0x00 | (Column & 0x0F));       // 低4位列地址（Column低4位）
}

/**
  * @brief  OLED全屏清屏（填充0x00，像素全灭）
  * @param  无
  * @retval 无
  */
void OLED_Clear(void)
{  
	uint8_t Page, Column;
	for (Page = 0; Page < 8; Page++)  // 遍历8个页
	{
		OLED_SetCursor(Page, 0);      // 定位到当前页第0列
		for(Column = 0; Column < 128; Column++)  // 遍历128列
		{
			OLED_WriteData(0x00);     // 写入空白数据（全灭）
		}
	}
}

/**
  * @brief  OLED局部清屏（指定页+列范围）
  * @param  Page_Start：起始页号（0~7）
  * @param  Page_End：结束页号（0~7，需≥Page_Start）
  * @param  Col_Start：起始列号（0~127）
  * @param  Col_End：结束列号（0~127，需≥Col_Start）
  * @retval 无
  * @note   仅清除指定区域，效率高于全屏清屏
  */
void OLED_Clear_Area(uint8_t Page_Start, uint8_t Page_End, uint8_t Col_Start, uint8_t Col_End)
{
    // 参数合法性校验（避免越界或非法范围）
    if (Page_Start > Page_End || Col_Start > Col_End || Page_End > 7 || Col_End > 127)
    {
        return;  // 参数错误，不执行清屏
    }

    uint8_t Curr_Page, Curr_Col;
    for (Curr_Page = Page_Start; Curr_Page <= Page_End; Curr_Page++)
    {
        OLED_SetCursor(Curr_Page, Col_Start);  // 每页仅定位1次光标（优化效率）
        for (Curr_Col = Col_Start; Curr_Col <= Col_End; Curr_Col++)
        {
            OLED_WriteData(0x00);  // 空白数据填充
        }
    }
}

/**
  * @brief  显示单个ASCII字符（6x8字体，占6列8行）
  * @param  Line：起始页号（0~7，1页=8行，刚好对应6x8字体高度）
  * @param  Column：起始列号（0~127，需预留6列空间）
  * @param  Char：要显示的ASCII字符（32~126，即空格~波浪号）
  * @retval 无
  */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
    // 参数合法性校验（避免越界或非法字符）
    if (Line > 7 || Column > 122 || Char < 32 || Char > 126)
    {
        return;  // 超出范围，不显示
    }

    uint8_t i;
    OLED_SetCursor(Line, Column);  // 定位光标
    for (i = 0; i < 6; i++)        // 6x8字体：每个字符占6列（6字节数据）
    {
        // F6x8：6x8字库数组，Char-32：ASCII从32开始，对应数组索引0
        OLED_WriteData(F6x8[Char - 32][i]);
    }
}

/**
  * @brief  显示ASCII字符串（基于6x8字体）
  * @param  Line：起始页号（0~7）
  * @param  Column：起始列号（0~127）
  * @param  String：要显示的字符串（以'\0'结尾）
  * @retval 无
  * @note   每个字符占6列，超出127列自动换行，超出7页回到第0页
  */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
    uint8_t i = 0;
    while (String[i] != '\0')  // 遍历字符串直到结束符
    {
        OLED_ShowChar(Line, Column, String[i]);  // 显示单个字符
        Column += 6;                              // 下一个字符：列+6（6x8字体宽度）
        
        // 列越界处理：超出127列→换行（页+1，列重置为0）
        if (Column > 127)
        {
            Column = 0;
            Line++;
            Line = (Line > 7) ? 0 : Line;  // 页越界→回到第0页
        }
        i++;
    }
}

/**
  * @brief  显示单个ASCII字符（8x16字体，占8列16行，跨2页）
  * @param  Line：起始页号（0~6，因跨2页，Line+1≤7）
  * @param  Column：起始列号（0~127，需预留8列空间）
  * @param  Char：要显示的ASCII字符（32~126）
  * @retval 无
  */
static void OLED_ShowChar_8x16(uint8_t Line, uint8_t Column, char Char)
{
    // 参数合法性校验（8x16跨2页，Line最大为6；列需预留8列）
    if (Line > 6 || Column > 120 || Char < 32 || Char > 126)
    {
        return;
    }

    uint8_t i;
    // 上8行（第Line页）
    OLED_SetCursor(Line, Column);
    for (i = 0; i < 8; i++)
    {
        // F8X16：8x16字库数组，前8字节为上8行
        OLED_WriteData(F8X16[Char - 32][i]);
    }
    // 下8行（第Line+1页）
    OLED_SetCursor(Line + 1, Column);
    for (i = 8; i < 16; i++)
    {
        // 后8字节为下8行
        OLED_WriteData(F8X16[Char - 32][i]);
    }
}

/**
  * @brief  显示数字（支持6x8/8x16字体，不足位数补0）
  * @param  Line：起始页号（0~7，8x16时需≤6）
  * @param  Column：起始列号（0~127）
  * @param  Number：要显示的数字（0~4294967295，即uint32_t最大值）
  * @param  Length：显示位数（1~10，不足补0）
  * @param  Size：字体大小（6=6x8，16=8x16）
  * @retval 无
  */
void OLED_ShowNum_size(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length, uint8_t Size)
{
    // 参数合法性校验（位数1~10，字体仅支持6/16）
    if (Length < 1 || Length > 10 || (Size != 6 && Size != 16))
    {
        return;
    }

    uint8_t i;
    char Num_Str[11] = {0};  // 缓存数字字符串（最大10位+结束符）

    // 数字转字符串（不足Length位补0）
    if (Number == 0)
    {
        // 数字为0时，强制填充Length个0
        memset(Num_Str, '0', Length);
    }
    else
    {
        // 从低位到高位取数，逆序存入数组
        for (i = 0; i < Length; i++)
        {
            Num_Str[Length - 1 - i] = '0' + (Number % 10);  // 个位转字符
            Number /= 10;  // 去掉已处理的个位
            // 数字不足Length位时，剩余高位补0
            if (Number == 0 && i < Length - 1)
            {
                for (; i < Length - 1; i++)
                {
                    Num_Str[Length - 1 - i - 1] = '0';
                }
                break;
            }
        }
    }

    // 根据字体大小显示字符串
    if (Size == 6)
    {
        // 6x8字体：复用OLED_ShowString
        OLED_ShowString(Line, Column, Num_Str);
    }
    else if (Size == 16)
    {
        // 8x16字体：逐个字符调用OLED_ShowChar_8x16
        for (i = 0; i < Length; i++)
        {
            OLED_ShowChar_8x16(Line, Column, Num_Str[i]);
            Column += 8;  // 下一个字符：列+8（8x16字体宽度）
            
            // 列越界处理：超出127列→换行（页+2，因8x16跨2页）
            if (Column > 127)
            {
                Column = 0;
                Line += 2;
                Line = (Line > 7) ? 0 : Line;  // 页越界→回到第0页
            }
        }
    }
}

/**
  * @brief  显示数字（默认6x8字体，不足位数补0）
  * @param  Line：起始页号（0~7）
  * @param  Column：起始列号（0~127）
  * @param  Number：要显示的数字（0~4294967295）
  * @param  Length：显示位数（1~10）
  * @retval 无
  * @note   简化接口，调用OLED_ShowNum_size并默认Size=6
  */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    OLED_ShowNum_size(Line, Column, Number, Length, 6);
}