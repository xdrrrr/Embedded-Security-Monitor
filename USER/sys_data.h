// USER/sys_data.h
#ifndef SYS_DATA_H
#define SYS_DATA_H
#include "stm32f10x.h"

// 1. 系统状态枚举（报警类型）
typedef enum {
    SYS_NORMAL = 0,    // 正常
    SYS_ENV_ALARM = 1, // 环境报警（温湿度超阈值）
    SYS_INTRUDE_ALARM=2// 入侵报警（光照突变）
} SysState;

// 2. 全局系统数据（所有模块共享）
typedef struct {
    // 环境采集数据
    uint8_t temp;       // 温度（℃，DHT11）
    uint8_t hum;        // 湿度（%，DHT11）
    uint16_t light;     // 当前光照（lux，ADC）
    uint16_t prev_light;// 上一次光照（突变检测用）
    // 控制数据
    uint8_t servo_angle;// 舵机角度（0-180°）
    SysState alarm_state;// 系统报警状态
    // 配置参数（可改）
    uint16_t light_th;  // 光照突变阈值（默认300lux）
    uint8_t temp_max;   // 温度上限（默认35℃）
    uint8_t temp_min;   // 温度下限（默认5℃）
    uint8_t hum_max;    // 湿度上限（默认80%）
    // 定时器标志位（控制频率）
    uint8_t flag_dht11; // 温湿度采集标志（1s一次）
    uint8_t flag_adc;   // 光照采集标志（100ms一次）
    uint8_t flag_oled;  // OLED刷新标志（200ms一次）
    uint8_t flag_alarm; // 报警判断标志（100ms一次）
	
	uint8_t servo_manual_flag; // 舵机手动控制标志：0=自动联动，1=手动控制
} SystemData;

// 3. 声明全局变量（在sys_data.c中定义，其他文件用extern调用）
extern volatile SystemData sys_data;


// USER/sys_data.h 中新增日志结构体
typedef struct {
    uint32_t timestamp;  // 时间戳（单位：秒，用定时器100ms计数/10）
    uint8_t alarm_type;  // 报警类型（1=环境报警，2=入侵报警）
    uint8_t temp;        // 报警时温度
    uint8_t hum;         // 报警时湿度
    uint16_t light;      // 报警时光照
} AlarmLog;//10个字节

// 日志配置（W25Q32共4MB=4096KB，每扇区4KB，每日志10字节，1扇区可存4096/10≈409条）
#define LOG_START_ADDR    0x000000    // 日志起始地址（第0扇区）
#define LOG_MAX_CNT       8192        // 最大日志数（约占用80KB，20个扇区，可根据需要调整）
#define LOG_SIZE          sizeof(AlarmLog) // 每条日志大小（10字节）
#define LOG_SECTOR_SIZE   4096        // 扇区大小（4KB，W25Q32与W25Q16相同）

#endif