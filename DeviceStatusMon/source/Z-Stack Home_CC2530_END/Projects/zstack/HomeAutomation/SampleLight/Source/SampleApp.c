// Things you need to do:
// 1. Rename the project SampleLight to a meaningful project.
// 2. Remove the Sample(SampleApp, SampleDesc, ...) key words in the project.
// 3. Change the rf_msg_length from 12 to 10, the last two bytes are ADC data,
//    now you can set these two bytes to some special value (recover 0xCC, heartbeat 0xEE).
// 4. Change the nv things stored into NV. Currently we only store 4 bytes time,
//    now we also need to store both 4 bytes time and 2 bytes ADC value. Also in
//    ShowHeartBeatInfo function, print the ADC value.
// 6. Change uart input command(reset, set sn) with special header.

/*********************************************************************
 * INCLUDES
 */
#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"
#include "OnBoard.h"

#include "MT_Uart.h"
#include "MT.h"

#include "hal_led.h"

#include "nv.h"
#include "printDebug.h"

#include "SampleApp.h"
#include "SampleAppHw.h"

#include "string.h"

/*********************************************************************
 * MACROS
 */

// If the data cannot be sent in 1 minute, we need to restart the stack.
#define FAIL_TIEM_TO_RESTART (NV_NUM_STORE * 2)

/*********************************************************************
 * CONSTANTS
 */
static uint8 EndDeviceStatus[]  = "Connect as EndDevice\n";

// This list should be filled with Application specific Cluster IDs.
const cId_t SampleApp_ClusterList[SAMPLEAPP_MAX_CLUSTERS] =
{
   SAMPLEAPP_PERIODIC_CLUSTERID
};

const SimpleDescriptionFormat_t SampleApp_SimpleDesc =
{
   HEARTBEAT_ENDPOINT,              //  int Endpoint;
   SAMPLEAPP_PROFID,                //  uint16 AppProfId[2];
   SAMPLEAPP_DEVICEID,              //  uint16 AppDeviceId[2];
   SAMPLEAPP_DEVICE_VERSION,        //  int    AppDevVer:4;
   SAMPLEAPP_FLAGS,                 //  int    AppFlags:4;
   0,                               //  uint8  AppNumInClusters;
   NULL,                            //  uint8* pAppInClusterList;
   SAMPLEAPP_MAX_CLUSTERS,          //  uint8  AppNumOutClusters;
   (cId_t *)SampleApp_ClusterList   //  uint8* pAppOutClusterList;
};

// This is the Endpoint/Interface description. It is defined here, but
// filled-in in SampleApp_Init(). Another way to go would be to fill
// in the structure here and make it a "const" (in code space).  The
// way it's defined in this sample app it is define in RAM.
endPointDesc_t SampleApp_epDesc;

// Task ID for internal task/event processing
uint8 SampleApp_TaskID;

// This variable will be received when SampleApp_Init() is called.
devStates_t SampleApp_NwkState;

// This is the unique message ID (counter)
uint8 SampleApp_TransID;

afAddrType_t SampleApp_Periodic_DstAddr;

uint32 osTime[NV_NUM_STORE];

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void SampleApp_SerialCMD(mtOSALSerialData_t* cmdMsg);
bool SampleApp_SendHeartBeatMessage(uint32 time);
void SampleApp_RecoverHeartBeatMessage(void);
void SampleApp_StoreHeartBeatMessage(uint32 time);

static void convertUint2Bytes(uint8 *dstBuf, uint32 value)
{
   dstBuf[0] = (value >> 24) & 0xFF;
   dstBuf[1] = (value >> 16) & 0xFF;
   dstBuf[2] = (value >> 8) & 0xFF;
   dstBuf[3] = value & 0xFF;
}

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

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
 *          used to send messages and set timers.
 *
 * @return  none
 */
void SampleApp_Init(uint8 task_id)
{
   SampleApp_TaskID = task_id;
   SampleApp_NwkState = DEV_INIT;
   SampleApp_TransID = 0;

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
   if (readCoordinatorJumper())
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
   SampleApp_Periodic_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
   SampleApp_Periodic_DstAddr.endPoint = HEARTBEAT_ENDPOINT;
   SampleApp_Periodic_DstAddr.addr.shortAddr = 0;

   // Fill out the endpoint description.
   SampleApp_epDesc.endPoint = HEARTBEAT_ENDPOINT;
   SampleApp_epDesc.task_id = &SampleApp_TaskID;
   SampleApp_epDesc.simpleDesc
      = (SimpleDescriptionFormat_t *)&SampleApp_SimpleDesc;
   SampleApp_epDesc.latencyReq = noLatencyReqs;

   // Register the endpoint description with the AF
   afRegister(&SampleApp_epDesc);

   // Register for all key events - This app will handle all key events
   RegisterForKeys(SampleApp_TaskID);

   nv_read_config();

   osal_start_timerEx(SampleApp_TaskID, SAMPLEAPP_HEARTBEAT_MSG_EVT, 500);
}

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

