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
 
typedef struct PERIPHERALCONTEXTTYPE    suPeripheralContext;

struct PERIPHERALCONTEXTTYPE
{
    eStatusPinIndex LockPinIndex;
    eStatusPinIndex DockPinIndex;
    gpio_num_t motorLockPin;
    gpio_num_t motorUnlockPin;
    gpio_num_t dockPin;
    gpio_num_t lockPin;
    suPeripheralData PeripheralData;
};


static uint32_t Start_Up_On_Execute(void* pContext);
static uint32_t Unknown_On_Execute(void* pContext);
static uint32_t Unlocked_On_Execute(void* pContext);
static uint32_t Locked_On_Execute(void* pContext);
 
// Framework that each endpoint uses for status evaluation
static suStateMachineClass individualStateMachine[LOCK_STATE_END] =
{
    [LOCK_STATE_START_UP] = {.sName = "LOCK_STATE_START_UP",    .on_Entry = NULL,   .on_Exit = NULL,    .on_Execute = Start_Up_On_Execute   },
    [LOCK_STATE_UNKNOWN] =  {.sName = "LOCK_STATE_UNKNOWN",     .on_Entry = NULL,   .on_Exit = NULL,    .on_Execute = Unknown_On_Execute    },
    [LOCK_STATE_UNLOCKED] = {.sName = "LOCK_STATE_UNLOCKED",    .on_Entry = NULL,   .on_Exit = NULL,    .on_Execute = Unlocked_On_Execute   },
    [LOCK_STATE_LOCKED] =   {.sName = "LOCK_STATE_LOCKED",      .on_Entry = NULL,   .on_Exit = NULL,    .on_Execute = Locked_On_Execute     },
};
 
// aggregation of all endpoint state machines for parsed referencing
static suStateMachineClass* aggregateStateMachine[STATUS_PIN_COUNT] =
{
    [STATUS_BOTTOM_LEFT] = &individualStateMachine[STATUS_BOTTOM_LEFT],
    [STATUS_BOTTOM_RIGHT] = &individualStateMachine[STATUS_BOTTOM_RIGHT],
    [STATUS_MIDDLE_LEFT] = &individualStateMachine[STATUS_MIDDLE_LEFT],
    [STATUS_MIDDLE_RIGHT] = &individualStateMachine[STATUS_MIDDLE_RIGHT],
    [STATUS_MIDDLE_CENTER] = &individualStateMachine[STATUS_MIDDLE_CENTER],
    [STATUS_UPPER_LEFT] = &individualStateMachine[STATUS_UPPER_LEFT],
    [STATUS_UPPER_RIGHT] = &individualStateMachine[STATUS_UPPER_RIGHT],
    [STATUS_TOP_LEFT] = &individualStateMachine[STATUS_TOP_LEFT],
    [STATUS_TOP_RIGHT] = &individualStateMachine[STATUS_TOP_RIGHT],
    [STATUS_TOP_CENTER] = &individualStateMachine[STATUS_TOP_CENTER],
};

// object to hold all endpoint state machine contexts
static suStateMachineContext  aIndividualStateMachineContext[STATUS_PIN_COUNT];
 
static suPeripheralContext aPeripheralContext[STATUS_PIN_COUNT];

extern suSystemData        systemData;
 
 
static void run_state_machine(suStateMachineContext StateMachineContext)
{
    uint32_t nextState = StateMachineContext.nextState;
   
    if (nextState != StateMachineContext.currentState)
    {
        if( nextState >= SYSTEM_STATE_END)
        {
            nextState = SYSTEM_STATE_FAULTED;
        }
       
        // network_send((uint8_t*)StateMachineContext.states[nextState].sName,
        //             strlen(StateMachineContext.states[nextState].sName),
        //             LOCK_STATE_REPORT);
                   
        if (StateMachineContext.states[StateMachineContext.currentState].on_Exit != NULL)
        {
            StateMachineContext.states[StateMachineContext.currentState].on_Exit(&StateMachineContext);
        }
 
        StateMachineContext.uStateStartTime = xTaskGetTickCount();
        StateMachineContext.uStateElapsedTime = 0;
        StateMachineContext.currentState = nextState;
 
        if (StateMachineContext.states[StateMachineContext.currentState].on_Entry != NULL)
        {
            StateMachineContext.states[StateMachineContext.currentState].on_Entry(&StateMachineContext);
        }
    }
    else
    {
        StateMachineContext.uStateElapsedTime = (xTaskGetTickCount() - StateMachineContext.uStateStartTime);
       
        if (StateMachineContext.states[StateMachineContext.currentState].on_Execute != NULL)
        {
            nextState = StateMachineContext.states[StateMachineContext.currentState].on_Execute(&StateMachineContext);
        }
    }
}
 
