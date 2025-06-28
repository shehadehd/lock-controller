#ifndef PERIPHERAL_H
#define PERIPHERAL_H
 
#include "driver/gpio.h"
 
#include "common.h"
 
#define GPIO_LOCK_STATUS        GPIO_NUM_20
#define GPIO_DOCKED_STATUS      GPIO_NUM_19
 
#define GPIO_LOCK_EXTERNAL      GPIO_NUM_18
#define GPIO_UNLOCK_EXTERNAL    GPIO_NUM_17
 
void peripheral_init(void);
void peripheral_lock_clamp(void);
void peripheral_unlock_clamp(void);
void peripheral_stop_clamp(void);
 
#endif