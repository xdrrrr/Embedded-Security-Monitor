// USER/sys_data.c
#include "sys_data.h"

// 全局变量初始化（所有值赋初始值，避免乱码）
volatile SystemData sys_data = {
    .temp = 0, .hum = 0, .light = 0, .prev_light = 0,
    .servo_angle = 90, // 舵机上电归中
    .alarm_state = SYS_NORMAL,
    .light_th = 350, .temp_max = 35, .temp_min = 5, .hum_max = 50,
    .flag_dht11 = 0, .flag_adc = 0, .flag_oled = 0, .flag_alarm = 0,
	.servo_manual_flag = 0
};