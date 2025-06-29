///////// INCLUDES /////////////////////////////////////////////////////////////////////////////////
#include "common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include <string.h>
 
#include "communication.h"
#include "peripheral.h"
 

///////// POST-INCLUDE PRE-PROCESSOR DIRECTIVES ////////////////////////////////////////////////////





///////// MNEMONICS ////////////////////////////////////////////////////////////////////////////////


///////// DEFINES //////////////////////////////////////////////////////////////////////////////

#define DRIVE_DEADTIME_MS               1
#define PERIPHERAL_START_UP_TIMEOUT_MS  1000
 
///////// ENUMERATIONS /////////////////////////////////////////////////////////////////////////

///////// FORWARD STRUCT DECLARATIONS //////////////////////////////////////////////////////////////
struct PERIPHERALCONTEXTTYPE;

///////// TYPEDEFS /////////////////////////////////////////////////////////////////////////////////

typedef struct PERIPHERALCONTEXTTYPE    suPeripheralContext;

///////// MACROS ///////////////////////////////////////////////////////////////////////////////////

#define LOCK_CLAMP(lock_pin, unlock_pin)            {                                                   \
                                    gpio_set_level(unlock_pin, false);    \
                                    vTaskDelay(DRIVE_DEADTIME_MS); /*1ms deadtime*/ \
                                    gpio_set_level(lock_pin, true);       \
                                }
 
#define UNLOCK_CLAMP(lock_pin, unlock_pin)          {                                                   \
                                    gpio_set_level(lock_pin, false);      \
                                    vTaskDelay(DRIVE_DEADTIME_MS); /*1ms deadtime*/ \
                                    gpio_set_level(unlock_pin, true);     \
                                }
 
#define STOP_CLAMP(lock_pin, unlock_pin)            {                                                   \
                                    gpio_set_level(lock_pin, false);      \
                                    gpio_set_level(unlock_pin, false);    \
                                }

///////// FUNCTION PROTOTYPES //////////////////////////////////////////////////////////////////////

static uint32_t Start_Up_On_Execute(void* pContext);
static uint32_t Unknown_On_Execute(void* pContext);
static uint32_t Unlocked_On_Execute(void* pContext);
static uint32_t Locked_On_Execute(void* pContext);

///////// STRUCT DECLARATIONS //////////////////////////////////////////////////////////////////////

struct PERIPHERALCONTEXTTYPE
{
    eEndpoint Endpoint;
    gpio_num_t motorLockPinAddress;
    gpio_num_t motorUnlockPinAddress;
    gpio_num_t dockPinAddress;
    gpio_num_t lockPinAddress;
    bool dockPinValue;
    bool lockPinValue;

    suPeripheralData PeripheralData;
    char *sName;
};

///////// LOCAL CONSTANTS //////////////////////////////////////////////////////////////////////////


///////// EXTERNAL CONSTANTS ///////////////////////////////////////////////////////////////////////


///////// LOCAL VARIABLES //////////////////////////////////////////////////////////////////////////
 
// Framework that each endpoint uses for status evaluation
static suStateMachineClass individualStateMachine[PERIPHERAL_STATE_END] =
{
    [PERIPHERAL_STATE_START_UP] = {.sName = NULL,   .on_Entry = NULL,   .on_Exit = NULL,    .on_Execute = Start_Up_On_Execute   },
    [PERIPHERAL_STATE_UNKNOWN] =  {.sName = NULL,   .on_Entry = NULL,   .on_Exit = NULL,    .on_Execute = Unknown_On_Execute    },
    [PERIPHERAL_STATE_UNLOCKED] = {.sName = NULL,   .on_Entry = NULL,   .on_Exit = NULL,    .on_Execute = Unlocked_On_Execute   },
    [PERIPHERAL_STATE_LOCKED] =   {.sName = NULL,   .on_Entry = NULL,   .on_Exit = NULL,    .on_Execute = Locked_On_Execute     },
};
 