static uint32_t Start_Up_On_Execute(void* pContext)
{
    suStateMachineContext *pStateMachineContext = (suStateMachineContext*)pContext;
    suPeripheralContext *pPeripheralContext = (suPeripheralContext*)pStateMachineContext->pContext;
    uint32_t nextState = LOCK_STATE_START_UP;
 
    if(pPeripheralContext->dockPin == ASSERTED)
    {
        if (pPeripheralContext->PeripheralData.MotorStatus == MOTOR_STATUS_UNLOCKING)
        {
            if (pStateMachineContext->uStateElapsedTime > PERIPHERAL_START_UP_TIMEOUT_MS)
            {
                pPeripheralContext->PeripheralData.MotorStatus = MOTOR_STATUS_UNLOCKED;
                pPeripheralContext->PeripheralData.LockStatus = LOCK_STATUS_UNLOCKED;
                nextState = LOCK_STATE_UNLOCKED;
            }
        }
    }
 
    return nextState;
}
 
static uint32_t Unknown_On_Execute(void* pContext)
{
    suStateMachineContext *pStateMachineContext = (suStateMachineContext*)pContext;
    suPeripheralContext *pPeripheralContext = (suPeripheralContext*)pStateMachineContext->pContext;
    uint32_t nextState = LOCK_STATE_UNKNOWN;
   
    if(pPeripheralContext->lockPin == ASSERTED)
    {
        if (pPeripheralContext->PeripheralData.MotorStatus == MOTOR_STATUS_LOCKING)
        {
            pPeripheralContext->PeripheralData.MotorStatus = MOTOR_STATUS_LOCKED;
            pPeripheralContext->PeripheralData.LockStatus = LOCK_STATUS_LOCKED;
            nextState = LOCK_STATE_LOCKED;
        }
        else if (pPeripheralContext->PeripheralData.MotorStatus == MOTOR_STATUS_UNLOCKING)
        {
            pPeripheralContext->PeripheralData.MotorStatus = MOTOR_STATUS_UNLOCKED;
            pPeripheralContext->PeripheralData.LockStatus = LOCK_STATUS_UNLOCKED;
            nextState = LOCK_STATE_UNLOCKED;
        }
    }
 
    return nextState;
}
 
static uint32_t Unlocked_On_Execute(void* pContext)
{
    suStateMachineContext *pStateMachineContext = (suStateMachineContext*)pContext;
    suPeripheralContext *pPeripheralContext = (suPeripheralContext*)pStateMachineContext->pContext;
    uint32_t nextState = LOCK_STATE_UNLOCKED;
   
    if(pPeripheralContext->lockPin != ASSERTED)
    {
        pPeripheralContext->PeripheralData.LockStatus = LOCK_STATUS_UNKNOWN;
        nextState = LOCK_STATE_UNKNOWN;
    }
 
    return nextState;
}
 
static uint32_t Locked_On_Execute(void* pContext)
{
    suStateMachineContext *pStateMachineContext = (suStateMachineContext*)pContext;
    suPeripheralContext *pPeripheralContext = (suPeripheralContext*)pStateMachineContext->pContext;
    uint32_t nextState = LOCK_STATE_LOCKED;
   
    if(pPeripheralContext->lockPin != ASSERTED)
    {
        pPeripheralContext->PeripheralData.LockStatus = LOCK_STATUS_UNKNOWN;
        nextState = LOCK_STATE_UNKNOWN;
    }
 
    return nextState;
}

