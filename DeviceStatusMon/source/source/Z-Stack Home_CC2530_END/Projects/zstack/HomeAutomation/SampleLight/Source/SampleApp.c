/**************************************************************************************************
  Filename:       SampleApp.c
  Revised:        $Date: 2009-03-18 15:56:27 -0700 (Wed, 18 Mar 2009) $
  Revision:       $Revision: 19453 $

  Description:    Sample Application (no Profile).


  Copyright 2007 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

/*********************************************************************
  This application isn't intended to do anything useful, it is
  intended to be a simple example of an application's structure.

  This application sends it's messages either as broadcast or
  broadcast filtered group messages.  The other (more normal)
  message addressing is unicast.  Most of the other sample
  applications are written to support the unicast message model.

  Key control:
    SW1:  Sends a flash command to all devices in Group 1.
    SW2:  Adds/Removes (toggles) this device in and out
          of Group 1.  This will enable and disable the
          reception of the flash command.
*********************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "OSAL.h"
#include "OSAL_Nv.h"
#include "ZGlobals.h"
#include "AF.h"
#include "aps_groups.h"
#include "ZDApp.h"

#include "SampleApp.h"
#include "SampleAppHw.h"

#include "OnBoard.h"

#include "MT_Uart.h"
#include "MT.h"

/* MAC */
#include "ioCC2530.h"

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_mcu.h"

#include "string.h"
#include <stdio.h>
#include <stdlib.h>
/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */
#ifdef DEBUG_TRACE
uint8 SnHeadStr[] = "serial number is:";
uint8 curTimeHeadStr[] = "curTime is:";
extern int8 macRssi;
#endif
/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
static uint8 EndDeviceStatus[]    = "Connect as EndDevice\n";

// This list should be filled with Application specific Cluster IDs.
const cId_t SampleApp_ClusterList[SAMPLEAPP_MAX_CLUSTERS] =
{
  SAMPLEAPP_PERIODIC_CLUSTERID,
  SAMPLEAPP_FLASH_CLUSTERID
};

const SimpleDescriptionFormat_t SampleApp_SimpleDesc =
{
  SAMPLEAPP_ENDPOINT,              //  int Endpoint;
  SAMPLEAPP_PROFID,                //  uint16 AppProfId[2];
  SAMPLEAPP_DEVICEID,              //  uint16 AppDeviceId[2];
  SAMPLEAPP_DEVICE_VERSION,        //  int   AppDevVer:4;
  SAMPLEAPP_FLAGS,                 //  int   AppFlags:4;
  SAMPLEAPP_MAX_CLUSTERS,          //  uint8  AppNumInClusters;
  (cId_t *)SampleApp_ClusterList,  //  uint8 *pAppInClusterList;
  SAMPLEAPP_MAX_CLUSTERS,          //  uint8  AppNumInClusters;
  (cId_t *)SampleApp_ClusterList   //  uint8 *pAppInClusterList;
};

// This is the Endpoint/Interface description.  It is defined here, but
// filled-in in SampleApp_Init().  Another way to go would be to fill
// in the structure here and make it a "const" (in code space).  The
// way it's defined in this sample app it is define in RAM.
endPointDesc_t SampleApp_epDesc;

const SimpleDescriptionFormat_t SampleApp_HeartBeatSimpleDesc =
{
  HEARTBEAT_ENDPOINT,              //  int Endpoint;
  SAMPLEAPP_PROFID,                //  uint16 AppProfId[2];
  SAMPLEAPP_DEVICEID,              //  uint16 AppDeviceId[2];
  SAMPLEAPP_DEVICE_VERSION,        //  int   AppDevVer:4;
  SAMPLEAPP_FLAGS,                 //  int   AppFlags:4;
  SAMPLEAPP_MAX_CLUSTERS,          //  uint8  AppNumInClusters;
  (cId_t *)SampleApp_ClusterList,  //  uint8 *pAppInClusterList;
  SAMPLEAPP_MAX_CLUSTERS,          //  uint8  AppNumInClusters;
  (cId_t *)SampleApp_ClusterList   //  uint8 *pAppInClusterList;
};

endPointDesc_t SampleApp_HeartBeatEpDesc;

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
uint8 SampleApp_TaskID;   // Task ID for internal task/event processing
                          // This variable will be received when
                          // SampleApp_Init() is called.
