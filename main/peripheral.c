#include "common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include <string.h>
 
#include "communication.h"
#include "peripheral.h"
 
#define DRIVE_DEADTIME_MS               1
#define PERIPHERAL_START_UP_TIMEOUT_MS  1000
 
 
#define GET_DOCKED_STATUS()     (gpio_get_level(GPIO_DOCKED_STATUS) == false)
#define GET_LOCK_STATUS()       (gpio_get_level(GPIO_LOCK_STATUS) == false)
 
#define LOCK_CLAMP()            {                                                   \
                                    gpio_set_level(GPIO_UNLOCK_EXTERNAL, false);    \
                                    vTaskDelay(DRIVE_DEADTIME_MS); /*1ms deadtime*/ \
                                    gpio_set_level(GPIO_LOCK_EXTERNAL, true);       \
                                }
 
#define UNLOCK_CLAMP()          {                                                   \
                                    gpio_set_level(GPIO_LOCK_EXTERNAL, false);      \
                                    vTaskDelay(DRIVE_DEADTIME_MS); /*1ms deadtime*/ \
                                    gpio_set_level(GPIO_UNLOCK_EXTERNAL, true);     \
                                }
 
#define STOP_CLAMP()            {                                                   \
                                    gpio_set_level(GPIO_LOCK_EXTERNAL, false);      \
                                    gpio_set_level(GPIO_UNLOCK_EXTERNAL, false);    \
                                }
 
static uint32_t Start_Up_On_Execute(void);
static uint32_t Unknown_On_Execute(void);
static uint32_t Unlocked_On_Execute(void);
static uint32_t Locked_On_Execute(void);
 
static suStateMachineClass stateMachine[LOCK_STATE_END] =
{
    [LOCK_STATE_START_UP] = {.sName = "LOCK_STATE_START_UP",    .on_Entry = NULL,   .on_Exit = NULL,    .on_Execute = Start_Up_On_Execute   },
    [LOCK_STATE_UNKNOWN] =  {.sName = "LOCK_STATE_UNKNOWN",     .on_Entry = NULL,   .on_Exit = NULL,    .on_Execute = Unknown_On_Execute    },
    [LOCK_STATE_UNLOCKED] = {.sName = "LOCK_STATE_UNLOCKED",    .on_Entry = NULL,   .on_Exit = NULL,    .on_Execute = Unlocked_On_Execute   },
    [LOCK_STATE_LOCKED] =   {.sName = "LOCK_STATE_LOCKED",      .on_Entry = NULL,   .on_Exit = NULL,    .on_Execute = Locked_On_Execute     },
};
 
static suStateMachineData  stateMachineData;
 
 
extern suSystemData        systemData;
 
 
static void run_state_machine()
{
    static uint32_t nextState = SYSTEM_STATE_START;
   
    if (nextState != stateMachineData.currentState)
    {
        if( nextState >= SYSTEM_STATE_END)
        {
            nextState = SYSTEM_STATE_FAULTED;
        }
       
        network_send((uint8_t*)stateMachineData.states[nextState].sName,
                    strlen(stateMachineData.states[nextState].sName),
                    LOCK_STATE_REPORT);
                   
        if (stateMachineData.states[stateMachineData.currentState].on_Exit != NULL)
        {
            stateMachineData.states[stateMachineData.currentState].on_Exit();
        }
 
        stateMachineData.uStateStartTime = xTaskGetTickCount();
        stateMachineData.uStateElapsedTime = 0;
        stateMachineData.currentState = nextState;
 
        if (stateMachineData.states[stateMachineData.currentState].on_Entry != NULL)
        {
            stateMachineData.states[stateMachineData.currentState].on_Entry();
        }
    }
    else
    {
        stateMachineData.uStateElapsedTime = (xTaskGetTickCount() - stateMachineData.uStateStartTime);
       
        if (stateMachineData.states[stateMachineData.currentState].on_Execute != NULL)
        {
            nextState = stateMachineData.states[stateMachineData.currentState].on_Execute();
        }
    }
}
 
static uint32_t Start_Up_On_Execute(void)
{
    uint32_t nextState = LOCK_STATE_START_UP;
 
    if(GET_LOCK_STATUS() == ASSERTED)
    {
        if (systemData.PeripheralData.MotorStatus == MOTOR_STATUS_UNLOCKING)
        {
            if (stateMachineData.uStateElapsedTime > PERIPHERAL_START_UP_TIMEOUT_MS)
            {
                systemData.PeripheralData.MotorStatus = MOTOR_STATUS_UNLOCKED;
                systemData.PeripheralData.LockStatus = LOCK_STATUS_UNLOCKED;
                nextState = LOCK_STATE_UNLOCKED;
            }
        }
    }
 
    return nextState;
}
 