// aggregation of all endpoint state machines for parsed referencing
static suStateMachineClass* aggregateStateMachine[ENDPOINT_COUNT] =
{
    [ENDPOINT_BOTTOM_LEFT] =      &individualStateMachine[ENDPOINT_BOTTOM_LEFT],
    [ENDPOINT_BOTTOM_RIGHT] =     &individualStateMachine[ENDPOINT_BOTTOM_RIGHT],
    [ENDPOINT_MIDDLE_LEFT] =      &individualStateMachine[ENDPOINT_MIDDLE_LEFT],
    [ENDPOINT_MIDDLE_RIGHT] =     &individualStateMachine[ENDPOINT_MIDDLE_RIGHT],
    [ENDPOINT_MIDDLE_CENTER] =    &individualStateMachine[ENDPOINT_MIDDLE_CENTER],
    [ENDPOINT_UPPER_LEFT] =       &individualStateMachine[ENDPOINT_UPPER_LEFT],
    [ENDPOINT_UPPER_RIGHT] =      &individualStateMachine[ENDPOINT_UPPER_RIGHT],
    [ENDPOINT_TOP_LEFT] =         &individualStateMachine[ENDPOINT_TOP_LEFT],
    [ENDPOINT_TOP_RIGHT] =        &individualStateMachine[ENDPOINT_TOP_RIGHT],
    [ENDPOINT_TOP_CENTER] =       &individualStateMachine[ENDPOINT_TOP_CENTER],
};

// list of all endpoint lock peripherals
static gpio_num_t aLockPin[ENDPOINT_COUNT] = 
{
    [ENDPOINT_BOTTOM_LEFT]    = GPIO_LOCK_1_,
    [ENDPOINT_BOTTOM_RIGHT]   = GPIO_LOCK_2_,
    [ENDPOINT_MIDDLE_LEFT]    = GPIO_LOCK_3_,
    [ENDPOINT_MIDDLE_RIGHT]   = GPIO_LOCK_4_,
    [ENDPOINT_MIDDLE_CENTER]  = GPIO_LOCK_5_,
    [ENDPOINT_UPPER_LEFT]     = GPIO_LOCK_6_,
    [ENDPOINT_UPPER_RIGHT]    = GPIO_LOCK_7_,
    [ENDPOINT_TOP_LEFT]       = GPIO_LOCK_8_,
    [ENDPOINT_TOP_RIGHT]      = GPIO_LOCK_9_,
    [ENDPOINT_TOP_CENTER]     = GPIO_LOCK_10_,
};
 
// list of all endpoint dock peripherals
static gpio_num_t aDockPin[ENDPOINT_COUNT] = 
{
    [ENDPOINT_BOTTOM_LEFT]    = GPIO_DOCK_1_,
    [ENDPOINT_BOTTOM_RIGHT]   = GPIO_DOCK_2_,
    [ENDPOINT_MIDDLE_LEFT]    = GPIO_DOCK_3_,
    [ENDPOINT_MIDDLE_RIGHT]   = GPIO_DOCK_4_,
    [ENDPOINT_MIDDLE_CENTER]  = GPIO_DOCK_5_,
    [ENDPOINT_UPPER_LEFT]     = GPIO_DOCK_6_,
    [ENDPOINT_UPPER_RIGHT]    = GPIO_DOCK_7_,
    [ENDPOINT_TOP_LEFT]       = GPIO_DOCK_8_,
    [ENDPOINT_TOP_RIGHT]      = GPIO_DOCK_9_,
    [ENDPOINT_TOP_CENTER]     = GPIO_DOCK_10_,
};
 
