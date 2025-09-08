#ifndef ALARM_H
#define ALARM_H
#include "sys_data.h"

// 声明报警核心函数
void Alarm_Check(void);
// 声明远程消警函数（供串口模块调用）
void Alarm_RemoteOff(void);

#endif