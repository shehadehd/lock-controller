#ifndef PERIPHERAL_H
#define PERIPHERAL_H

///////// PRE-INCLUDE PRE-PROCESSOR DIRECTIVES /////////////////////////////////////////////////////
		/*
			NOTE:	The following header MUST be available in the project-specific include
					directory!  It defines the build configuration for the specific project!
		*/


///////// INCLUDES /////////////////////////////////////////////////////////////////////////////////

#include "driver/gpio.h"
 
#include "common.h"

///////// POST-INCLUDE PRE-PROCESSOR DIRECTIVES ////////////////////////////////////////////////////





///////// MNEMONICS ////////////////////////////////////////////////////////////////////////////////

///////// DEFINES //////////////////////////////////////////////////////////////////////////////

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


///////// ENUMERATIONS /////////////////////////////////////////////////////////////////////////



///////// FORWARD STRUCT DECLARATIONS //////////////////////////////////////////////////////////////





///////// TYPEDEFS /////////////////////////////////////////////////////////////////////////////////


///////// MACROS ///////////////////////////////////////////////////////////////////////////////////


///////// STRUCT DECLARATIONS //////////////////////////////////////////////////////////////////////



///////// FUNCTION PROTOTYPES //////////////////////////////////////////////////////////////////////
void peripheral_init(void);
void peripheral_lock_clamp(eEndpoint Endpoint);
void peripheral_unlock_clamp(eEndpoint Endpoint);
void peripheral_stop_clamp(eEndpoint Endpoint);
eDockStatus peripheral_get_dock_status(eEndpoint Endpoint);
eLockStatus peripheral_get_lock_status(eEndpoint Endpoint);


///////// CONSTANTS ////////////////////////////////////////////////////////////////////////////////


///////// VARIABLES ////////////////////////////////////////////////////////////////////////////////



///////// POST-COMPILE PRE-PROCESSOR DIRECTIVES ////////////////////////////////////////////////////

#endif