// list of all endpoint dock peripherals
static gpio_num_t aMotorLockPin[ENDPOINT_COUNT] = 
{
    [ENDPOINT_BOTTOM_LEFT]    = GPIO_MOTOR_LOCK_1_,
    [ENDPOINT_BOTTOM_RIGHT]   = GPIO_MOTOR_LOCK_2_,
    [ENDPOINT_MIDDLE_LEFT]    = GPIO_MOTOR_LOCK_3_,
    [ENDPOINT_MIDDLE_RIGHT]   = GPIO_MOTOR_LOCK_4_,
    [ENDPOINT_MIDDLE_CENTER]  = GPIO_MOTOR_LOCK_5_,
    [ENDPOINT_UPPER_LEFT]     = GPIO_MOTOR_LOCK_6_,
    [ENDPOINT_UPPER_RIGHT]    = GPIO_MOTOR_LOCK_7_,
    [ENDPOINT_TOP_LEFT]       = GPIO_MOTOR_LOCK_8_,
    [ENDPOINT_TOP_RIGHT]      = GPIO_MOTOR_LOCK_9_,
    [ENDPOINT_TOP_CENTER]     = GPIO_MOTOR_LOCK_10_,
};
 
// list of all endpoint dock peripherals
static gpio_num_t aMotorUnlockPin[ENDPOINT_COUNT] = 
{
    [ENDPOINT_BOTTOM_LEFT]    = GPIO_MOTOR_UNLOCK_1_,
    [ENDPOINT_BOTTOM_RIGHT]   = GPIO_MOTOR_UNLOCK_2_,
    [ENDPOINT_MIDDLE_LEFT]    = GPIO_MOTOR_UNLOCK_3_,
    [ENDPOINT_MIDDLE_RIGHT]   = GPIO_MOTOR_UNLOCK_4_,
    [ENDPOINT_MIDDLE_CENTER]  = GPIO_MOTOR_UNLOCK_5_,
    [ENDPOINT_UPPER_LEFT]     = GPIO_MOTOR_UNLOCK_6_,
    [ENDPOINT_UPPER_RIGHT]    = GPIO_MOTOR_UNLOCK_7_,
    [ENDPOINT_TOP_LEFT]       = GPIO_MOTOR_UNLOCK_8_,
    [ENDPOINT_TOP_RIGHT]      = GPIO_MOTOR_UNLOCK_9_,
    [ENDPOINT_TOP_CENTER]     = GPIO_MOTOR_UNLOCK_10_,
};



static char* aPeripheralStateNames[PERIPHERAL_STATE_END] =
{
    [PERIPHERAL_STATE_START_UP]     = "PERIPHERAL_START_UP",
    [PERIPHERAL_STATE_UNKNOWN]      = "PERIPHERAL_UNKNOWN",
    [PERIPHERAL_STATE_UNLOCKED]     = "PERIPHERAL_UNLOCKED",
    [PERIPHERAL_STATE_LOCKED]       = "PERIPHERAL_LOCKED",
};

// object to hold all endpoint state machine contexts
static suStateMachineContext  aIndividualStateMachineContext[ENDPOINT_COUNT];
 
static suPeripheralContext aPeripheralContext[ENDPOINT_COUNT];

///////// EXTERNAL VARIABLES ///////////////////////////////////////////////////////////////////////

extern suSystemData        systemData;


///////// LOCAL FUNCTIONS ////////////////////////////////////////////////////////////////////////// 
 
static void run_state_machine(suStateMachineContext StateMachineContext)
{
    suPeripheralContext *pPeripheralContext = (suPeripheralContext*)StateMachineContext.pContext;
    uint32_t nextState = StateMachineContext.nextState;
   
    if (nextState != StateMachineContext.currentState)
    {
        if( nextState >= SYSTEM_STATE_END)
        {
            nextState = SYSTEM_STATE_FAULTED;
        }
       
        suStateReport StateReport = { .sStateName = StateMachineContext.StateMachine[nextState].sName, .sEndpointName = pPeripheralContext->sName};
        network_send(STATE_REPORT, (uint8_t*)&StateReport, (strlen(StateReport.sStateName) + strlen(StateReport.sEndpointName)));
                   
        if (StateMachineContext.StateMachine[StateMachineContext.currentState].on_Exit != NULL)
        {
            StateMachineContext.StateMachine[StateMachineContext.currentState].on_Exit(&StateMachineContext);
        }
 
        StateMachineContext.uStateStartTime = xTaskGetTickCount();
        StateMachineContext.uStateElapsedTime = 0;
        StateMachineContext.currentState = nextState;
 
        if (StateMachineContext.StateMachine[StateMachineContext.currentState].on_Entry != NULL)
        {
            StateMachineContext.StateMachine[StateMachineContext.currentState].on_Entry(&StateMachineContext);
        }
    }
    else
    {
        StateMachineContext.uStateElapsedTime = (xTaskGetTickCount() - StateMachineContext.uStateStartTime);
       
        if (StateMachineContext.StateMachine[StateMachineContext.currentState].on_Execute != NULL)
        {
            nextState = StateMachineContext.StateMachine[StateMachineContext.currentState].on_Execute(&StateMachineContext);
        }
    }
}
 