devStates_t SampleApp_NwkState;

uint8 SampleApp_TransID;  // This is the unique message ID (counter)

afAddrType_t SampleApp_Periodic_DstAddr;
afAddrType_t SampleApp_HeartBeat_DstAddr;

aps_Group_t SampleApp_Group;

uint8 SampleAppPeriodicCounter = 0;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void SampleApp_HandleKeys( uint8 shift, uint8 keys );
void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pckt );
void SampleApp_SerialCMD(mtOSALSerialData_t* cmdMsg);

void SampleApp_SendHeartBeatMessage(void);
void SampleApp_RecoverHeartBeatMessage(void);

#ifdef DEBUG_TRACE
static void ShowHeartBeatInfo(uint8 *serialNumber, uint8 *curTime);
static void FormatDebugInfo(uint8 *dst, const uint8 * src, uint8 len);
static void ShowRssiInfo( void );
#endif
/*********************************************************************
 * NETWORK LAYER CALLBACKS
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

#define END_ID    2
#define SN_LEN    4
#define TIME_LEN  4
#define NV_CONFIG_DATA 0x1000
#define NV_RECOVER_DATA 0x1001

static uint8 serialNumber[SN_LEN] = {0};
static uint32 lastNvTime = 0;
static uint32 nvMsgNum = 0;
static uint32 recoverMsgNum = 0;
static uint32 heartBitFailNum = 0;

static uint8 timerCount = 0;

static void nv_read_config(void)
{
  uint8 nv_buffer[12];
  osal_nv_item_init(NV_CONFIG_DATA, 12, NULL);
  osal_nv_read(NV_CONFIG_DATA, 0, 12, nv_buffer);
  memcpy(serialNumber, nv_buffer, 4);
  memcpy(&lastNvTime, nv_buffer + 4, 4);
  memcpy(&recoverMsgNum, nv_buffer + 8, 4);

  if (recoverMsgNum == 0xFFFFFFFF)
  {
     recoverMsgNum = 0;
  }

  if (lastNvTime == 0xFFFFFFFF)
  {
     lastNvTime = 0;
  }
}

static void nv_write_config(void)
{
  uint8 nv_buffer[12];
  uint32 curTime = lastNvTime + osal_GetSystemClock();
  memcpy(nv_buffer, serialNumber, 4);
  memcpy(nv_buffer + 4, &curTime, 4);
  memcpy(nv_buffer + 8, &recoverMsgNum, 4);

  osal_nv_item_init(NV_CONFIG_DATA, 12, NULL);
  osal_nv_write(NV_CONFIG_DATA, 0, 12, nv_buffer);
}

static void nv_reset_config(void)
{
  uint8 nv_buffer[12];
  uint32 curTime = 0;
  recoverMsgNum = 0;
  memset(serialNumber, 0, 4);
  memcpy(nv_buffer, serialNumber, 4);
  memcpy(nv_buffer + 4, &curTime, 4);
  memcpy(nv_buffer + 8, &recoverMsgNum, 4);

  osal_nv_item_init(NV_CONFIG_DATA, 12, NULL);
  osal_nv_write(NV_CONFIG_DATA, 0, 12, nv_buffer);
}

#define MSG_ELEM_LENGTH  (sizeof(uint32))
// write threee nv msg at one time
#define MAX_TIME  3
//For testing , we only store 12 msg at most
#define LARGEST_NV_TIME  32
static uint32 osTime[MAX_TIME];

static void nv_write_msg(void)
{
  if (LARGEST_NV_TIME < recoverMsgNum + nvMsgNum)
  {
     return;
  }

  osal_nv_item_init(
      NV_RECOVER_DATA,
      MSG_ELEM_LENGTH * (recoverMsgNum + nvMsgNum),
      NULL);

  osal_nv_write(
      NV_RECOVER_DATA,
      recoverMsgNum * MSG_ELEM_LENGTH,
      MSG_ELEM_LENGTH * nvMsgNum,
      osTime);

  recoverMsgNum += nvMsgNum;
  nvMsgNum = 0;

  nv_write_config();
}

static void nv_read_msg(uint32 msgIndex, uint32* curTime)
{
  osal_nv_item_init(NV_RECOVER_DATA, MSG_ELEM_LENGTH * msgIndex, NULL);
  osal_nv_read(NV_RECOVER_DATA, msgIndex * MSG_ELEM_LENGTH, MSG_ELEM_LENGTH, curTime);
}

/*********************************************************************
 * @fn      SampleApp_Init
 *
 * @brief   Initialization function for the Generic App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notificaiton ... ).
 *
 * @param   task_id - the ID assigned by OSAL.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */
