#include "led_rgb.h"
#include "sys_data.h"  // 引用全局系统数据（获取报警状态）


// 引脚定义：共阴三色灯（R=PB1, G=PB2, B=PB4）
#define LED_R_PIN    GPIO_Pin_3
#define LED_G_PIN    GPIO_Pin_4
#define LED_B_PIN    GPIO_Pin_5
#define LED_PORT     GPIOB
#define LED_RCC_PORT RCC_APB2Periph_GPIOB

/**
 * @brief 三色灯GPIO初始化（推挽输出）
 */
void LED_RGB_Init(void) {
    GPIO_InitTypeDef GPIO_InitStru;
    
	// 1. 必须先使能AFIO时钟（用于引脚重映射）
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    // 2. 禁用JTAG功能，释放PB3/PB4，保留SWD调试（不影响下载）
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    // 使能GPIO时钟
    RCC_APB2PeriphClockCmd(LED_RCC_PORT, ENABLE);
    
    // 配置RGB引脚为推挽输出（高电平点亮，低电平熄灭）
    GPIO_InitStru.GPIO_Pin = LED_R_PIN | LED_G_PIN | LED_B_PIN;
    GPIO_InitStru.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStru.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED_PORT, &GPIO_InitStru);
    
    // 初始状态：全灭（低电平）
    GPIO_ResetBits(LED_PORT, LED_R_PIN | LED_G_PIN | LED_B_PIN);
}

/**
 * @brief 根据系统报警状态控制LED颜色
 * @param 无（依赖全局变量sys_data.alarm_state）
 * @note 状态映射：
 * - SYS_NORMAL：绿灯亮（G=高，R/B=低）
 * - SYS_ENV_ALARM：黄灯亮（R/G=高，B=低）
 * - SYS_INTRUDE_ALARM：红灯亮（R=高，G/B=低）
 */
void LED_Alarm_Control(void) {
    switch (sys_data.alarm_state) {
        case SYS_NORMAL:  // 正常状态：绿灯
            GPIO_ResetBits(LED_PORT, LED_R_PIN | LED_B_PIN);  // R、B灭（低电平）
            GPIO_SetBits(LED_PORT, LED_G_PIN);                // G亮（高电平）
            break;
            
        case SYS_ENV_ALARM:  // 环境报警：黄灯（红+绿）
            GPIO_ResetBits(LED_PORT, LED_B_PIN);              // B灭（低电平）
            GPIO_SetBits(LED_PORT, LED_R_PIN | LED_G_PIN);    // R、G亮（高电平）
            break;
            
        case SYS_INTRUDE_ALARM:  // 入侵报警：红灯
            GPIO_ResetBits(LED_PORT, LED_G_PIN | LED_B_PIN);  // G、B灭（低电平）
            GPIO_SetBits(LED_PORT, LED_R_PIN);                // R亮（高电平）
            break;
            
        default:  // 异常状态：全灭
            GPIO_ResetBits(LED_PORT, LED_R_PIN | LED_G_PIN | LED_B_PIN);  // 全低电平
            break;
    }
}
