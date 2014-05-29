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

#define FAIL_TIME_TO_RESTART (MAX_RECOVER_MSG_IN_MEN * 2)
//index 16
#define HEART_BIT_STATUS_INDEX (SN_LEN + NV_LEN)
//index 12
#define HEART_BIT_AD1_INDEX (SN_LEN + TIME_LEN)
#define HEART_BIT_MSG_LEN  (SN_LEN + NV_LEN + 1)

#define HEART_BIT_STATUS_REPAIR_MASK           0x8
#define HEART_BIT_STATUS_REALTIME_MASK         0x4
#define HEART_BIT_STATUS_POWER_SUPPLY_MASK     0x2
#define HEART_BIT_STATUS_BATTERY_CHARGING_MASK 0x1

#define SET_HEART_BIT_STATUS(sendToG1Data, mask, value) \
do \
{ \
   if (value) sendToG1Data[HEART_BIT_STATUS_INDEX] = sendToG1Data[HEART_BIT_STATUS_INDEX] | mask; \
   else sendToG1Data[HEART_BIT_STATUS_INDEX] = sendToG1Data[HEART_BIT_STATUS_INDEX] & ~mask; \
}while(0)


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

tRfData rfData[MAX_RECOVER_MSG_IN_MEN];

bool inNetwork = false;

static uint32 adcValueSum[2] = {0};
/*********************************************************************
 * LOCAL FUNCTIONS
 */
void CurrentDetectionT1_SerialCMD(mtOSALSerialData_t* cmdMsg);
bool CurrentDetectionT1_SendHeartBeatMessage(uint8* sendToG1Data);
void CurrentDetectionT1_RecoverHeartBeatMessage(uint8* sendToG1Data);
void CurrentDetectionT1_StoreHeartBeatMessage(uint8* sendToG1Data);
bool CurrentDetectionT1_HandleSendRepairMessage(uint8 * sendToG1Data);
void CurrentDetectionT1_SampleCurrentAdcValue(void);
void CurrentDetectionT1_SetAverageCurrentAdcValue(uint8* sendToG1Data, uint16 timerCount);
bool CurrentDetectionT1_CheckDelta(uint8* nvBuf);
void checkLedStatus(void);
void checkWorkingStatus(void);
void setSn(uint8 *uartMsg);
void resetNvConfig(uint8 *uartMsg);

static PFUNC cmdProcFuncs[] = {setSn, resetNvConfig};

static void swapUint32Bytes(uint8 * buf)
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
#ifdef DEBUG_TRACE
   //memset(serialNumber, 0, SN_LEN);
   //serialNumber[SN_LEN - 1] = 20;
#endif

   memcpy(serialNumber, aExtendedAddress, SN_LEN);
   //Feed WatchDog
   WDCTL = 0xa0;
   WDCTL = 0x50;

   osal_start_timerEx(CurrentDetectionT1_TaskID, DTCT_HEARTBEAT_MSG_EVT, 10);
   osal_start_timerEx(CurrentDetectionT1_TaskID, DTCT_LED_WTD_EVT, 10);
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
               //HalLedBlink(HAL_LED_1, 2, 50, 500);
#endif

            // Received whenever the device changes state in the network
            case ZDO_STATE_CHANGE:
               CurrentDetectionT1_NwkState = (devStates_t)(MSGpkt->hdr.status);
               if (CurrentDetectionT1_NwkState == DEV_END_DEVICE)
               {
                  HalLedSet(HAL_LED_2,  HAL_LED_MODE_ON);
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
      uint8 sendToG1Data[HEART_BIT_MSG_LEN];
      memcpy(sendToG1Data, serialNumber, SN_LEN);

      if (CurrentDetectionT1_HandleSendRepairMessage(sendToG1Data))
      {
         osal_start_timerEx(
            CurrentDetectionT1_TaskID,
            DTCT_HEARTBEAT_MSG_EVT,
            DTCT_TIMER_MSG_TIMEOUT);

         // The repairing status, no need to send ADC value any more.
         return (events ^ DTCT_HEARTBEAT_MSG_EVT);
      }

      CurrentDetectionT1_SampleCurrentAdcValue();
      
      ++timerCount;
      
      // send recovery message every 0.5s.
      if (timerCount == 100 || timerCount == 200 || timerCount == 300)
      {
         CurrentDetectionT1_RecoverHeartBeatMessage(sendToG1Data);
      }

      // send heart beat every 2s.
      else if (timerCount == 400)
      {
         CurrentDetectionT1_SetAverageCurrentAdcValue(sendToG1Data, timerCount);
         SET_HEART_BIT_STATUS(sendToG1Data , HEART_BIT_STATUS_POWER_SUPPLY_MASK, 0);
         SET_HEART_BIT_STATUS(sendToG1Data , HEART_BIT_STATUS_BATTERY_CHARGING_MASK, 1);
         
         if (inNetwork)
         {
            // Send the Heartbeat Message
            if (!CurrentDetectionT1_SendHeartBeatMessage(sendToG1Data))
            {
               ++heartBitFailNum;
               CurrentDetectionT1_StoreHeartBeatMessage(sendToG1Data);
            }
            else
            {
               heartBitFailNum = 0;
            }
         }
         else
         {
            ++heartBitFailNum;
            if(CurrentDetectionT1_CheckDelta(sendToG1Data))
            {
                CurrentDetectionT1_StoreHeartBeatMessage(sendToG1Data);
            }
         }

         if (heartBitFailNum == FAIL_TIME_TO_RESTART)
         {
            if (recoverMsgNumInMem != 0)
            {
               nv_write_msg();
            }

            HAL_SYSTEM_RESET();
         }
         timerCount = 0;
      }

      osal_start_timerEx(
            CurrentDetectionT1_TaskID,
            DTCT_HEARTBEAT_MSG_EVT,
            DTCT_TIMER_MSG_TIMEOUT);

      // return unprocessed events
      return (events ^ DTCT_HEARTBEAT_MSG_EVT);
   }

   if (events & DTCT_LED_WTD_EVT)
   {
      //Feed WatchDog
      WDCTL = 0xa0;
      WDCTL = 0x50;

      if (P2_0)
      {
        HalLedSet(HAL_LED_1,  HAL_LED_MODE_ON);
        HalLedSet(HAL_LED_2,  HAL_LED_MODE_OFF);
      }
      else
      {
	checkLedStatus();
      }

      osal_start_timerEx(
        CurrentDetectionT1_TaskID,
        DTCT_LED_WTD_EVT,
        DTCT_LED_WTD_EVT_TIMEOUT);

      return (events ^ DTCT_LED_WTD_EVT);
   }
   // Discard unknown events
   return 0;
}

