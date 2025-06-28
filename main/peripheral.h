#ifndef PERIPHERAL_H
#define PERIPHERAL_H
 
#include "driver/gpio.h"
 
#include "common.h"
 
#define GPIO_LOCK_1_            GPIO_NUM_20
#define GPIO_LOCK_2_            GPIO_NUM_20
#define GPIO_LOCK_3_            GPIO_NUM_20
#define GPIO_LOCK_4_            GPIO_NUM_20
#define GPIO_LOCK_5_            GPIO_NUM_20
#define GPIO_LOCK_6_            GPIO_NUM_20
#define GPIO_LOCK_7_            GPIO_NUM_20
#define GPIO_LOCK_8_            GPIO_NUM_20
#define GPIO_LOCK_9_            GPIO_NUM_20
#define GPIO_LOCK_10_           GPIO_NUM_20

#define GPIO_DOCK_1_            GPIO_NUM_19
#define GPIO_DOCK_2_            GPIO_NUM_19
#define GPIO_DOCK_3_            GPIO_NUM_19
#define GPIO_DOCK_4_            GPIO_NUM_19
#define GPIO_DOCK_5_            GPIO_NUM_19
#define GPIO_DOCK_6_            GPIO_NUM_19
#define GPIO_DOCK_7_            GPIO_NUM_19
#define GPIO_DOCK_8_            GPIO_NUM_19
#define GPIO_DOCK_9_            GPIO_NUM_19
#define GPIO_DOCK_10_           GPIO_NUM_19

#define GPIO_MOTOR_LOCK_1_      GPIO_NUM_18
#define GPIO_MOTOR_LOCK_2_      GPIO_NUM_18
#define GPIO_MOTOR_LOCK_3_      GPIO_NUM_18
#define GPIO_MOTOR_LOCK_4_      GPIO_NUM_18
#define GPIO_MOTOR_LOCK_5_      GPIO_NUM_18
#define GPIO_MOTOR_LOCK_6_      GPIO_NUM_18
#define GPIO_MOTOR_LOCK_7_      GPIO_NUM_18
#define GPIO_MOTOR_LOCK_8_      GPIO_NUM_18
#define GPIO_MOTOR_LOCK_9_      GPIO_NUM_18
#define GPIO_MOTOR_LOCK_10_     GPIO_NUM_18

#define GPIO_MOTOR_UNLOCK_1_    GPIO_NUM_17
#define GPIO_MOTOR_UNLOCK_2_    GPIO_NUM_17
#define GPIO_MOTOR_UNLOCK_3_    GPIO_NUM_17
#define GPIO_MOTOR_UNLOCK_4_    GPIO_NUM_17
#define GPIO_MOTOR_UNLOCK_5_    GPIO_NUM_17
#define GPIO_MOTOR_UNLOCK_6_    GPIO_NUM_17
#define GPIO_MOTOR_UNLOCK_7_    GPIO_NUM_17
#define GPIO_MOTOR_UNLOCK_8_    GPIO_NUM_17
#define GPIO_MOTOR_UNLOCK_9_    GPIO_NUM_17
#define GPIO_MOTOR_UNLOCK_10_   GPIO_NUM_17

// list of all endpoint lock peripherals
gpio_num_t aLockPin[STATUS_PIN_COUNT] = 
{
    [STATUS_BOTTOM_LEFT]    = GPIO_LOCK_1_,
    [STATUS_BOTTOM_RIGHT]   = GPIO_LOCK_2_,
    [STATUS_MIDDLE_LEFT]    = GPIO_LOCK_3_,
    [STATUS_MIDDLE_RIGHT]   = GPIO_LOCK_4_,
    [STATUS_MIDDLE_CENTER]  = GPIO_LOCK_5_,
    [STATUS_UPPER_LEFT]     = GPIO_LOCK_6_,
    [STATUS_UPPER_RIGHT]    = GPIO_LOCK_7_,
    [STATUS_TOP_LEFT]       = GPIO_LOCK_8_,
    [STATUS_TOP_RIGHT]      = GPIO_LOCK_9_,
    [STATUS_TOP_CENTER]     = GPIO_LOCK_10_,
};
 
// list of all endpoint dock peripherals
gpio_num_t aDockPin[STATUS_PIN_COUNT] = 
{
    [STATUS_BOTTOM_LEFT]    = GPIO_DOCK_1_,
    [STATUS_BOTTOM_RIGHT]   = GPIO_DOCK_2_,
    [STATUS_MIDDLE_LEFT]    = GPIO_DOCK_3_,
    [STATUS_MIDDLE_RIGHT]   = GPIO_DOCK_4_,
    [STATUS_MIDDLE_CENTER]  = GPIO_DOCK_5_,
    [STATUS_UPPER_LEFT]     = GPIO_DOCK_6_,
    [STATUS_UPPER_RIGHT]    = GPIO_DOCK_7_,
    [STATUS_TOP_LEFT]       = GPIO_DOCK_8_,
    [STATUS_TOP_RIGHT]      = GPIO_DOCK_9_,
    [STATUS_TOP_CENTER]     = GPIO_DOCK_10_,
};
 
// list of all endpoint dock peripherals
gpio_num_t aMotorLockPin[STATUS_PIN_COUNT] = 
{
    [STATUS_BOTTOM_LEFT]    = GPIO_MOTOR_LOCK_1_,
    [STATUS_BOTTOM_RIGHT]   = GPIO_MOTOR_LOCK_2_,
    [STATUS_MIDDLE_LEFT]    = GPIO_MOTOR_LOCK_3_,
    [STATUS_MIDDLE_RIGHT]   = GPIO_MOTOR_LOCK_4_,
    [STATUS_MIDDLE_CENTER]  = GPIO_MOTOR_LOCK_5_,
    [STATUS_UPPER_LEFT]     = GPIO_MOTOR_LOCK_6_,
    [STATUS_UPPER_RIGHT]    = GPIO_MOTOR_LOCK_7_,
    [STATUS_TOP_LEFT]       = GPIO_MOTOR_LOCK_8_,
    [STATUS_TOP_RIGHT]      = GPIO_MOTOR_LOCK_9_,
    [STATUS_TOP_CENTER]     = GPIO_MOTOR_LOCK_10_,
};
 
// list of all endpoint dock peripherals
gpio_num_t aMotorUnlockPin[STATUS_PIN_COUNT] = 
{
    [STATUS_BOTTOM_LEFT]    = GPIO_MOTOR_UNLOCK_1_,
    [STATUS_BOTTOM_RIGHT]   = GPIO_MOTOR_UNLOCK_2_,
    [STATUS_MIDDLE_LEFT]    = GPIO_MOTOR_UNLOCK_3_,
    [STATUS_MIDDLE_RIGHT]   = GPIO_MOTOR_UNLOCK_4_,
    [STATUS_MIDDLE_CENTER]  = GPIO_MOTOR_UNLOCK_5_,
    [STATUS_UPPER_LEFT]     = GPIO_MOTOR_UNLOCK_6_,
    [STATUS_UPPER_RIGHT]    = GPIO_MOTOR_UNLOCK_7_,
    [STATUS_TOP_LEFT]       = GPIO_MOTOR_UNLOCK_8_,
    [STATUS_TOP_RIGHT]      = GPIO_MOTOR_UNLOCK_9_,
    [STATUS_TOP_CENTER]     = GPIO_MOTOR_UNLOCK_10_,
};

void peripheral_init(void);
void peripheral_lock_clamp(void);
void peripheral_unlock_clamp(void);
void peripheral_stop_clamp(void);
 
#endif