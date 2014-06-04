#ifndef DTCT_H
#define DTCT_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"

/*********************************************************************
 * CONSTANTS
 */

// These constants are only for example and should be changed to the
// device's needs

#define HEARTBEAT_ENDPOINT      10

#define DTCT_PROFID             0x0F08
#define DTCT_DEVICEID           0x0001
#define DTCT_DEVICE_VERSION     0
#define DTCT_FLAGS              0

#define DTCT_MAX_CLUSTERS       1
#define DTCT_PERIODIC_CLUSTERID 1

// Send Message Timeout
#define DTCT_TIMER_MSG_TIMEOUT  1    // Every 1ms
// LED and WatchDog Timeout
#define DTCT_LED_WTD_EVT_TIMEOUT   200   // Every 200ms

// Application Events (OSAL) - These are bit weighted definitions.
#define DTCT_HEARTBEAT_MSG_EVT  0X0001
// LED and WatchDog EVT
#define DTCT_LED_WTD_EVT        0x0100

//index 16
#define HEART_BIT_STATUS_INDEX (SN_LEN + NV_LEN)
//index 12
#define HEART_BIT_AD1_INDEX (SN_LEN + TIME_LEN)
#define HEART_BIT_MSG_LEN  (SN_LEN + NV_LEN + 1)

typedef void (*PFUNC)(uint8 *);
/*********************************************************************
 * MACROS
 */
/*********************************************************************
 * FUNCTIONS
 */

/*
 * Task Initialization for the Generic Application
 */
extern void CurrentDetectionT1_Init( uint8 task_id );

/*
 * Task Event Processor for the Generic Application
 */
extern UINT16 CurrentDetectionT1_ProcessEvent( uint8 task_id, uint16 events );

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* DTCT_H */