void CurrentDetectionT1_StoreHeartBeatMessage(uint8* sendToG1Data)
{
   memcpy(rfData[recoverMsgNumInMem++].data, sendToG1Data + SN_LEN, NV_LEN);
   if (recoverMsgNumInMem == MAX_RECOVER_MSG_IN_MEN)
   {
      nv_write_msg();
   }
}

bool CurrentDetectionT1_CheckDelta(uint8* sendToG1Data)
{
  uint16 curAd1Value;
  uint16 lastAd1Value;
  //uint16 lastAd2Value;
  if (recoverMsgNumInMem == 0)
  {
     uint8 buf[NV_LEN];
     if (!nv_read_last_msg(buf))
     {
        return true;
     }
     
     lastAd1Value = (buf[TIME_LEN] << 8) + buf[TIME_LEN + 1];;
     //lastAd2Value = (buf[TIME_LEN + 2] << 8) + buf[TIME_LEN + 3];
  }
  else
  {
     uint8 * buf = rfData[recoverMsgNumInMem - 1].data;
     lastAd1Value = (buf[TIME_LEN] << 8) + buf[TIME_LEN + 1];
     //lastAd2Value = (buf[TIME_LEN + 2] << 8) + buf[TIME_LEN + 3];
  }
  
  curAd1Value = sendToG1Data[SN_LEN + TIME_LEN + 1];
  
  if ( (lastAd1Value > curAd1Value) && ((lastAd1Value - curAd1Value) > delta)) 
  {
      return true;
  }
  else if ( (lastAd1Value < curAd1Value) && ((curAd1Value - lastAd1Value) > delta)) 
  {
      return true;
  }

  return false;
}

void CurrentDetectionT1_SerialCMD(mtOSALSerialData_t *cmdMsg)
{
   uint8 *uartMsg = cmdMsg->msg;

   cmdProcFuncs[uartMsg[1]](uartMsg);
   return;
}

void CurrentDetectionT1_RecoverHeartBeatMessage(uint8* sendToG1Buf)
{
   if (!inNetwork)
   {
     return;
   }

   if (recoverMsgNumInMem == 0)
   {
      if (nv_read_last_msg(sendToG1Buf + SN_LEN))
      {
         SET_HEART_BIT_STATUS(sendToG1Buf , HEART_BIT_STATUS_REALTIME_MASK, 0);
         
         if (AF_DataRequest(
                  &CurrentDetectionT1_Periodic_DstAddr,
                  &CurrentDetectionT1_epDesc,
                  DTCT_PERIODIC_CLUSTERID,
                  HEART_BIT_MSG_LEN,
                  sendToG1Buf,
                  &CurrentDetectionT1_TransID,
                  AF_DISCV_ROUTE,
                  AF_DEFAULT_RADIUS ) == afStatus_SUCCESS)
         {
            if (--recoverMsgNumInNv == 0)
            {
               nv_write_config();
            }

            PrintRecover(sendToG1Buf, true);
            HalLedBlink(HAL_LED_2, 2, 20, 50);
         }
         else
         {
            PrintRecover(sendToG1Buf, false);
         }
      }
   }
   else
   {
      memcpy(sendToG1Buf + SN_LEN, rfData[recoverMsgNumInMem - 1].data, NV_LEN);
     SET_HEART_BIT_STATUS(sendToG1Buf , HEART_BIT_STATUS_REALTIME_MASK, 0);

      if (AF_DataRequest(
               &CurrentDetectionT1_Periodic_DstAddr,
               &CurrentDetectionT1_epDesc,
               DTCT_PERIODIC_CLUSTERID,
               HEART_BIT_MSG_LEN,
               sendToG1Buf,
               &CurrentDetectionT1_TransID,
               AF_DISCV_ROUTE,
               AF_DEFAULT_RADIUS ) == afStatus_SUCCESS)
      {
         --recoverMsgNumInMem;
         PrintRecover(sendToG1Buf, true);
         HalLedBlink(HAL_LED_2, 2, 20, 50);
      }
      else
      {
         PrintRecover(sendToG1Buf, false);
      }
   }
}