static uint32_t Start_Up_On_Execute(void* pContext)
{
    suStateMachineContext *pStateMachineContext = (suStateMachineContext*)pContext;
    suPeripheralContext *pPeripheralContext = (suPeripheralContext*)pStateMachineContext->pContext;
    uint32_t nextState = PERIPHERAL_STATE_START_UP;
 
    if(pPeripheralContext->dockPinValue == ASSERTED)
    {
        if (pPeripheralContext->PeripheralData.MotorStatus == MOTOR_STATUS_UNLOCKING)
        {
            if (pStateMachineContext->uStateElapsedTime > PERIPHERAL_START_UP_TIMEOUT_MS)
            {
                pPeripheralContext->PeripheralData.MotorStatus = MOTOR_STATUS_UNLOCKED;
                pPeripheralContext->PeripheralData.LockStatus = LOCK_STATUS_UNLOCKED;
                nextState = PERIPHERAL_STATE_UNLOCKED;
            }
        }
    }
 
    return nextState;
}
 
static uint32_t Unknown_On_Execute(void* pContext)
{
    suStateMachineContext *pStateMachineContext = (suStateMachineContext*)pContext;
    suPeripheralContext *pPeripheralContext = (suPeripheralContext*)pStateMachineContext->pContext;
    uint32_t nextState = PERIPHERAL_STATE_UNKNOWN;
   
    if(pPeripheralContext->lockPinValue == ASSERTED)
    {
        if (pPeripheralContext->PeripheralData.MotorStatus == MOTOR_STATUS_LOCKING)
        {
            pPeripheralContext->PeripheralData.MotorStatus = MOTOR_STATUS_LOCKED;
            pPeripheralContext->PeripheralData.LockStatus = LOCK_STATUS_LOCKED;
            nextState = PERIPHERAL_STATE_LOCKED;
        }
        else if (pPeripheralContext->PeripheralData.MotorStatus == MOTOR_STATUS_UNLOCKING)
        {
            pPeripheralContext->PeripheralData.MotorStatus = MOTOR_STATUS_UNLOCKED;
            pPeripheralContext->PeripheralData.LockStatus = LOCK_STATUS_UNLOCKED;
            nextState = PERIPHERAL_STATE_UNLOCKED;
        }
    }
 
    return nextState;
}
 
static uint32_t Unlocked_On_Execute(void* pContext)
{
    suStateMachineContext *pStateMachineContext = (suStateMachineContext*)pContext;
    suPeripheralContext *pPeripheralContext = (suPeripheralContext*)pStateMachineContext->pContext;
    uint32_t nextState = PERIPHERAL_STATE_UNLOCKED;
   
    if(pPeripheralContext->lockPinValue != ASSERTED)
    {
        pPeripheralContext->PeripheralData.LockStatus = LOCK_STATUS_UNKNOWN;
        nextState = PERIPHERAL_STATE_UNKNOWN;
    }
 
    return nextState;
}
 
static uint32_t Locked_On_Execute(void* pContext)
{
    suStateMachineContext *pStateMachineContext = (suStateMachineContext*)pContext;
    suPeripheralContext *pPeripheralContext = (suPeripheralContext*)pStateMachineContext->pContext;
    uint32_t nextState = PERIPHERAL_STATE_LOCKED;
   
    if(pPeripheralContext->lockPinValue != ASSERTED)
    {
        pPeripheralContext->PeripheralData.LockStatus = LOCK_STATUS_UNKNOWN;
        nextState = PERIPHERAL_STATE_UNKNOWN;
    }
 
    return nextState;
}

