#ifndef COMMON_H
#define COMMON_H
 
///////// PRE-INCLUDE PRE-PROCESSOR DIRECTIVES /////////////////////////////////////////////////////

///////// INCLUDES /////////////////////////////////////////////////////////////////////////////////

#include <inttypes.h>
#include "esp_now.h"
 
///////// POST-INCLUDE PRE-PROCESSOR DIRECTIVES ////////////////////////////////////////////////////





///////// MNEMONICS ////////////////////////////////////////////////////////////////////////////////

///////// DEFINES //////////////////////////////////////////////////////////////////////////////

#define MAX_PACKET_LENGTH 256
 
#define ASSERTED        true
#define DEASSERTED      false
 
///////// ENUMERATIONS /////////////////////////////////////////////////////////////////////////

enum SYSTEMSTATES
{
    SYSTEM_STATE_START,
    SYSTEM_STATE_INVALID = SYSTEM_STATE_START,
    
    SYSTEM_STATE_START_UP,
    SYSTEM_STATE_WAIT_FOR_DOCK,
    SYSTEM_STATE_LOCKING,
    SYSTEM_STATE_UNLOCKING,
    SYSTEM_STATE_LOCKED,
    SYSTEM_STATE_FAULTED,
 
    SYSTEM_STATE_END
};
 
enum PERIPHERALSTATES
{
    PERIPHERAL_STATE_START,
    PERIPHERAL_STATE_INVALID = PERIPHERAL_STATE_START,

    PERIPHERAL_STATE_START_UP,
    PERIPHERAL_STATE_UNKNOWN,
    PERIPHERAL_STATE_UNLOCKED,
    PERIPHERAL_STATE_LOCKED,
 
    PERIPHERAL_STATE_END
};
 
enum DOCKSTATUS
{
    DOCK_STATUS_UNKNOWN,
    DOCK_STATUS_UNDOCKED,
    DOCK_STATUS_DOCKED,
};
 
enum LOCKSTATUS
{
    LOCK_STATUS_UNKNOWN,
    LOCK_STATUS_UNLOCKED,
    LOCK_STATUS_LOCKED,
};
 
enum MOTORSTATUS
{
    MOTOR_STATUS_LOCKING,
    MOTOR_STATUS_UNLOCKING,
    MOTOR_STATUS_LOCKED,
    MOTOR_STATUS_UNLOCKED,
    MOTOR_STATUS_STOPPED,
};
 
enum MESSAGEIDTYPE
{
    HELLO           = 0,
    SYSTEM_REPORT   = 10,
    STATE_REPORT,
    ERROR_REPORT,
 
    DOCK_COMMAND    = 20,
 
    MESSAGE_ID_END  = UINT8_MAX
};

enum ENDPOINTTYPE
{
    ENDPOINT_BOTTOM_LEFT,
    ENDPOINT_BOTTOM_RIGHT,
    ENDPOINT_MIDDLE_LEFT,
    ENDPOINT_MIDDLE_RIGHT,
    ENDPOINT_MIDDLE_CENTER,
    ENDPOINT_UPPER_LEFT,
    ENDPOINT_UPPER_RIGHT,
    ENDPOINT_TOP_LEFT,
    ENDPOINT_TOP_RIGHT,
    ENDPOINT_TOP_CENTER,

    ENDPOINT_COUNT
};

///////// FORWARD STRUCT DECLARATIONS //////////////////////////////////////////////////////////////

struct STATEMACHINECONTEXTTYPE;
struct SYSTEMDATATYPE;
struct STATEMACHINECLASSTYPE;
struct PERIPHERALDATATYPE;
struct STATEREPORTTYPE;

struct NETWORKHEADERTYPE;
struct SENDDATAPACKETTYPE;
struct NETWORKRECEIVEDATATYPE;
 
///////// TYPEDEFS /////////////////////////////////////////////////////////////////////////////////

typedef enum SYSTEMSTATES               eSystemState;
typedef enum DOCKSTATUS                 eDockStatus;
typedef enum LOCKSTATUS                 eLockStatus;
typedef enum MOTORSTATUS                eMotorStatus;
typedef enum PERIPHERALSTATES           ePeripheralState;
typedef enum MESSAGEIDTYPE              eMessageId;
typedef enum ENDPOINTTYPE               eEndpoint;

typedef struct STATEMACHINECONTEXTTYPE  suStateMachineContext;
typedef struct SYSTEMDATATYPE           suSystemData;
typedef struct STATEMACHINECLASSTYPE    suStateMachineClass;
typedef struct PERIPHERALDATATYPE       suPeripheralData;
typedef struct STATEREPORTTYPE          suStateReport;

typedef struct NETWORKHEADERTYPE        suNetworkHeader;
typedef struct NETWORKPACKETTYPE        suNetworkPacket;
typedef struct NETWORKRECEIVEDATATYPE   suNetworkReceiveData;
 
typedef void (*state_entry)(void* pContext);
typedef uint32_t (*state_execute)(void* pContext);
typedef void (*state_exit)(void* pContext);
 
///////// MACROS ///////////////////////////////////////////////////////////////////////////////////


///////// STRUCT DECLARATIONS //////////////////////////////////////////////////////////////////////

/* User defined field of data. */
#pragma pack(push,1)
struct NETWORKHEADERTYPE
{
    eMessageId  MessageId;
    uint16_t seq_num;                     //Sequence number of ESPNOW data.
    uint16_t crc;                         //CRC16 value of ESPNOW data.
    uint8_t uLength;
};
#pragma pack(pop)
 
#pragma pack(push,1)
struct NETWORKPACKETTYPE
{
    suNetworkHeader Header;
    uint8_t payload[MAX_PACKET_LENGTH];                   //Real payload of ESPNOW data.
};
#pragma pack(pop)
 
struct NETWORKRECEIVEDATATYPE
{
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    uint8_t uLength;
    suNetworkPacket NetworkPacket;
};
 
struct STATEMACHINECLASSTYPE
{
    char *sName;
    state_entry on_Entry;
    state_exit on_Exit;
    state_execute on_Execute;
};
 
struct STATEMACHINECONTEXTTYPE
{
    suStateMachineClass *StateMachine;
    uint32_t currentState;
    uint32_t nextState;
    uint32_t uStateStartTime;
    uint32_t uStateElapsedTime;
    void* pContext;
};
 
#pragma pack(push,1)
struct PERIPHERALDATATYPE
{
    eDockStatus DockStatus;
    eLockStatus LockStatus;
    eMotorStatus MotorStatus;
};
#pragma pack(pop)
 
struct SYSTEMDATATYPE
{
    bool bDockingRequested;
    suPeripheralData AggregatePeripheralData;
};

#pragma pack(push,1)
struct STATEREPORTTYPE
{
    char* sEndpointName;
    char *sStateName;
};
#pragma pack(pop)
 
///////// FUNCTION PROTOTYPES //////////////////////////////////////////////////////////////////////



///////// CONSTANTS ////////////////////////////////////////////////////////////////////////////////


///////// VARIABLES ////////////////////////////////////////////////////////////////////////////////

extern char* aEndpointNames[ENDPOINT_COUNT];

///////// POST-COMPILE PRE-PROCESSOR DIRECTIVES ////////////////////////////////////////////////////

#endif