bool CurrentDetectionT1_SendHeartBeatMessage(uint8* sendToG1Data)
{
   SET_HEART_BIT_STATUS(sendToG1Data , HEART_BIT_STATUS_REALTIME_MASK, 1);
            
   ShowHeartBeatInfo(sendToG1Data);
   ShowRssiInfo();

   if (AF_DataRequest(
            &CurrentDetectionT1_Periodic_DstAddr,
            &CurrentDetectionT1_epDesc,
            DTCT_PERIODIC_CLUSTERID,
            HEART_BIT_MSG_LEN,
            sendToG1Data,
            &CurrentDetectionT1_TransID,
            AF_DISCV_ROUTE,
            AF_DEFAULT_RADIUS) == afStatus_SUCCESS)
   {
      HalLedBlink(HAL_LED_2, 2, 20, 50);
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
      HalLedBlink(HAL_LED_2, 4, 50, 190);
   }
   else
   {
      HalLedSet(HAL_LED_2,  HAL_LED_MODE_OFF);
   }

  return;
}

static bool CurrentDetectionT1_CheckNeedSendRepairMessage(void)
{
   static bool preP2_0 = false;
   
   bool sendRepair = false;

   //If P20 has a rising edge, we send a need repair message
   if ((P2_0 ^ preP2_0) && (P2_0))
   {
      sendRepair = true;
   }

   preP2_0 = P2_0;

   return sendRepair;
}

bool CurrentDetectionT1_HandleSendRepairMessage(uint8 * sendToG1Data)
{
   static bool needResendRepair = false;
   static uint32 repairTime;

   if (needResendRepair || CurrentDetectionT1_CheckNeedSendRepairMessage())
   {
      if (!needResendRepair)
      {
         repairTime = lastNvTime + osal_GetSystemClock();
      }
      memcpy(sendToG1Data + SN_LEN, &repairTime, sizeof(repairTime));
      swapUint32Bytes(sendToG1Data + SN_LEN);
      
      SET_HEART_BIT_STATUS(sendToG1Data , HEART_BIT_STATUS_REPAIR_MASK, 1);
           
      if (AF_DataRequest(
            &CurrentDetectionT1_Periodic_DstAddr,
            &CurrentDetectionT1_epDesc,
            DTCT_PERIODIC_CLUSTERID,
            HEART_BIT_MSG_LEN,
            sendToG1Data,
            &CurrentDetectionT1_TransID,
            AF_DISCV_ROUTE,
            AF_DEFAULT_RADIUS) == afStatus_SUCCESS)
      {
         HalLedBlink(HAL_LED_2, 2, 50, 50);
         needResendRepair = false;
      }
      else
      {
         needResendRepair = true;
      }
   }

    return (P2_0);
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

void CurrentDetectionT1_SampleCurrentAdcValue()
{
  //AC_IN0: P0_7
  //AC-IN1: P0_6
  adcValueSum[0] += HalAdcRead(HAL_ADC_CHN_AIN7, HAL_ADC_RESOLUTION_12);
  adcValueSum[1] += HalAdcRead(HAL_ADC_CHN_AIN6, HAL_ADC_RESOLUTION_12);

  return;
}

void CurrentDetectionT1_SetAverageCurrentAdcValue(uint8* sendToG1Data, uint16 timerCount)
{
   uint16 adcValue = (adcValueSum[0]/timerCount);

   uint32 curTime = lastNvTime + osal_GetSystemClock();
   memcpy(sendToG1Data + SN_LEN, &curTime, sizeof(curTime));
   swapUint32Bytes(sendToG1Data + SN_LEN);
   
   sendToG1Data[HEART_BIT_AD1_INDEX] = adcValue >> 8;
   sendToG1Data[HEART_BIT_AD1_INDEX + 1] = adcValue & 0x00FF;
   
   adcValue = (adcValueSum[1]/timerCount);
   sendToG1Data[HEART_BIT_AD1_INDEX + 2] = adcValue >> 8;
   sendToG1Data[HEART_BIT_AD1_INDEX + 3] = adcValue & 0x00FF;

   adcValueSum[0] = 0;
   adcValueSum[1] = 0;

   return;
}
