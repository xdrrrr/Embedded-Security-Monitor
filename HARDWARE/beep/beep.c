// HARDWARE/beep/beep.c
#include "beep.h"
#include "sys_data.h"  // 引用全局系统数据（报警状态、OLED周期标志）

// 硬件定义：无源蜂鸣器（TIM1_CH1复用PA8引脚）
#define BEEP_TIM        TIM1                  // PA8默认复用TIM1_CH1
#define BEEP_TIM_RCC    RCC_APB2Periph_TIM1    // TIM1挂载APB2总线
#define BEEP_GPIO_PORT  GPIOA                 // 改为GPIOA
#define BEEP_GPIO_PIN   GPIO_Pin_8             // 改为PA8引脚
#define BEEP_GPIO_RCC   RCC_APB2Periph_GPIOA   // 使能GPIOA时钟
#define BEEP_TIM_CH     TIM_Channel_1          // 改为通道1

// PWM参数：2kHz频率（周期500us），占空比50%（高电平250us）
// 计算依据：TIM1挂载APB2（时钟72MHz），PWM频率 = 72MHz / [(PSC+1)*(ARR+1)]
#define BEEP_TIM_PSC    8       // 预分频系数：72MHz/(8+1)=8MHz
#define BEEP_TIM_ARR    3999    // 自动重装载值：8MHz/(3999+1)=2000Hz（2kHz）
#define BEEP_TIM_CCR    2000    // 比较值（占空比50%：(2000)/(4000)=50%）

/**
 * @brief 无源蜂鸣器初始化（GPIO复用+TIM1 PWM配置）
 */
void BEEP_Init(void) {
    GPIO_InitTypeDef GPIO_InitStru;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStru;
    TIM_OCInitTypeDef TIM_OCInitStru;
    
    // 1. 使能时钟（TIM1 + GPIOA + 复用功能）
    RCC_APB2PeriphClockCmd(BEEP_TIM_RCC, ENABLE);    // 使能TIM1时钟（APB2）
    RCC_APB2PeriphClockCmd(BEEP_GPIO_RCC, ENABLE);   // 使能GPIOA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE); // 使能复用功能时钟

    // 2. 配置GPIO为复用推挽输出（PA8 -> TIM1_CH1）
    GPIO_InitStru.GPIO_Pin = BEEP_GPIO_PIN;
    GPIO_InitStru.GPIO_Mode = GPIO_Mode_AF_PP;       // 复用推挽（TIM1输出）
    GPIO_InitStru.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BEEP_GPIO_PORT, &GPIO_InitStru);

    // 3. 配置TIM1时基参数（生成2kHz计数周期）
    TIM_TimeBaseStru.TIM_Period = BEEP_TIM_ARR;          // 自动重装载值
    TIM_TimeBaseStru.TIM_Prescaler = BEEP_TIM_PSC;       // 预分频系数
    TIM_TimeBaseStru.TIM_ClockDivision = TIM_CKD_DIV1;   // 时钟分频：不分频
    TIM_TimeBaseStru.TIM_CounterMode = TIM_CounterMode_Up; // 向上计数
    TIM_TimeBaseInit(BEEP_TIM, &TIM_TimeBaseStru);

    // 4. 配置TIM1 PWM输出模式（通道1）
    TIM_OCInitStru.TIM_OCMode = TIM_OCMode_PWM1;         // PWM模式1（CNT<CCR时输出高电平）
    TIM_OCInitStru.TIM_OutputState = TIM_OutputState_Enable; // 使能输出
    TIM_OCInitStru.TIM_OCPolarity = TIM_OCPolarity_High; // 输出极性：高电平有效
    TIM_OCInitStru.TIM_Pulse = BEEP_TIM_CCR;             // 比较值（占空比50%）
    TIM_OC1Init(BEEP_TIM, &TIM_OCInitStru);              // 初始化通道1（原通道2）

    // 5. 使能TIM1预装载和计数器（默认关闭PWM输出，避免初始发声）
    TIM_OC1PreloadConfig(BEEP_TIM, TIM_OCPreload_Enable); // 使能CCR1预装载（原CCR2）
    TIM_ARRPreloadConfig(BEEP_TIM, ENABLE);               // 使能ARR预装载
    TIM_CtrlPWMOutputs(BEEP_TIM, ENABLE);                // 高级定时器需使能主输出
	TIM_ClearFlag(BEEP_TIM, TIM_FLAG_Update);
	TIM_ClearITPendingBit(BEEP_TIM, TIM_FLAG_Update);
//    TIM_Cmd(BEEP_TIM, DISABLE);                           // 初始关闭定时器（不发声）
	TIM_Cmd(BEEP_TIM, ENABLE); // 强制开启定时器，输出PWM
}

/**
 * @brief 无源蜂鸣器报警控制（根据系统状态切换发声模式）
 * @note 发声逻辑：
 * - SYS_NORMAL：关闭PWM（不发声）
 * - SYS_ENV_ALARM：短鸣（200ms响/200ms停，借OLED 200ms周期标志）
 * - SYS_INTRUDE_ALARM：长鸣（持续使能PWM）
 */
static uint8_t beep_cnt = 0; // 短鸣周期计数器（200ms递增）
void BEEP_Alarm_Control(void) {
    switch (sys_data.alarm_state) {
        case SYS_NORMAL:  // 正常：关闭PWM
            TIM_Cmd(BEEP_TIM, DISABLE);
            beep_cnt = 0; // 重置计数器
            break;
            
        case SYS_ENV_ALARM:  // 环境报警：短鸣（200ms响/200ms停）
            if (sys_data.flag_oled) { // 借OLED 200ms刷新标志（避免额外定时器）
                beep_cnt++;
                if (beep_cnt % 2 == 1) { // 奇数次：开启PWM（响）
                    TIM_Cmd(BEEP_TIM, ENABLE);
                } else { // 偶数次：关闭PWM（停）
                    TIM_Cmd(BEEP_TIM, DISABLE);
                }
            }
            break;
            
        case SYS_INTRUDE_ALARM:  // 入侵报警：长鸣（持续PWM）
            TIM_Cmd(BEEP_TIM, ENABLE);
            beep_cnt = 0; // 重置计数器
            break;
            
        default:  // 异常：关闭PWM
            TIM_Cmd(BEEP_TIM, DISABLE);
            beep_cnt = 0;
            break;
    }
}

/**
 * @brief 远程消警控制（强制关闭蜂鸣器）
 */
void BEEP_Stop(void) {
    TIM_Cmd(BEEP_TIM, DISABLE);
    beep_cnt = 0;
}