void update_peripheral(suPeripheralContext *pPeripheralContext, uint32_t peripheral)
{
    pPeripheralContext->dockPinValue = (gpio_get_level(pPeripheralContext->dockPinAddress) == ASSERTED);
    pPeripheralContext->PeripheralData.DockStatus = (pPeripheralContext->dockPinValue == ASSERTED) ? DOCK_STATUS_DOCKED : DOCK_STATUS_UNDOCKED;

    pPeripheralContext->lockPinValue = (gpio_get_level(pPeripheralContext->lockPinAddress) == ASSERTED);
}

void aggregate_peripheral_data(void)
{
    eDockStatus DockStatus = DOCK_STATUS_DOCKED;
    eLockStatus LockStatus = LOCK_STATUS_LOCKED;

    // calculate Dock aggregate. If any are unknown or undocked, then set the aggregate to match
    for (int endpoint = 0; endpoint < ENDPOINT_COUNT; endpoint++)
    {
        if (aPeripheralContext[endpoint].PeripheralData.DockStatus == DOCK_STATUS_UNKNOWN)
        {
            DockStatus = DOCK_STATUS_UNKNOWN;
            break;
        }
        else if (aPeripheralContext[endpoint].PeripheralData.DockStatus == DOCK_STATUS_UNDOCKED)
        {
            DockStatus = DOCK_STATUS_UNDOCKED;
            break;
        }
    }

    // calculate Lock aggregate. If any are unknown or unlocked, then set the aggregate to match
    for (int endpoint = 0; endpoint < ENDPOINT_COUNT; endpoint++)
    {
        if (aPeripheralContext[endpoint].PeripheralData.LockStatus == LOCK_STATUS_UNKNOWN)
        {
            LockStatus = LOCK_STATUS_UNKNOWN;
            break;
        }
        else if (aPeripheralContext[endpoint].PeripheralData.LockStatus == LOCK_STATUS_UNLOCKED)
        {
            LockStatus = LOCK_STATUS_UNLOCKED;
            break;
        }
    }

    systemData.AggregatePeripheralData.DockStatus = DockStatus;
    systemData.AggregatePeripheralData.LockStatus = LockStatus;
}
 
void peripheral_task(void *pvParameter)
{
    while(1)
    {
        vTaskDelay(1000);
        for (int peripheral = 0; peripheral < ENDPOINT_COUNT; peripheral++)
        {
            update_peripheral(aIndividualStateMachineContext[peripheral].pContext, peripheral);
            run_state_machine(aIndividualStateMachineContext[peripheral]);
        }
        aggregate_peripheral_data();
        network_send(SYSTEM_REPORT, (uint8_t*)&systemData, sizeof(systemData));
        // network_send((uint8_t*)&systemData.PeripheralData.DockedState, sizeof(systemData.PeripheralData.DockedState), DOCK_COMMAND);
    }
}

///////// EXTERNAL FUNCTIONS ///////////////////////////////////////////////////////////////////////

