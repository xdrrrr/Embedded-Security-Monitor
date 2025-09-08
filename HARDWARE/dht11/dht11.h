#ifndef DHT11_H
#define DHT11_H

#include "stm32f10x.h"  // Device header
#include <stdbool.h>

void dht11_init(void);
bool DHT11_ReadData(volatile uint8_t *temp,volatile uint8_t *hum);

#endif
