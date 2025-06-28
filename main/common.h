#ifndef COMMON_H
#define COMMON_H
 
#include <inttypes.h>
#include "esp_now.h"
 
 
#define MAX_PACKET_LENGTH 256
 
#define ASSERTED        true
#define DEASSERTED      false
 
enum SYSTEMSTATES
{
    SYSTEM_STATE_START,
   
    SYSTEM_STATE_START_UP = SYSTEM_STATE_START,
    SYSTEM_STATE_WAIT_FOR_DOCK,
    SYSTEM_STATE_LOCKING,
    SYSTEM_STATE_UNLOCKING,
    SYSTEM_STATE_LOCKED,
    SYSTEM_STATE_FAULTED,
 
    SYSTEM_STATE_END
};
 
enum LOCKSTATES
{
    LOCK_STATE_START,
 
    LOCK_STATE_START_UP = LOCK_STATE_START,
    LOCK_STATE_UNKNOWN,
    LOCK_STATE_UNLOCKED,
    LOCK_STATE_LOCKED,
 
    LOCK_STATE_END
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
    SYSTEM_STATE_REPORT,
    LOCK_STATE_REPORT,
    ERROR_REPORT,
 
    DOCK_COMMAND    = 20,
 
    MESSAGE_ID_END  = UINT8_MAX
};
 
struct STATEMACHINEDATATYPE;
struct SYSTEMDATATYPE;
struct STATEMACHINECLASSTYPE;
struct PERIPHERALDATATYPE;
 
struct NETWORKHEADERTYPE;
struct SENDDATAPACKETTYPE;
struct NETWORKRECEIVEDATATYPE;
 
typedef enum SYSTEMSTATES               eSystemState;
typedef enum DOCKSTATUS                 eDockStatus;
typedef enum LOCKSTATUS                 eLockStatus;
typedef enum MOTORSTATUS                eMotorStatus;
typedef enum DOCKSTATES                 eDockState;
typedef enum LOCKSTATES                 eLockState;
typedef enum MESSAGEIDTYPE              eMessageId;
 
typedef struct STATEMACHINEDATATYPE     suStateMachineData;
typedef struct SYSTEMDATATYPE           suSystemData;
typedef struct STATEMACHINECLASSTYPE    suStateMachineClass;
typedef struct PERIPHERALDATATYPE       suPeripheralData;
 
typedef struct NETWORKHEADERTYPE        suNetworkHeader;
typedef struct NETWORKPACKETTYPE        suNetworkPacket;
typedef struct NETWORKRECEIVEDATATYPE   suNetworkReceiveData;
 
typedef void (*state_entry)(void);
typedef uint32_t (*state_execute)(void);
typedef void (*state_exit)(void);
 
 
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
 
struct STATEMACHINEDATATYPE
{
    suStateMachineClass *states;
    uint32_t currentState;
    uint32_t uStateStartTime;
    uint32_t uStateElapsedTime;
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
    suPeripheralData PeripheralData;
};
 
#endif