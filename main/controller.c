 ///////// INCLUDES /////////////////////////////////////////////////////////////////////////////////
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include <string.h>
 
#include "common.h"
#include "communication.h"
#include "peripheral.h"
 
///////// POST-INCLUDE PRE-PROCESSOR DIRECTIVES ////////////////////////////////////////////////////





///////// MNEMONICS ////////////////////////////////////////////////////////////////////////////////


///////// DEFINES //////////////////////////////////////////////////////////////////////////////

#define IDLE_DURATION_MS        10000
 
///////// ENUMERATIONS /////////////////////////////////////////////////////////////////////////

///////// FORWARD STRUCT DECLARATIONS //////////////////////////////////////////////////////////////

struct SYSTEMCONTEXTTYPE;

///////// TYPEDEFS /////////////////////////////////////////////////////////////////////////////////
typedef struct SYSTEMCONTEXTTYPE    suSystemContext;

///////// MACROS ///////////////////////////////////////////////////////////////////////////////////

///////// FUNCTION PROTOTYPES //////////////////////////////////////////////////////////////////////

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

///////// STRUCT DECLARATIONS //////////////////////////////////////////////////////////////////////

struct SYSTEMCONTEXTTYPE
{
    eEndpoint Endpoint;
    char* sName;
};

///////// LOCAL CONSTANTS //////////////////////////////////////////////////////////////////////////


///////// EXTERNAL CONSTANTS ///////////////////////////////////////////////////////////////////////


///////// LOCAL VARIABLES //////////////////////////////////////////////////////////////////////////

static suStateMachineClass StateMachineTemplate[SYSTEM_STATE_END] =
{
    [SYSTEM_STATE_INVALID] =        {.sName = NULL, .on_Entry = NULL,                   .on_Exit = NULL,    .on_Execute = NULL                      },
    [SYSTEM_STATE_START_UP] =       {.sName = NULL, .on_Entry = Start_Up_On_Entry,      .on_Exit = NULL,    .on_Execute = Start_Up_On_Execute       },
    [SYSTEM_STATE_WAIT_FOR_DOCK] =  {.sName = NULL, .on_Entry = Wait_For_Dock_On_Entry, .on_Exit = NULL,    .on_Execute = Wait_For_Dock_On_Execute  },
    [SYSTEM_STATE_LOCKING] =        {.sName = NULL, .on_Entry = Locking_On_Entry,       .on_Exit = NULL,    .on_Execute = Locking_On_Execute        },
    [SYSTEM_STATE_UNLOCKING] =      {.sName = NULL, .on_Entry = Unlocking_On_Entry,     .on_Exit = NULL,    .on_Execute = Unlocking_On_Execute      },
    [SYSTEM_STATE_LOCKED] =         {.sName = NULL, .on_Entry = Locked_On_Entry,        .on_Exit = NULL,    .on_Execute = Locked_On_Execute         },
    [SYSTEM_STATE_FAULTED] =        {.sName = NULL, .on_Entry = Faulted_On_Entry,       .on_Exit = NULL,    .on_Execute = Faulted_On_Execute        },
};
 
static char* aSystemStateNames[SYSTEM_STATE_END] =
{
    [SYSTEM_STATE_INVALID]          = "SYSTEM_INVALID",
    [SYSTEM_STATE_START_UP]         = "SYSTEM_START_UP",
    [SYSTEM_STATE_WAIT_FOR_DOCK]    = "SYSTEM_WAIT_FOR_DOCK",
    [SYSTEM_STATE_LOCKING]          = "SYSTEM_LOCKING",
    [SYSTEM_STATE_UNLOCKING]        = "SYSTEM_UNLOCKING",
    [SYSTEM_STATE_LOCKED]           = "SYSTEM_LOCKED",
    [SYSTEM_STATE_FAULTED]          = "SYSTEM_FAULTED",
};

// aggregation of all endpoint state machines for parsed referencing
static suStateMachineClass aggregateStateMachine[ENDPOINT_COUNT][SYSTEM_STATE_END];

// object to hold all endpoint state machine contexts
static suStateMachineContext  aIndividualStateMachineContext[ENDPOINT_COUNT];
static suSystemContext aSystemContext[ENDPOINT_COUNT];

///////// EXTERNAL VARIABLES ///////////////////////////////////////////////////////////////////////

extern suSystemData systemData;
 
///////// LOCAL FUNCTIONS ////////////////////////////////////////////////////////////////////////// 

static void run_state_machine(suStateMachineContext* pStateMachineContext)
{
    suSystemContext *pSystemContext = (suSystemContext*)pStateMachineContext->pContext;
    uint32_t nextState = pStateMachineContext->nextState;
   
    if (nextState != pStateMachineContext->currentState)
    {
        if( nextState >= SYSTEM_STATE_END)
        {
            nextState = SYSTEM_STATE_FAULTED;
        }
       
        suStateReport StateReport = { .sStateName = pStateMachineContext->StateMachine[nextState].sName, .sEndpointName = pSystemContext->sName};
        network_send(STATE_REPORT, (uint8_t*)&StateReport, (strlen(StateReport.sStateName) + strlen(StateReport.sEndpointName)));
                   
        if (pStateMachineContext->StateMachine[pStateMachineContext->currentState].on_Exit != NULL)
        {
            pStateMachineContext->StateMachine[pStateMachineContext->currentState].on_Exit(pStateMachineContext);
        }
 
        pStateMachineContext->uStateStartTime = xTaskGetTickCount();
        pStateMachineContext->uStateElapsedTime = 0;
        pStateMachineContext->currentState = nextState;
 
        if (pStateMachineContext->StateMachine[pStateMachineContext->currentState].on_Entry != NULL)
        {
            pStateMachineContext->StateMachine[pStateMachineContext->currentState].on_Entry(pStateMachineContext);
        }
    }
    else
    {
        pStateMachineContext->uStateElapsedTime = (xTaskGetTickCount() - pStateMachineContext->uStateStartTime);
       
        if (pStateMachineContext->StateMachine[pStateMachineContext->currentState].on_Execute != NULL)
        {
            pStateMachineContext->nextState = pStateMachineContext->StateMachine[pStateMachineContext->currentState].on_Execute(pStateMachineContext);
        }
    }
}
 