void update_peripheral(suPeripheralContext *pPeripheralContext, uint32_t peripheral)
{
    pPeripheralContext->dockPin = (gpio_get_level(aDockPin[peripheral]) == ASSERTED);
    pPeripheralContext->PeripheralData.DockStatus = (pPeripheralContext->dockPin == ASSERTED) ? DOCK_STATUS_DOCKED : DOCK_STATUS_UNDOCKED;

    pPeripheralContext->lockPin = (gpio_get_level(aLockPin[peripheral]) == ASSERTED);
}
 
void peripheral_task(void *pvParameter)
{
    while(1)
    {
        vTaskDelay(1000);
        for (int peripheral = 0; peripheral < STATUS_PIN_COUNT; peripheral++)
        {
            update_peripheral(aIndividualStateMachineContext[peripheral].pContext, peripheral);
            run_state_machine(aIndividualStateMachineContext[peripheral]);
        }
        network_send((uint8_t*)&systemData, sizeof(systemData), SYSTEM_REPORT);
        // network_send((uint8_t*)&systemData.PeripheralData.DockedState, sizeof(systemData.PeripheralData.DockedState), DOCK_COMMAND);
    }
}
 
void peripheral_init(void)
{
    gpio_config_t gpio_conf = {0};
 
    // initialize each endpoint state machine
    for (int i = 0; i < STATUS_PIN_COUNT; i++)
    {
        aPeripheralContext[i].LockPinIndex = aLockPin[i];
        aPeripheralContext[i].DockPinIndex = aDockPin[i];
        aPeripheralContext[i].motorLockPin = aMotorLockPin[i];
        aPeripheralContext[i].motorUnlockPin = aMotorUnlockPin[i];

        aIndividualStateMachineContext[i].states = aggregateStateMachine[i];
        aIndividualStateMachineContext[i].currentState = LOCK_STATE_START_UP;
        aIndividualStateMachineContext[i].nextState = LOCK_STATE_START_UP;
        
        aIndividualStateMachineContext[i].pContext = (void*)&aPeripheralContext[i];

        aPeripheralContext[i].PeripheralData.DockStatus = DOCK_STATUS_UNKNOWN;
        aPeripheralContext[i].PeripheralData.LockStatus = LOCK_STATUS_UNKNOWN;
        aPeripheralContext[i].PeripheralData.MotorStatus = MOTOR_STATUS_STOPPED;
    }

    systemData.AggregatePeripheralData.DockStatus = DOCK_STATUS_UNKNOWN;
    systemData.AggregatePeripheralData.LockStatus = LOCK_STATUS_UNKNOWN;
    systemData.AggregatePeripheralData.MotorStatus = MOTOR_STATUS_STOPPED;
 
    gpio_conf.intr_type = GPIO_INTR_DISABLE;    // Disable interrupt for now
    gpio_conf.mode = GPIO_MODE_INPUT;  // Set as input mode
    for (int i = 0; i < STATUS_PIN_COUNT; i++)
    {
        gpio_conf.pin_bit_mask |= 1ULL << aLockPin[i];
        gpio_conf.pin_bit_mask |= 1ULL << aDockPin[i];
    }
    gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;  // Enable internal pull-up resistor
    gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // Disable pull-down
    gpio_config(&gpio_conf);
 
    gpio_conf.intr_type = GPIO_INTR_DISABLE;    // Disable interrupt for now
    gpio_conf.mode = GPIO_MODE_OUTPUT;  // Set as input mode
    gpio_conf.pin_bit_mask = 0;
    for (int i = 0; i < STATUS_PIN_COUNT; i++)
    {
        gpio_conf.pin_bit_mask |= 1ULL << aMotorLockPin[i];
        gpio_conf.pin_bit_mask |= 1ULL << aMotorUnlockPin[i];
    }
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