uint16 SampleApp_ProcessEvent(uint8 task_id, uint16 events)
{
   static bool inNetwork = false;
   static uint8 timerCount = 0;
   static uint32 heartBitFailNum = 0;

   afIncomingMSGPacket_t* MSGpkt;

   (void)task_id;  // Intentionally unreferenced parameter

   if (events & SYS_EVENT_MSG)
   {
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(SampleApp_TaskID);
      while (MSGpkt)
      {
         switch (MSGpkt->hdr.event)
         {
            case CMD_SERIAL_MSG:
               SampleApp_SerialCMD((mtOSALSerialData_t *)MSGpkt);
               break;

            // Received whenever the device changes state in the network
            case ZDO_STATE_CHANGE:
               SampleApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
               if (SampleApp_NwkState == DEV_END_DEVICE)
               {
                  HalLedBlink(HAL_LED_1, 2, 50, 500);
                  HalUARTWrite(0, EndDeviceStatus, strlen((char *)EndDeviceStatus));
                  inNetwork = true;
               }
               break;

            default:
               break;
         }

         // Release the memory
         osal_msg_deallocate((uint8 *)MSGpkt);

         // Next - if one is available
         MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(SampleApp_TaskID);
      }

      // return unprocessed events
      return (events ^ SYS_EVENT_MSG);
   }

   // Send a message out - This event is generated by a timer
   //  (setup in SampleApp_Init()).
   if (events & SAMPLEAPP_HEARTBEAT_MSG_EVT)
   {
      if (inNetwork)
      {
         SampleApp_RecoverHeartBeatMessage();
      }

      // send heart bit every 10s.
      if (++timerCount == (SAMPLEAPP_TIMER_MSG_TIMEOUT/1000 * 10))
      {
         uint32 curTime = lastNvTime + osal_GetSystemClock();
         if (inNetwork)
         {
            // Send the Heartbeat Message
            if (!SampleApp_SendHeartBeatMessage(curTime))
            {
               ++heartBitFailNum;
               SampleApp_StoreHeartBeatMessage(curTime);
            }
            else
            {
               heartBitFailNum = 0;
            }
         }
         else
         {
            ++heartBitFailNum;
            SampleApp_StoreHeartBeatMessage(curTime);
         }

         if (heartBitFailNum == FAIL_TIEM_TO_RESTART)
         {
            if (nvMsgNum != 0)
            {
               nv_write_msg();
            }

            HAL_SYSTEM_RESET();
         }
         timerCount = 0;
      }

      // Setup to send heartbeat again in 10s
      osal_start_timerEx(
            SampleApp_TaskID,
            SAMPLEAPP_HEARTBEAT_MSG_EVT,
            SAMPLEAPP_TIMER_MSG_TIMEOUT);

      // return unprocessed events
      return (events ^ SAMPLEAPP_HEARTBEAT_MSG_EVT);
   }

   // Discard unknown events
   return 0;
}

void SampleApp_StoreHeartBeatMessage(uint32 time)
{
   osTime[nvMsgNum++] = time;
   if (nvMsgNum == NV_NUM_STORE)
   {
      nv_write_msg();
   }
}

void SampleApp_SerialCMD(mtOSALSerialData_t *cmdMsg)
{
   uint8 *uartMsg = cmdMsg->msg;

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
   uint8 buf[12];
   uint32 msgTime;

   if (nvMsgNum == 0)
   {
      if (recoverMsgNum != 0)
      {
         nv_read_msg(recoverMsgNum - 1, &msgTime);
         memcpy(buf, serialNumber, 4);
         convertUint2Bytes(buf + 4, msgTime);
         buf[8]  = '1';
         buf[9]  = '2';
         buf[10] = '3';
         buf[11] = '4';

         if (AF_DataRequest(
                  &SampleApp_Periodic_DstAddr,
                  &SampleApp_epDesc,
                  SAMPLEAPP_PERIODIC_CLUSTERID,
                  12,
                  buf,
                  &SampleApp_TransID,
                  AF_DISCV_ROUTE,
                  AF_DEFAULT_RADIUS ) == afStatus_SUCCESS)
         {
            if (--recoverMsgNum == 0)
            {
               nv_write_config();
            }

            PrintRecover(buf, true);
         }
         else
         {
            PrintRecover(buf, false);
         }
      }
   }
   else
   {
      msgTime = osTime[nvMsgNum - 1];
      memcpy(buf, serialNumber, 4);
      convertUint2Bytes(buf + 4, msgTime);
      buf[8]  = 'A';
      buf[9]  = 'B';
      buf[10] = 'C';
      buf[11] = 'D';

      if (AF_DataRequest(
               &SampleApp_Periodic_DstAddr,
               &SampleApp_epDesc,
               SAMPLEAPP_PERIODIC_CLUSTERID,
               12,
               buf,
               &SampleApp_TransID,
               AF_DISCV_ROUTE,
               AF_DEFAULT_RADIUS ) == afStatus_SUCCESS)
      {
         --nvMsgNum;
         PrintRecover(buf, true);
      }
      else
      {
         PrintRecover(buf, false);
      }
   }
}

bool SampleApp_SendHeartBeatMessage(uint32 curTime)
{
   uint8 buf[12];
   memcpy(buf, serialNumber, 4);
   convertUint2Bytes(buf + 4, curTime);
   buf[8]  = '8';
   buf[9]  = '9';
   buf[10] = 'X';
   buf[11] = 'Y';

   ShowHeartBeatInfo(buf, buf + 4);
   ShowRssiInfo();

   if (AF_DataRequest(
            &SampleApp_Periodic_DstAddr,
            &SampleApp_epDesc,
            SAMPLEAPP_PERIODIC_CLUSTERID,
            12,
            buf,
            &SampleApp_TransID,
            AF_DISCV_ROUTE,
            AF_DEFAULT_RADIUS) == afStatus_SUCCESS)
   {
      return true;
   }

   return false;
}