static uint32_t Unknown_On_Execute(void)
{
    uint32_t nextState = LOCK_STATE_UNKNOWN;
   
    if(GET_LOCK_STATUS() == ASSERTED)
    {
        if (systemData.PeripheralData.MotorStatus == MOTOR_STATUS_LOCKING)
        {
            systemData.PeripheralData.MotorStatus = MOTOR_STATUS_LOCKED;
            systemData.PeripheralData.LockStatus = LOCK_STATUS_LOCKED;
            nextState = LOCK_STATE_LOCKED;
        }
        else if (systemData.PeripheralData.MotorStatus == MOTOR_STATUS_UNLOCKING)
        {
            systemData.PeripheralData.MotorStatus = MOTOR_STATUS_UNLOCKED;
            systemData.PeripheralData.LockStatus = LOCK_STATUS_UNLOCKED;
            nextState = LOCK_STATE_UNLOCKED;
        }
    }
 
    return nextState;
}
 
static uint32_t Unlocked_On_Execute(void)
{
    uint32_t nextState = LOCK_STATE_UNLOCKED;
   
    if(GET_LOCK_STATUS() != ASSERTED)
    {
        systemData.PeripheralData.LockStatus = LOCK_STATUS_UNKNOWN;
        nextState = LOCK_STATE_UNKNOWN;
    }
 
    return nextState;
}
 
static uint32_t Locked_On_Execute(void)
{
    uint32_t nextState = LOCK_STATE_LOCKED;
   
    if(GET_LOCK_STATUS() != ASSERTED)
    {
        systemData.PeripheralData.LockStatus = LOCK_STATUS_UNKNOWN;
        nextState = LOCK_STATE_UNKNOWN;
    }
 
    return nextState;
}
 
void run_peripheral(void)
{
    systemData.PeripheralData.DockStatus = (GET_DOCKED_STATUS() == ASSERTED) ? DOCK_STATUS_DOCKED : DOCK_STATUS_UNDOCKED;
 
    run_state_machine();
}
 
void peripheral_task(void *pvParameter)
{
    while(1)
    {
        vTaskDelay(1000);
        run_peripheral();
        network_send((uint8_t*)&systemData, sizeof(systemData), SYSTEM_REPORT);
        // network_send((uint8_t*)&systemData.PeripheralData.DockedState, sizeof(systemData.PeripheralData.DockedState), DOCK_COMMAND);
    }
}
 
void peripheral_init(void)
{
    gpio_config_t gpio_conf = {0};
 
    stateMachineData.states = stateMachine;
    stateMachineData.currentState = LOCK_STATE_START_UP;
 
    systemData.PeripheralData.DockStatus = DOCK_STATUS_UNKNOWN;
    systemData.PeripheralData.LockStatus = LOCK_STATUS_UNKNOWN;
    systemData.PeripheralData.MotorStatus = MOTOR_STATUS_STOPPED;
 
    gpio_conf.intr_type = GPIO_INTR_DISABLE;    // Disable interrupt for now
    gpio_conf.mode = GPIO_MODE_INPUT;  // Set as input mode
    gpio_conf.pin_bit_mask = (1ULL << GPIO_DOCKED_STATUS | 1ULL << GPIO_LOCK_STATUS);
    gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;  // Enable internal pull-up resistor
    gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // Disable pull-down
    gpio_config(&gpio_conf);
 
    gpio_conf.intr_type = GPIO_INTR_DISABLE;    // Disable interrupt for now
    gpio_conf.mode = GPIO_MODE_OUTPUT;  // Set as input mode
    gpio_conf.pin_bit_mask = (1ULL << GPIO_LOCK_EXTERNAL | 1ULL << GPIO_UNLOCK_EXTERNAL);
    gpio_conf.pull_up_en = GPIO_PULLUP_DISABLE;  // Disable internal pull-up resistor
    gpio_conf.pull_down_en = GPIO_PULLDOWN_ENABLE; // Enable internal pull-down
    gpio_config(&gpio_conf);  
   
    xTaskCreate(peripheral_task, "state_machine_task", 2048, NULL, 4, NULL);
}
 
void peripheral_lock_clamp(void)
{
    systemData.PeripheralData.MotorStatus = MOTOR_STATUS_LOCKING;
    LOCK_CLAMP();
}
 
void peripheral_unlock_clamp(void)
{
    systemData.PeripheralData.MotorStatus = MOTOR_STATUS_UNLOCKING;
    UNLOCK_CLAMP();
}
 
void peripheral_stop_clamp(void)
{
    systemData.PeripheralData.MotorStatus = MOTOR_STATUS_STOPPED;
    STOP_CLAMP();
}