void SampleApp_Init( uint8 task_id )
{
  SampleApp_TaskID = task_id;
  SampleApp_NwkState = DEV_INIT;
  SampleApp_TransID = 0;
  /*Init UART*/
  MT_UartInit();
  MT_UartRegisterTaskID(task_id);

  // Device hardware initialization can be added here or in main() (Zmain.c).
  // If the hardware is application specific - add it here.
  // If the hardware is other parts of the device add it in main().

 #if defined ( BUILD_ALL_DEVICES )
  // The "Demo" target is setup to have BUILD_ALL_DEVICES and HOLD_AUTO_START
  // We are looking at a jumper (defined in SampleAppHw.c) to be jumpered
  // together - if they are - we will start up a coordinator. Otherwise,
  // the device will start as a router.
  if ( readCoordinatorJumper() )
    zgDeviceLogicalType = ZG_DEVICETYPE_COORDINATOR;
  else
    zgDeviceLogicalType = ZG_DEVICETYPE_ROUTER;
#endif // BUILD_ALL_DEVICES

#if defined ( HOLD_AUTO_START )
  // HOLD_AUTO_START is a compile option that will surpress ZDApp
  //  from starting the device and wait for the application to
  //  start the device.
  ZDOInitDevice(0);
#endif

  // Setup for the periodic message's destination address
  // Broadcast to everyone
  SampleApp_Periodic_DstAddr.addrMode = (afAddrMode_t)AddrBroadcast;
  SampleApp_Periodic_DstAddr.endPoint = HEARTBEAT_ENDPOINT;
  SampleApp_Periodic_DstAddr.addr.shortAddr = 0xFFFF;

  // Fill out the endpoint description.
  SampleApp_epDesc.endPoint = HEARTBEAT_ENDPOINT;
  SampleApp_epDesc.task_id = &SampleApp_TaskID;
  SampleApp_epDesc.simpleDesc
            = (SimpleDescriptionFormat_t *)&SampleApp_SimpleDesc;
  SampleApp_epDesc.latencyReq = noLatencyReqs;

  // Register the endpoint description with the AF
  afRegister( &SampleApp_epDesc );

  // Register for all key events - This app will handle all key events
  RegisterForKeys( SampleApp_TaskID );

  nv_read_config();

  osal_start_timerEx( SampleApp_TaskID,
                      SAMPLEAPP_HEARTBEAT_MSG_EVT,
                      500 );
}

//extern uint8 get_uip_data;

/*********************************************************************
 * @fn      SampleApp_ProcessEvent
 *
 * @brief   Generic Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The OSAL assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  none
 */

