#include "delay.h"
#include "stm32f10x.h"

static uint8_t fac_us = 0; // 微秒延时倍乘数

// 初始化延时函数
// SYSCLK: 系统时钟频率（单位：MHz），例如72MHz
void delay_init(uint8_t SYSCLK) {
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8); // 选择外部时钟（HCLK/8）
    fac_us = SYSCLK / 8; // 计算微秒延时倍乘数
    // 注：HCLK为系统时钟，STM32F103默认72MHz，因此fac_us=72/8=9
}

// 微秒级延时
// us: 延时微秒数（范围：0~2^24/ fac_us，72MHz下约为1864135us）
void delay_us(uint32_t us) {
    uint32_t temp;
    SysTick->LOAD = us * fac_us; // 加载计数值（LOAD = 延时时间 * 频率）
    SysTick->VAL = 0x00; // 清空计数器
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk; // 启动计数器
    
    // 等待计数结束（CTRL寄存器的第16位为1时表示计数完成）
    do {
        temp = SysTick->CTRL;
    } while ((temp & 0x01) && !(temp & (1 << 16)));
    
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk; // 关闭计数器
    SysTick->VAL = 0x00; // 清空计数器
}

// 毫秒级延时（基于微秒延时实现）
// ms: 延时毫秒数
void delay_ms(uint16_t ms) {
    uint16_t i;
    for (i = 0; i < ms; i++) {
        delay_us(1000); // 每次延时1000微秒（1毫秒）
    }
}


//void delay_ms(uint32_t ms)
//{
//    // 8MHz时钟下的经验值系数，需要根据实际情况校准！
//    // 大约需要约2000次循环来实现1ms延时（在8MHz下）
//    #define LOOPS_PER_MS 2000
//    
//    for(uint32_t i = 0; i < ms; i++) {
//        // 内部循环实现约1ms的延时
//        volatile uint32_t j = LOOPS_PER_MS;
//        while(j--);
//    }
//}