void peripheral_init(void)
{
    gpio_config_t gpio_conf = {0};
 
    // initialize each endpoint state machine
    for (int endpoint = 0; endpoint < ENDPOINT_COUNT; endpoint++)
    {
        aPeripheralContext[endpoint].Endpoint = endpoint;
        aPeripheralContext[endpoint].lockPinAddress = aLockPin[endpoint];
        aPeripheralContext[endpoint].dockPinAddress = aDockPin[endpoint];
        aPeripheralContext[endpoint].motorLockPinAddress = aMotorLockPin[endpoint];
        aPeripheralContext[endpoint].motorUnlockPinAddress = aMotorUnlockPin[endpoint];
        aPeripheralContext[endpoint].PeripheralData.DockStatus = DOCK_STATUS_UNKNOWN;
        aPeripheralContext[endpoint].PeripheralData.LockStatus = LOCK_STATUS_UNKNOWN;
        aPeripheralContext[endpoint].PeripheralData.MotorStatus = MOTOR_STATUS_STOPPED;
        aPeripheralContext[endpoint].sName = aEndpointNames[endpoint];

        aggregateStateMachine[endpoint]->sName = aPeripheralStateNames[endpoint];

        aIndividualStateMachineContext[endpoint].StateMachine = aggregateStateMachine[endpoint];
        aIndividualStateMachineContext[endpoint].currentState = PERIPHERAL_STATE_START_UP;
        aIndividualStateMachineContext[endpoint].nextState = PERIPHERAL_STATE_START_UP;
        
        aIndividualStateMachineContext[endpoint].pContext = (void*)&aPeripheralContext[endpoint];
    }

    systemData.AggregatePeripheralData.DockStatus = DOCK_STATUS_UNKNOWN;
    systemData.AggregatePeripheralData.LockStatus = LOCK_STATUS_UNKNOWN;
    systemData.AggregatePeripheralData.MotorStatus = MOTOR_STATUS_STOPPED;
 
    gpio_conf.intr_type = GPIO_INTR_DISABLE;    // Disable interrupt for now
    gpio_conf.mode = GPIO_MODE_INPUT;  // Set as input mode
    for (int endpoint = 0; endpoint < ENDPOINT_COUNT; endpoint++)
    {
        gpio_conf.pin_bit_mask |= 1ULL << aLockPin[endpoint];
        gpio_conf.pin_bit_mask |= 1ULL << aDockPin[endpoint];
    }
    gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;  // Enable internal pull-up resistor
    gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // Disable pull-down
    gpio_config(&gpio_conf);
 
    gpio_conf.intr_type = GPIO_INTR_DISABLE;    // Disable interrupt for now
    gpio_conf.mode = GPIO_MODE_OUTPUT;  // Set as input mode
    gpio_conf.pin_bit_mask = 0;
    for (int endpoint = 0; endpoint < ENDPOINT_COUNT; endpoint++)
    {
        gpio_conf.pin_bit_mask |= 1ULL << aMotorLockPin[endpoint];
        gpio_conf.pin_bit_mask |= 1ULL << aMotorUnlockPin[endpoint];
    }
    gpio_conf.pull_up_en = GPIO_PULLUP_DISABLE;  // Disable internal pull-up resistor
    gpio_conf.pull_down_en = GPIO_PULLDOWN_ENABLE; // Enable internal pull-down
    gpio_config(&gpio_conf);  
   
    xTaskCreate(peripheral_task, "state_machine_task", 2048, NULL, 4, NULL);
}
 
void peripheral_lock_clamp(eEndpoint Endpoint)
{
    aPeripheralContext[Endpoint].PeripheralData.MotorStatus = MOTOR_STATUS_LOCKING;
    LOCK_CLAMP(aPeripheralContext[Endpoint].motorLockPinAddress, aPeripheralContext[Endpoint].motorUnlockPinAddress);
}
 
void peripheral_unlock_clamp(eEndpoint Endpoint)
{
    aPeripheralContext[Endpoint].PeripheralData.MotorStatus = MOTOR_STATUS_UNLOCKING;
    UNLOCK_CLAMP(aPeripheralContext[Endpoint].motorLockPinAddress, aPeripheralContext[Endpoint].motorUnlockPinAddress);
}
 
void peripheral_stop_clamp(eEndpoint Endpoint)
{
    aPeripheralContext[Endpoint].PeripheralData.MotorStatus = MOTOR_STATUS_STOPPED;
    STOP_CLAMP(aPeripheralContext[Endpoint].motorLockPinAddress, aPeripheralContext[Endpoint].motorUnlockPinAddress);
}

eDockStatus peripheral_get_dock_status(eEndpoint Endpoint)
{
    return aPeripheralContext[Endpoint].PeripheralData.DockStatus;
}

eLockStatus peripheral_get_lock_status(eEndpoint Endpoint)
{
    return aPeripheralContext[Endpoint].PeripheralData.LockStatus;
}

///////// POST-CODE COMPILER DIRECTIVES ////////////////////////////////////////////////////////////
