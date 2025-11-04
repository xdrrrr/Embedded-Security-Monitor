#include "sys_data.h"
#include "alarm.h"
//#include "printf.h"

// 静态变量：新增计时变量，控制计数超时
static uint8_t intrude_conti_count = 0;  // 连续光照突变计数
static uint8_t intrude_lock = 0;         // 入侵锁定标志（1=锁定，0=正常检测）

static uint8_t intrude_timer = 0;        // 计数超时计时器（单位：100ms，对应Alarm_Check周期）
#define INTRUDE_COUNT_TIMEOUT 10         // 计数超时时间（10*100ms=1秒，可调整）

void Alarm_Check(void)
{
    int16_t light_diff;
    uint8_t is_env_alarm = 0;

    // -------------------------- 高优先级：入侵锁定判断 --------------------------
    if (intrude_lock == 1)
    {
        sys_data.alarm_state = SYS_INTRUDE_ALARM;
        return;
    }

    // -------------------------- 1. 光照突变检测（带超时计数） --------------------------
    light_diff = (int16_t)sys_data.prev_light - (int16_t)sys_data.light;
    light_diff = light_diff > 0 ? light_diff : -light_diff;
//	printf("%d %d\r\n",light_diff,sys_data.light_th);
    // 情况1：检测到光照突变（差值超阈值）
    if (light_diff > sys_data.light_th)
    {
//		printf("%d %d.................\r\n",light_diff,sys_data.light_th);
        intrude_conti_count++;
        intrude_timer = INTRUDE_COUNT_TIMEOUT;  // 重置计时器（确保有1秒时间等第2次突变）
        
        // 连续2次突变，触发入侵并锁定
        if (intrude_conti_count >= 2)
        {
            intrude_lock = 1;
            sys_data.alarm_state = SYS_INTRUDE_ALARM;
            intrude_conti_count = 0;  // 清零计数，避免重复触发
            intrude_timer = 0;        // 清零计时器
//            printf("INTRUDE ...\r\n");
            return;
        }
    }
    // 情况2：未检测到光照突变（差值≤阈值）
    else
    {
        // 若已有1次计数，启动计时器倒计时
        if (intrude_conti_count == 1)
        {
            if (intrude_timer > 0)
            {
                intrude_timer--;  // 每100ms减1，直到0
            }
            else
            {
                // 计时器归0（1秒内未检测到第2次），清零计数
                intrude_conti_count = 0;
                intrude_timer = 0;
//                printf("count = 0\r\n");
            }
        }
        // 若计数为0，无需处理（计时器也为0）
        else
        {
            intrude_conti_count = 0;
            intrude_timer = 0;
        }
    }

    // -------------------------- 2. 环境报警检测（逻辑不变） --------------------------
    if ((sys_data.temp > sys_data.temp_max 
     || sys_data.temp < sys_data.temp_min 
     || sys_data.hum > sys_data.hum_max) && sys_data.hum != 0)
    {
        is_env_alarm = 1;
        sys_data.alarm_state = SYS_ENV_ALARM;
    }
    else
    {
        sys_data.alarm_state = SYS_NORMAL;
    }
}

// 远程消警函数（逻辑不变）
void Alarm_RemoteOff(void)
{
    intrude_lock = 0;
    sys_data.alarm_state = SYS_NORMAL;
    intrude_conti_count = 0;  // 额外清零计数，避免消警后残留计数
    intrude_timer = 0;        // 额外清零计时器
//    printf("SYS_NORMAL\r\n");
}