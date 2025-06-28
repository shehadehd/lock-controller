#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include <string.h>
 
#include "common.h"
#include "main.h"
#include "communication.h"
#include "peripheral.h"
 
#define IDLE_DURATION_MS        10000
 
extern suSystemData systemData;
 
static void Start_Up_On_Entry(void* pContext);
static uint32_t Start_Up_On_Execute(void* pContext);
static void Wait_For_Dock_On_Entry(void* pContext);
static uint32_t Wait_For_Dock_On_Execute(void* pContext);
static void Locking_On_Entry(void* pContext);
static uint32_t Locking_On_Execute(void* pContext);
static void Unlocking_On_Entry(void* pContext);
static uint32_t Unlocking_On_Execute(void* pContext);
static void Locked_On_Entry(void* pContext);
static uint32_t Locked_On_Execute(void* pContext);
static void Faulted_On_Entry(void* pContext);
static uint32_t Faulted_On_Execute(void* pContext);
 
static suStateMachineClass stateMachine[SYSTEM_STATE_END] =
{
    [SYSTEM_STATE_START_UP] =        {.sName = "START_UP",       .on_Entry = Start_Up_On_Entry,      .on_Exit = NULL,    .on_Execute = Start_Up_On_Execute       },
    [SYSTEM_STATE_WAIT_FOR_DOCK] =   {.sName = "WAIT_FOR_DOCK",  .on_Entry = Wait_For_Dock_On_Entry, .on_Exit = NULL,    .on_Execute = Wait_For_Dock_On_Execute  },
    [SYSTEM_STATE_LOCKING] =         {.sName = "LOCKING",        .on_Entry = Locking_On_Entry,       .on_Exit = NULL,    .on_Execute = Locking_On_Execute        },
    [SYSTEM_STATE_UNLOCKING] =       {.sName = "UNLOCKING",      .on_Entry = Unlocking_On_Entry,     .on_Exit = NULL,    .on_Execute = Unlocking_On_Execute      },
    [SYSTEM_STATE_LOCKED] =          {.sName = "LOCKED",         .on_Entry = Locked_On_Entry,        .on_Exit = NULL,    .on_Execute = Locked_On_Execute         },
    [SYSTEM_STATE_FAULTED] =         {.sName = "FAULTED",        .on_Entry = Faulted_On_Entry,       .on_Exit = NULL,    .on_Execute = Faulted_On_Execute        },
};
 
static suStateMachineContext  stateMachineContext;
extern suSystemData systemData;
 
 
static void run_state_machine(suStateMachineContext  StateMachineContext)
{
    static uint32_t nextState = SYSTEM_STATE_START;
   
    if (nextState != stateMachineContext.currentState)
    {
        if( nextState >= SYSTEM_STATE_END)
        {
            nextState = SYSTEM_STATE_FAULTED;
        }
       
        network_send((uint8_t*)StateMachineContext.states[nextState].sName,
                    strlen(StateMachineContext.states[nextState].sName),
                    SYSTEM_STATE_REPORT);
                   
        if (StateMachineContext.states[stateMachineContext.currentState].on_Exit != NULL)
        {
            StateMachineContext.states[StateMachineContext.currentState].on_Exit(StateMachineContext.pContext);
        }
 
        StateMachineContext.uStateStartTime = xTaskGetTickCount();
        StateMachineContext.uStateElapsedTime = 0;
        StateMachineContext.currentState = nextState;
 
        if (StateMachineContext.states[StateMachineContext.currentState].on_Entry != NULL)
        {
            StateMachineContext.states[StateMachineContext.currentState].on_Entry(StateMachineContext.pContext);
        }
    }
    else
    {
        StateMachineContext.uStateElapsedTime = (xTaskGetTickCount() - StateMachineContext.uStateStartTime);
       
        if (StateMachineContext.states[StateMachineContext.currentState].on_Execute != NULL)
        {
            nextState = StateMachineContext.states[StateMachineContext.currentState].on_Execute(StateMachineContext.pContext);
        }
    }
}
 
static void Start_Up_On_Entry(void* pContext)
{
    peripheral_unlock_clamp();
}
 
static uint32_t Start_Up_On_Execute(void* pContext)
{
    uint32_t nextState = SYSTEM_STATE_START_UP;
   
    nextState = SYSTEM_STATE_WAIT_FOR_DOCK;
   
    return nextState;
}
 
static void Wait_For_Dock_On_Entry(void* pContext)
{
    peripheral_stop_clamp();
}
 
static uint32_t Wait_For_Dock_On_Execute(void* pContext)
{
    uint32_t nextState = SYSTEM_STATE_WAIT_FOR_DOCK;
   
    if (systemData.bDockingRequested)
    {
        if(systemData.AggregatePeripheralData.DockStatus == DOCK_STATUS_DOCKED)
        {
            // docking detected, proceed to locking phase
            nextState = SYSTEM_STATE_LOCKING;
        }
    }
   
    return nextState;
}
 
static void Locking_On_Entry(void* pContext)
{
    peripheral_lock_clamp();
}
 
static uint32_t Locking_On_Execute(void* pContext)
{
    uint32_t nextState = SYSTEM_STATE_LOCKING;
   
    if (systemData.bDockingRequested)
    {
        if (systemData.AggregatePeripheralData.DockStatus == DOCK_STATUS_DOCKED)
        {
            if (systemData.AggregatePeripheralData.LockStatus == LOCK_STATUS_LOCKED)
            {
                // detected locking complete. stop driving actuator
                nextState = SYSTEM_STATE_LOCKED;
            }
        }
        else
        {
            // detected undocking condition, unlock clamp to try again
            nextState = SYSTEM_STATE_UNLOCKING;
        }
    }
    else
    {
        // detected docking request is no longer active. Begin unlocking to allow for undocking
        nextState = SYSTEM_STATE_UNLOCKING;
    }
 
    return nextState;
}
 
static void Unlocking_On_Entry(void* pContext)
{
    peripheral_unlock_clamp();
}
 
static uint32_t Unlocking_On_Execute(void* pContext)
{
    uint32_t nextState = SYSTEM_STATE_LOCKING;
   
    if (systemData.AggregatePeripheralData.LockStatus == LOCK_STATUS_UNLOCKED)
    {
        // clamp fully unlocked, return to idle to proceed with next request
        nextState = SYSTEM_STATE_WAIT_FOR_DOCK;
    }
 
    return nextState;
}
 
static void Locked_On_Entry(void* pContext)
{
    peripheral_stop_clamp();
}
 
static uint32_t Locked_On_Execute(void* pContext)
{
    uint32_t nextState = SYSTEM_STATE_LOCKING;
   
    if (!systemData.bDockingRequested)
    {
        // detected successful lock. Wait here until undocking is desired
        nextState = SYSTEM_STATE_UNLOCKING;
    }
 
    return nextState;
}
 
static void Faulted_On_Entry(void* pContext)
{
    peripheral_unlock_clamp();
}
 
static uint32_t Faulted_On_Execute(void* pContext)
{
    uint32_t nextState = SYSTEM_STATE_FAULTED;
   
    nextState = SYSTEM_STATE_UNLOCKING;
 
    return nextState;
}
 
void state_machine_task(void *pvParameter)
{
    while(1)
    {
        vTaskDelay(10);
        run_state_machine(stateMachineContext);
    }
}
 
void state_machine_init()
{
    stateMachineContext.states = stateMachine;
    stateMachineContext.currentState = SYSTEM_STATE_START_UP;
 
    xTaskCreate(state_machine_task, "state_machine_task", 2048, NULL, 4, NULL);
}