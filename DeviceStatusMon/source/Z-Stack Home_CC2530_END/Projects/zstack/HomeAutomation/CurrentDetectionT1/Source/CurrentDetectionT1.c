// Things you need to do:
// 1. Rename the project SampleLight to a meaningful project.
// 2. Remove the Sample(SampleApp, SampleDesc, ...) key words in the project.

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
#include "hal_adc.h"

#include "nv.h"
#include "printDebug.h"

#include "CurrentDetectionT1.h"
#include "CurrentDetectionT1Hw.h"

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
const cId_t CurrentDetectionT1_ClusterList[DTCT_MAX_CLUSTERS] =
{
   DTCT_PERIODIC_CLUSTERID
};

const SimpleDescriptionFormat_t CurrentDetectionT1_SimpleDesc =
{
   HEARTBEAT_ENDPOINT,              //  int Endpoint;
   DTCT_PROFID,                //  uint16 AppProfId[2];
   DTCT_DEVICEID,              //  uint16 AppDeviceId[2];
   DTCT_DEVICE_VERSION,        //  int    AppDevVer:4;
   DTCT_FLAGS,                 //  int    AppFlags:4;
   0,                               //  uint8  AppNumInClusters;
   NULL,                            //  uint8* pAppInClusterList;
   DTCT_MAX_CLUSTERS,          //  uint8  AppNumOutClusters;
   (cId_t *)CurrentDetectionT1_ClusterList   //  uint8* pAppOutClusterList;
};

// This is the Endpoint/Interface description. It is defined here, but
// filled-in in CurrentDetectionT1_Init(). Another way to go would be to fill
// in the structure here and make it a "const" (in code space).  The
// way it's defined in this sample app it is define in RAM.
endPointDesc_t CurrentDetectionT1_epDesc;

// Task ID for internal task/event processing
uint8 CurrentDetectionT1_TaskID;

// This variable will be received when CurrentDetectionT1_Init() is called.
devStates_t CurrentDetectionT1_NwkState;

// This is the unique message ID (counter)
uint8 CurrentDetectionT1_TransID;

afAddrType_t CurrentDetectionT1_Periodic_DstAddr;

tRfData rfData[NV_NUM_STORE];

bool inNetwork = false;

uint32 adcValueSum[2] = {0};
/*********************************************************************
 * LOCAL FUNCTIONS
 */
void CurrentDetectionT1_SerialCMD(mtOSALSerialData_t* cmdMsg);
bool CurrentDetectionT1_SendHeartBeatMessage(uint8* buf);
void CurrentDetectionT1_RecoverHeartBeatMessage(void);
void CurrentDetectionT1_StoreHeartBeatMessage(uint8* buf);
void CurrentDetectionT1_SendNeedRepairMessage(uint8* buf);
void CurrentDetectionT1_SampleCurrentAdcValue(void);
void CurrentDetectionT1_SetAverageCurrentAdcValue(uint8* buf, uint16 timerCount);
void checkLedStatus(void);
void checkWorkingStatus(void);
void setSn(uint8 *uartMsg);
void resetNvConfig(uint8 *uartMsg);
void getP2_0Status(void);

static PFUNC cmdProcFuncs[] = {setSn, resetNvConfig};

static void revertUint32Bytes(uint8 * buf)
{
   uint8 temp = buf[0];
   buf[0] = buf[3];
   buf[3] = temp;

   temp = buf[1];
   buf[1] = buf[2];
   buf[2] = temp;
}

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      CurrentDetectionT1_Init
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

static void IO_Init(void)
{
  P1DIR |= 0x0F;     //Set P1_0, P1_3 as Output

  P1_0 = 0;
  P1_3 = 1;

  //Turn off Led
  P1_1 = 1;
  P1_2 = 1;

  P0SEL |= 0xC0;   //Set P0_6 and P0_7 Peripheral function(ADC)
  APCFG |= 0xC0;   //Set P0_6 and P0_7 AnalogI/O enabled
}

void CurrentDetectionT1_Init(uint8 task_id)
{
   IO_Init();

   CurrentDetectionT1_TaskID = task_id;
   CurrentDetectionT1_NwkState = DEV_INIT;
   CurrentDetectionT1_TransID = 0;

   MT_UartInit();
   MT_UartRegisterTaskID(task_id);

   // Device hardware initialization can be added here or in main() (Zmain.c).
   // If the hardware is application specific - add it here.
   // If the hardware is other parts of the device add it in main().

#if defined ( BUILD_ALL_DEVICES )
   // The "Demo" target is setup to have BUILD_ALL_DEVICES and HOLD_AUTO_START
   // We are looking at a jumper (defined in CurrentDetectionT1Hw.c) to be jumpered
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
   CurrentDetectionT1_Periodic_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
   CurrentDetectionT1_Periodic_DstAddr.endPoint = HEARTBEAT_ENDPOINT;
   CurrentDetectionT1_Periodic_DstAddr.addr.shortAddr = 0;

   // Fill out the endpoint description.
   CurrentDetectionT1_epDesc.endPoint = HEARTBEAT_ENDPOINT;
   CurrentDetectionT1_epDesc.task_id = &CurrentDetectionT1_TaskID;
   CurrentDetectionT1_epDesc.simpleDesc
      = (SimpleDescriptionFormat_t *)&CurrentDetectionT1_SimpleDesc;
   CurrentDetectionT1_epDesc.latencyReq = noLatencyReqs;

   // Register the endpoint description with the AF
   afRegister(&CurrentDetectionT1_epDesc);

   // Register for all key events - This app will handle all key events
   RegisterForKeys(CurrentDetectionT1_TaskID);

   nv_read_config();

   HalAdcInit();
   HalAdcSetReference(HAL_ADC_REF_AVDD);

   memcpy(serialNumber, aExtendedAddress, SN_LEN);

   osal_start_timerEx(CurrentDetectionT1_TaskID, DTCT_HEARTBEAT_MSG_EVT, 500);
}

