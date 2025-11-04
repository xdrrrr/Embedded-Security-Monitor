// USER/log.c
#include "printf.h"
#include <string.h>
#include "log.h"
#include "flash/w25qxx.h"  
#include "sys_data.h"
//#include "tim.h" // 用于获取时间戳（timer_100ms_cnt是TIM3的100ms计数器）


extern volatile SystemData sys_data;
extern uint32_t timer_100ms_cnt;  // 从timer3.c中引用（需在timer3.h中声明）

uint32_t log_current_addr = LOG_START_ADDR; // 当前日志存储地址

// 1. 存储单条报警日志（仅在报警发生时调用）
void Log_Save(AlarmLog *log) {
    // 1. 检查扇区擦除（逻辑不变，W25Q32扇区同样4KB）
    if ((log_current_addr % LOG_SECTOR_SIZE == 0) && (log_current_addr != LOG_START_ADDR)) {
        W25QXX_Erase_Sector(log_current_addr); // 仅替换为W25Q32的擦除函数
    }
	
    // 2. 准备日志数据
    uint8_t log_buf[LOG_SIZE];
    memcpy(log_buf, log, LOG_SIZE);

    // 3. 计算当前页剩余字节数，判断是否跨页
    uint16_t remaining = 256 - (log_current_addr % 256);
    if (remaining >= LOG_SIZE) {
        // 剩余空间足够，单次写入
        W25QXX_Write_Page( log_buf,log_current_addr, LOG_SIZE);
    } else {
        // 剩余空间不足，分两次写入
        W25QXX_Write_Page( log_buf,log_current_addr, remaining); // 当前页剩余部分
        W25QXX_Write_Page( log_buf + remaining,log_current_addr + remaining, LOG_SIZE - remaining); // 下一页
    }

    // 4. 更新地址（循环覆盖逻辑不变）
    log_current_addr += LOG_SIZE;
    if (log_current_addr >= LOG_START_ADDR + LOG_MAX_CNT * LOG_SIZE) {
        log_current_addr = LOG_START_ADDR;
    }
}

// 2. 读取N条日志（从指定地址开始，返回实际读取条数）
uint16_t Log_Read(uint32_t start_addr, AlarmLog *logs, uint16_t read_cnt) {
    uint16_t actual_cnt = 0;
    for (uint16_t i = 0; i < read_cnt; i++) {
        uint32_t addr = start_addr + i * LOG_SIZE;
        // 检查地址是否超出日志范围
        if (addr >= LOG_START_ADDR + LOG_MAX_CNT * LOG_SIZE) {
            break;
        }
        // 读取单条日志
        W25QXX_Read( (uint8_t*)&logs[i],addr, LOG_SIZE);
        // 过滤空日志（擦除后是0xFF，日志时间戳不会是0xFFFFFFFF）
        if (logs[i].timestamp != 0xFFFFFFFF) {
            actual_cnt++;
        }
    }
    return actual_cnt;
}

    
// 3. 触发日志存储（在报警状态变化时调用）
void Log_TriggerSave(void) {

    // 仅当报警状态从“正常”变为“报警”时存储（避免重复存储）
    if ((sys_data.alarm_state != SYS_NORMAL) && (prev_alarm_state == SYS_NORMAL)) {
//		printf("%d\r\n",sys_data.alarm_state);
        AlarmLog log;
        log.timestamp = timer_100ms_cnt / 10; // 100ms计数→秒（时间戳）
        log.alarm_type = sys_data.alarm_state;
        log.temp = sys_data.temp;
        log.hum = sys_data.hum;
        log.light = sys_data.light;
        Log_Save(&log);
		
//        printf("Log Saved! Timestamp: %d, Type: %d\r\n", log.timestamp, log.alarm_type);
    }
    prev_alarm_state = sys_data.alarm_state;
}