static void Start_Up_On_Entry(void* pContext)
{
    suStateMachineContext *pStateMachineContext = (suStateMachineContext*)pContext;
    suSystemContext *pSystemContext = (suSystemContext*)pStateMachineContext->pContext;
    peripheral_unlock_clamp(pSystemContext->Endpoint);
}
 
static uint32_t Start_Up_On_Execute(void* pContext)
{
    uint32_t nextState = SYSTEM_STATE_START_UP;
   
    nextState = SYSTEM_STATE_UNLOCKING;
   
    return nextState;
}
 
static void Wait_For_Dock_On_Entry(void* pContext)
{
    suStateMachineContext *pStateMachineContext = (suStateMachineContext*)pContext;
    suSystemContext *pSystemContext = (suSystemContext*)pStateMachineContext->pContext;
    peripheral_stop_clamp(pSystemContext->Endpoint);
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
    suStateMachineContext *pStateMachineContext = (suStateMachineContext*)pContext;
    suSystemContext *pSystemContext = (suSystemContext*)pStateMachineContext->pContext;
    peripheral_lock_clamp(pSystemContext->Endpoint);
}
 
static uint32_t Locking_On_Execute(void* pContext)
{
    suStateMachineContext *pStateMachineContext = (suStateMachineContext*)pContext;
    suSystemContext *pSystemContext = (suSystemContext*)pStateMachineContext->pContext;
    uint32_t nextState = SYSTEM_STATE_LOCKING;
   
    if (systemData.bDockingRequested)
    {
        if (peripheral_get_dock_status(pSystemContext->Endpoint) == DOCK_STATUS_DOCKED)
        {
            if (peripheral_get_lock_status(pSystemContext->Endpoint) == LOCK_STATUS_LOCKED)
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
    suStateMachineContext *pStateMachineContext = (suStateMachineContext*)pContext;
    suSystemContext *pSystemContext = (suSystemContext*)pStateMachineContext->pContext;
    peripheral_unlock_clamp(pSystemContext->Endpoint);
}
 
static uint32_t Unlocking_On_Execute(void* pContext)
{
    suStateMachineContext *pStateMachineContext = (suStateMachineContext*)pContext;
    suSystemContext *pSystemContext = (suSystemContext*)pStateMachineContext->pContext;
    uint32_t nextState = SYSTEM_STATE_LOCKING;
   
    if (peripheral_get_lock_status(pSystemContext->Endpoint) == LOCK_STATUS_UNLOCKED)
    {
        // clamp fully unlocked, return to idle to proceed with next request
        nextState = SYSTEM_STATE_WAIT_FOR_DOCK;
    }
 
    return nextState;
}
 
static void Locked_On_Entry(void* pContext)
{
    suStateMachineContext *pStateMachineContext = (suStateMachineContext*)pContext;
    suSystemContext *pSystemContext = (suSystemContext*)pStateMachineContext->pContext;
    peripheral_stop_clamp(pSystemContext->Endpoint);
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
    suStateMachineContext *pStateMachineContext = (suStateMachineContext*)pContext;
    suSystemContext *pSystemContext = (suSystemContext*)pStateMachineContext->pContext;
    peripheral_unlock_clamp(pSystemContext->Endpoint);
}
 
static uint32_t Faulted_On_Execute(void* pContext)
{
    uint32_t nextState = SYSTEM_STATE_FAULTED;
   
    nextState = SYSTEM_STATE_UNLOCKING;
 
    return nextState;
}
 
void controller_task(void *pvParameter)
{
    while(1)
    {
        vTaskDelay(100);
        for (int peripheral = 0; peripheral < ENDPOINT_COUNT; peripheral++)
        {
            run_state_machine(&aIndividualStateMachineContext[peripheral]);
        }
    }
}

///////// EXTERNAL FUNCTIONS ///////////////////////////////////////////////////////////////////////

void controller_init()
{
        // initialize each endpoint state machine
    for (int endpoint = 0; endpoint < ENDPOINT_COUNT; endpoint++)
    {
        aSystemContext[endpoint].Endpoint = endpoint;
        aSystemContext[endpoint].sName = aEndpointNames[endpoint];

        memcpy(&aggregateStateMachine[endpoint], StateMachineTemplate, sizeof(StateMachineTemplate));
        for (int state = SYSTEM_STATE_START; state < SYSTEM_STATE_END; state++)
        {
            aggregateStateMachine[endpoint][state].sName = aSystemStateNames[state];
        }

        aIndividualStateMachineContext[endpoint].StateMachine = aggregateStateMachine[endpoint];
        aIndividualStateMachineContext[endpoint].currentState = SYSTEM_STATE_INVALID;
        aIndividualStateMachineContext[endpoint].nextState = SYSTEM_STATE_START_UP;
        aIndividualStateMachineContext[endpoint].pContext = (void*)&aSystemContext[endpoint];
    }
 
    xTaskCreate(controller_task, "controller_task", 2048, NULL, 4, NULL);
}

///////// POST-CODE COMPILER DIRECTIVES ////////////////////////////////////////////////////////////