uint16 SampleApp_ProcessEvent( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  (void)task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG )
  {
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
    while ( MSGpkt )
    {
      switch ( MSGpkt->hdr.event )
      {
        case CMD_SERIAL_MSG:
          SampleApp_SerialCMD((mtOSALSerialData_t *)MSGpkt);
          break;

        // Received when a key is pressed
        case KEY_CHANGE:
          SampleApp_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
          break;

        // Received when a messages is received (OTA) for this endpoint
        case AF_INCOMING_MSG_CMD:
          SampleApp_MessageMSGCB( MSGpkt );
#ifdef DEBUG_TRACE
#endif
          //HalLedBlink(HAL_LED_1, 2, 50, 200);
          break;

        // Received whenever the device changes state in the network
        case ZDO_STATE_CHANGE:
          SampleApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
          if (SampleApp_NwkState == DEV_END_DEVICE)
          {
              HalLedBlink(HAL_LED_1, 2, 50, 500);
              HalUARTWrite(0, EndDeviceStatus, strlen((char *)EndDeviceStatus));
              //Start sending the periodic message in a regular interval.
          }
          break;

        default:
          break;
      }

      // Release the memory
      osal_msg_deallocate( (uint8 *)MSGpkt );

      // Next - if one is available
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  // Send a message out - This event is generated by a timer
  //  (setup in SampleApp_Init()).
  if ( events & SAMPLEAPP_HEARTBEAT_MSG_EVT )
  {
    ++timerCount;
    
    SampleApp_RecoverHeartBeatMessage();
    if (timerCount == 10)
    {
        // Send the Heartbeat Message
        SampleApp_SendHeartBeatMessage();
        timerCount = 0;
    }
    
     // Setup to send heartbeat again in 10s
     osal_start_timerEx( SampleApp_TaskID, SAMPLEAPP_HEARTBEAT_MSG_EVT, SAMPLEAPP_TIMER_MSG_TIMEOUT);
    
    // return unprocessed events
    return (events ^ SAMPLEAPP_HEARTBEAT_MSG_EVT);
  }

  // Discard unknown events
  return 0;
}

/*********************************************************************
 * Event Generation Functions
 */
/*********************************************************************
 * @fn      SampleApp_HandleKeys
 *
 * @brief   Handles all key events for this device.
 *
 * @param   shift - true if in shift/alt.
 * @param   keys - bit field for key events. Valid entries:
 *                 HAL_KEY_SW_2
 *                 HAL_KEY_SW_1
 *
 * @return  none
 */

void SampleApp_HandleKeys( uint8 shift, uint8 keys )
{
  uint8 i = 0;

  (void)shift;  // Intentionally unreferenced parameter
  uint32 curTime = 0;
  if ( keys & HAL_KEY_SW_6 )
  {
    nv_read_config();
    HalUARTWrite(0, "xxxx", 5);
    HalUARTWrite(0, (uint8 *)&nvMsgNum, 4);
    nv_read_msg(i++, &curTime);
    HalUARTWrite(0, (uint8 *)&curTime, 4);
  }
}

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      SampleApp_MessageMSGCB
 *
 * @brief   Data message processor callback.  This function processes
 *          any incoming data - probably from other devices.  So, based
 *          on cluster ID, perform the intended action.
 *
 * @param   none
 *
 * @return  none
 */
void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{
  switch (pkt->clusterId)
  {
    case SAMPLEAPP_PERIODIC_CLUSTERID:
      //HalLedBlink(HAL_LED_1, 2, 50, 500);
      //HalUARTWrite(0, &pkt->cmd.Data[0], pkt->cmd.DataLength);
      break;

    case SAMPLEAPP_FLASH_CLUSTERID:
      break;
  }
}

void SampleApp_SerialCMD(mtOSALSerialData_t *cmdMsg)
{
  uint8 *uartMsg = cmdMsg->msg;
  //uint8 uartMsgLen = *(uartMsg);//The First Byte Stores the Length of Uart Message.

  //Set the SN through UART

  if(uartMsg[0] == 4)
  {
     memcpy(serialNumber, (uartMsg + 1), SN_LEN);
     nv_write_config();
  }

  if(uartMsg[0] == 1 && uartMsg[1] == '9')
  {
     nv_reset_config();
  }
}

void SampleApp_RecoverHeartBeatMessage(void)
{
   uint8 buf[SN_LEN + 4 + 4];
   uint32 msgTime;
   if (nvMsgNum == 0)
   {
       if (recoverMsgNum != 0)
       {
          nv_read_msg(recoverMsgNum - 1, &msgTime);
          memcpy(buf, serialNumber, SN_LEN);
          memcpy(buf + 4, &msgTime, 4);
          buf[8] = 1;
          buf[9] = 2;
          buf[10] = 3;
          buf[11] = 4;
          
          SampleApp_Periodic_DstAddr.endPoint = HEARTBEAT_ENDPOINT;
          if (AF_DataRequest(
              &SampleApp_Periodic_DstAddr,
              &SampleApp_epDesc,
              SAMPLEAPP_PERIODIC_CLUSTERID,
              SN_LEN + 4 + 4,
              buf,
              &SampleApp_TransID,
              AF_DISCV_ROUTE,
              AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
         {
            --recoverMsgNum;
         }
       }
   }
   else
   {
       msgTime = osTime[nvMsgNum - 1];
       memcpy(buf, serialNumber, SN_LEN);
       memcpy(buf + 4, &msgTime, 4);
       buf[8] = 11;
       buf[9] = 12;
       buf[10] = 13;
       buf[11] = 14;
       
       SampleApp_Periodic_DstAddr.endPoint = HEARTBEAT_ENDPOINT;
       if (AF_DataRequest(
              &SampleApp_Periodic_DstAddr,
              &SampleApp_epDesc,
              SAMPLEAPP_PERIODIC_CLUSTERID,
              SN_LEN + 4 + 4,
              buf,
              &SampleApp_TransID,
              AF_DISCV_ROUTE,
              AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
      {
         --nvMsgNum;
      }
   }
}

void SampleApp_SendHeartBeatMessage(void)
{
  uint32 curTime = lastNvTime + osal_GetSystemClock();
  uint8 buf[SN_LEN + 4];
  memcpy(buf, serialNumber, SN_LEN);
  memcpy(buf + 4, &curTime, 4);

  SampleApp_Periodic_DstAddr.endPoint = HEARTBEAT_ENDPOINT;
  if ( AF_DataRequest(
           &SampleApp_Periodic_DstAddr, &SampleApp_epDesc,
           SAMPLEAPP_PERIODIC_CLUSTERID,
           SN_LEN + 4,
           buf,
           &SampleApp_TransID,
           AF_DISCV_ROUTE,
           AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
  {
      heartBitFailNum = 0;
  }
  else
  {
      osTime[nvMsgNum] = curTime;
      ++nvMsgNum;
      ++heartBitFailNum;
  }

#ifdef DEBUG_TRACE
  ShowHeartBeatInfo(buf, buf + 4);
  ShowRssiInfo();
#endif

  if (nvMsgNum == MAX_TIME)
  {
     nv_write_msg();
  }

  if (heartBitFailNum == MAX_TIME)
  {
      if (nvMsgNum != 0)
      {
         nv_write_msg();
      }
      HAL_SYSTEM_RESET();
  }
}

/*********************************************************************
*********************************************************************/
#ifdef DEBUG_TRACE
static void ShowHeartBeatInfo(uint8 *serialNumber, uint8 *curTime)
{
  uint8 i = 0;
  uint8 reverseSerialNumber[SN_LEN] = {0};
  uint8 serialNumberStr[SN_LEN * 2 + 2] = {0};
  uint8 curIimeStr[TIME_LEN * 2 + 2] = {0};

  for (i = 0; i < SN_LEN; i++)
  {
    reverseSerialNumber[i] = serialNumber[SN_LEN - i - 1];
  }

  FormatDebugInfo(serialNumberStr, reverseSerialNumber, SN_LEN);
  serialNumberStr[SN_LEN * 2 + 1] = '\n';
  FormatDebugInfo(curIimeStr, curTime, TIME_LEN);
  curIimeStr[TIME_LEN * 2 + 1] = '\n';
  HalUARTWrite(0, SnHeadStr, strlen((char *)SnHeadStr));
  HalUARTWrite(0, serialNumberStr, SN_LEN * 2 + 2);
  HalUARTWrite(0, curTimeHeadStr, strlen((char *)curTimeHeadStr));
  HalUARTWrite(0, curIimeStr, TIME_LEN * 2 + 2);
}

static void FormatDebugInfo(uint8 *dst, const uint8 * src, uint8 len)
{
  uint8 i = 0;
  const uint8 *ptr = src + len - 1;

  for (i = 0; i < (len * 2); ptr--)
  {
    uint8 ch;
    ch = (*ptr >> 4) & 0x0F;
    dst[i++] = ch + (( ch < 10 ) ? '0' : '7');
    ch = *ptr & 0x0F;
    dst[i++] = ch + (( ch < 10 ) ? '0' : '7');
  }
}

/*Show Received Signal Strength Indication*/
static void ShowRssiInfo(void)
{
  int8 rssi = 0;
  uint8 str[8] = {0};

  /* read latest RSSI value */
  rssi = macRssi;

  if (rssi < 0)
  {
    str[0] = '-';
    rssi = 0 - rssi;
  }
  else
  {
    str[0] = '+';
  }
  str[1] = rssi/10 + '0';
  str[2] = rssi%10 + '0';

  memcpy(str + 3, "dbm\n", 4);

  HalUARTWrite(0, "rssi is:", 9);
  HalUARTWrite(0, str, 8);
}
#endif