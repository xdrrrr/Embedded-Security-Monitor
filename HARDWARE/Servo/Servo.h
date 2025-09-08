#ifndef SERVO_H
#define SERVO_H

#include "stm32f10x.h"

void Servo_init(void);

void Servo_SetAngle(uint8_t angle);
void Servo_Link_Alarm(void);

#endif