/*********************************************************************
 * @fn      CurrentDetectionT1_ProcessEvent
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

uint16 CurrentDetectionT1_ProcessEvent(uint8 task_id, uint16 events)
{
   static uint16 timerCount = 0;
   static uint32 heartBitFailNum = 0;

   afIncomingMSGPacket_t* MSGpkt;

   (void)task_id;  // Intentionally unreferenced parameter

   if (events & SYS_EVENT_MSG)
   {
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(CurrentDetectionT1_TaskID);
      while (MSGpkt)
      {
         switch (MSGpkt->hdr.event)
         {
            case CMD_SERIAL_MSG:
               CurrentDetectionT1_SerialCMD((mtOSALSerialData_t *)MSGpkt);
               break;

#ifdef DEBUG_TRACE
            case AF_INCOMING_MSG_CMD:
               HalLedBlink(HAL_LED_1, 2, 50, 500);
#endif

            // Received whenever the device changes state in the network
            case ZDO_STATE_CHANGE:
               CurrentDetectionT1_NwkState = (devStates_t)(MSGpkt->hdr.status);
               if (CurrentDetectionT1_NwkState == DEV_END_DEVICE)
               {
                  //HalLedBlink(HAL_LED_1, 2, 50, 500);
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
         MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(CurrentDetectionT1_TaskID);
      }

      // return unprocessed events
      return (events ^ SYS_EVENT_MSG);
   }

   // Send a message out - This event is generated by a timer
   //  (setup in CurrentDetectionT1_Init()).
   if (events & DTCT_HEARTBEAT_MSG_EVT)
   {
      uint8 buf[SN_LEN + NV_LEN];
      uint32 curTime = lastNvTime + osal_GetSystemClock();
      memcpy(buf, serialNumber, SN_LEN);
      memcpy(buf + SN_LEN, &curTime, sizeof(curTime));
      revertUint32Bytes(buf + SN_LEN);

      CurrentDetectionT1_SendNeedRepairMessage(buf);

      if (P2_0)
      {
        HalLedBlink(HAL_LED_1, 2, 50, 500);
        HalLedSet(HAL_LED_2,  HAL_LED_MODE_OFF);

        osal_start_timerEx(
            CurrentDetectionT1_TaskID,
            DTCT_HEARTBEAT_MSG_EVT,
            DTCT_TIMER_MSG_TIMEOUT*200);

        // return unprocessed events
        return (events ^ DTCT_HEARTBEAT_MSG_EVT);
      }

      CurrentDetectionT1_SampleCurrentAdcValue();

      // send heart bit every 2s.
      if (++timerCount == 400)
      {
         CurrentDetectionT1_SetAverageCurrentAdcValue(buf + 12, timerCount);

         checkLedStatus();
         CurrentDetectionT1_RecoverHeartBeatMessage();

         if (inNetwork)
         {
            // Send the Heartbeat Message
            if (!CurrentDetectionT1_SendHeartBeatMessage(buf))
            {
               ++heartBitFailNum;
               CurrentDetectionT1_StoreHeartBeatMessage(buf + SN_LEN);
            }
            else
            {
               heartBitFailNum = 0;
            }
         }
         else
         {
            ++heartBitFailNum;
            CurrentDetectionT1_StoreHeartBeatMessage(buf + SN_LEN);
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
            CurrentDetectionT1_TaskID,
            DTCT_HEARTBEAT_MSG_EVT,
            DTCT_TIMER_MSG_TIMEOUT);

      // return unprocessed events
      return (events ^ DTCT_HEARTBEAT_MSG_EVT);
   }

   // Discard unknown events
   return 0;
}

void CurrentDetectionT1_StoreHeartBeatMessage(uint8* nvBuf)
{
   memcpy(rfData[nvMsgNum++].data, nvBuf, NV_LEN);
   if (nvMsgNum == NV_NUM_STORE)
   {
      nv_write_msg();
   }
}

void CurrentDetectionT1_SerialCMD(mtOSALSerialData_t *cmdMsg)
{
   uint8 *uartMsg = cmdMsg->msg;

   cmdProcFuncs[uartMsg[1]](uartMsg);
   return;
}

void CurrentDetectionT1_RecoverHeartBeatMessage(void)
{
   uint8 buf[SN_LEN + NV_LEN];

   if (!inNetwork)
   {
     return;
   }

   if (nvMsgNum == 0)
   {
      if (recoverMsgNum != 0)
      {
         nv_read_msg(recoverMsgNum - 1, buf + SN_LEN);
         memcpy(buf, serialNumber, SN_LEN);

         if (AF_DataRequest(
                  &CurrentDetectionT1_Periodic_DstAddr,
                  &CurrentDetectionT1_epDesc,
                  DTCT_PERIODIC_CLUSTERID,
                  SN_LEN + NV_LEN,
                  buf,
                  &CurrentDetectionT1_TransID,
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
      memcpy(buf, serialNumber, SN_LEN);
      memcpy(buf + SN_LEN, rfData[nvMsgNum - 1].data, NV_LEN);

      if (AF_DataRequest(
               &CurrentDetectionT1_Periodic_DstAddr,
               &CurrentDetectionT1_epDesc,
               DTCT_PERIODIC_CLUSTERID,
               SN_LEN + NV_LEN,
               buf,
               &CurrentDetectionT1_TransID,
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

bool CurrentDetectionT1_SendHeartBeatMessage(uint8* buf)
{
   ShowHeartBeatInfo(buf);
   ShowRssiInfo();

   if (AF_DataRequest(
            &CurrentDetectionT1_Periodic_DstAddr,
            &CurrentDetectionT1_epDesc,
            DTCT_PERIODIC_CLUSTERID,
            SN_LEN + NV_LEN,
            buf,
            &CurrentDetectionT1_TransID,
            AF_DISCV_ROUTE,
            AF_DEFAULT_RADIUS) == afStatus_SUCCESS)
   {
      return true;
   }

   return false;
}

void checkLedStatus(void)
{
   //P2_0: NeedRepair Switch, when it is 1: Need Repair, blink Red Led and turn off Green Led. 0: Don't Need Repair
   //P1_2: Red Led  LED1
   //P1_1: Green Led, Green Led blink when device is not in network LED2

   HalLedSet(HAL_LED_1,  HAL_LED_MODE_OFF);
   if (!inNetwork)
   {
      HalLedBlink(HAL_LED_2, 2, 50, 500);
   }
   else
   {
      HalLedSet(HAL_LED_2,  HAL_LED_MODE_ON);;
   }

  return;
}

void CurrentDetectionT1_SendNeedRepairMessage(uint8* buf)
{
   static bool repairReported = false;
   static bool preP2_0 = false;
   uint32 needRepairFlag = 0x78563412;

   //If P20 has a rising edge, we send a need repair message
   if ((P2_0^preP2_0) && (P2_0))
   {
      repairReported = false;
   }

   preP2_0 = P2_0;

   if (repairReported)
   {
      return;
   }

   memcpy(buf + 12, &needRepairFlag, sizeof(needRepairFlag));

   if (AF_DataRequest(
            &CurrentDetectionT1_Periodic_DstAddr,
            &CurrentDetectionT1_epDesc,
            DTCT_PERIODIC_CLUSTERID,
            SN_LEN + NV_LEN,
            buf,
            &CurrentDetectionT1_TransID,
            AF_DISCV_ROUTE,
            AF_DEFAULT_RADIUS) == afStatus_SUCCESS)
   {
      repairReported = true;
   }

   return;
}

static void setSn(uint8 *uartMsg)
{
   HalUARTWrite(0, "Set Sn:\n", 9);
   return;
}

static void resetNvConfig(uint8 *uartMsg)
{
   HalUARTWrite(0, "resetNv:\n", 10);
   return;
}

void getP2_0Status(void)
{
  uint8 str[] = {'P', '2', '_', '0', '=', '0', '\n'};
  str[5] = P2_0 + '0';

  HalUARTWrite(0, str, 7);

  return;
}

void CurrentDetectionT1_SampleCurrentAdcValue()
{
  adcValueSum[0] += HalAdcRead(HAL_ADC_CHN_AIN6, HAL_ADC_RESOLUTION_12);
  adcValueSum[1] += HalAdcRead(HAL_ADC_CHN_AIN7, HAL_ADC_RESOLUTION_12);

  return;
}

void CurrentDetectionT1_SetAverageCurrentAdcValue(uint8* buf, uint16 timerCount)
{
  uint16 adcValue = 0;

  adcValue = (adcValueSum[0]/timerCount);
  buf[0] = adcValue >> 8;
  buf[1] = adcValue & 0x00FF;
  adcValue = (adcValueSum[1]/timerCount);;
  buf[2] = adcValue >> 8;
  buf[3] = adcValue & 0x00FF;

  adcValueSum[0] = 0;
  adcValueSum[1] = 0;

  return;
}
