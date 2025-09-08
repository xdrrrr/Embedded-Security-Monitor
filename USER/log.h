// USER/log.h
#ifndef LOG_H
#define LOG_H
#include "stm32f10x.h"
#include "sys_data.h"


extern uint32_t log_current_addr;

extern uint8_t prev_alarm_state;
// º¯ÊıÉùÃ÷
void Log_Save(AlarmLog *log);
uint16_t Log_Read(uint32_t start_addr, AlarmLog *logs, uint16_t read_cnt);
void Log_TriggerSave(void);

#endif