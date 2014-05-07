/*********************************************************************
 * INCLUDES
 */
#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"
#include "OnBoard.h"
#include "hal_led.h"

#include "uip_tcpapp.h"
#include "livelist.h"
#include "queue.h"
#include "nv.h"

#include "SampleApp.h"
#include "SampleAppHw.h"

#include "string.h"
#include "Watchdog.h"

/*********************************************************************
 * GLOBAL VARIABLES
 */

// This list should be filled with Application specific Cluster IDs.
const cId_t SampleApp_ClusterList[SAMPLEAPP_MAX_CLUSTERS] =
{
   SAMPLEAPP_PERIODIC_CLUSTERID,
};

const SimpleDescriptionFormat_t SampleApp_HeartBeatSimpleDesc =
{
   HEARTBEAT_ENDPOINT,
   SAMPLEAPP_PROFID,
   SAMPLEAPP_DEVICEID,
   SAMPLEAPP_DEVICE_VERSION,
   SAMPLEAPP_FLAGS,
   SAMPLEAPP_MAX_CLUSTERS,
   (cId_t *)SampleApp_ClusterList,
   0,
   NULL
};

endPointDesc_t SampleApp_HeartBeatEpDesc;

LockFreeQueue queue;

// Task ID for internal task/event processing
uint8 SampleApp_TaskID;

// This variable will be received when
// SampleApp_Init() is called.
devStates_t SampleApp_NwkState;

// This is the unique message ID (counter)
uint8 SampleApp_TransID;

uint8 CorSendGap = 1;
/*********************************************************************
 * @fn SampleApp_Init
 *
 * @brief Initialization function for the Generic App Task.
 * This is called during initialization and should contain
 * any application specific initialization (ie. hardware
 * initialization/setup, table initialization, power up
 * notificaiton ... ).
 *
 * @param task_id - the ID assigned by OSAL. This ID should be
 * used to send messages and set timers.
 *
 * @return none
 */
void SampleApp_Init(uint8 task_id)
{
   SampleApp_TaskID = task_id;
   SampleApp_NwkState = DEV_INIT;
   SampleApp_TransID = 0;

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
   // from starting the device and wait for the application to
   // start the device.
   ZDOInitDevice(0);
#endif

   SampleApp_HeartBeatEpDesc.endPoint = HEARTBEAT_ENDPOINT;
   SampleApp_HeartBeatEpDesc.task_id = &SampleApp_TaskID;
   SampleApp_HeartBeatEpDesc.simpleDesc
      = (SimpleDescriptionFormat_t *)&SampleApp_HeartBeatSimpleDesc;
   SampleApp_HeartBeatEpDesc.latencyReq = noLatencyReqs;
   afRegister(&SampleApp_HeartBeatEpDesc);

   initLiveList();
   InitFreeQueque(&queue);

   WatchdogClear();

   osal_start_timerEx(SampleApp_TaskID, SAMPLEAPP_WTD_EVT, SAMPLEAPP_WTD_TIMEOUT);
}

/*********************************************************************
 * @fn SampleApp_ProcessEvent
 *
 * @brief Generic Application Task event processor. This function
 * is called to process all events for the task. Events
 * include timers, messages and any other user defined events.
 *
 * @param task_id - The OSAL assigned task ID.
 * @param events - events to process. This is a bit map and can
 * contain more than one event.
 *
 * @return none
 */
uint16 SampleApp_ProcessEvent( uint8 task_id, uint16 events )
{
   static int timerTick = 0;

   afIncomingMSGPacket_t *MSGpkt;
   (void)task_id; // Intentionally unreferenced parameter

   if ( events & SYS_EVENT_MSG )
   {
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
      while (MSGpkt)
      {
         switch (MSGpkt->hdr.event)
         {
               // Received when a messages is received (OTA) for this endpoint
            case AF_INCOMING_MSG_CMD:
               if ((MSGpkt->endPoint == HEARTBEAT_ENDPOINT)
                && (ELEMENT_SIZE == MSGpkt->cmd.DataLength))
               {
                  HalLedBlink(HAL_LED_1, 2, 50, 50);
                  int emptyIndex = -1;

                  int index = findLiveList(MSGpkt->cmd.Data, &emptyIndex);
                  if (index == -1)
                  {
                     if(!insertLiveList(emptyIndex, MSGpkt->cmd.Data))
                     {
                        break;
                     }
                  }
                  else
                  {
                     setLiveStatus(index, true);
                     setSendGapCount(index);
                  }

                  index = (index == -1)?(emptyIndex):(index);

                  if (CorSendGap == getSendGapCount(index))
                  {
                    if (IsFreeQueueFull(&queue))
                    {
                       // If the queue is full, write current msg to the nv.
                       nv_write_msg(MSGpkt->cmd.Data);
                    }
                    else
                    {
                       // If the queue is not full, push the msg to the queue.
                       FreeQueuePush(&queue, MSGpkt->cmd.Data);
                    }

                    clearSendGapCount(index);
                  }
               }
               break;

               // Received whenever the device changes state in the network
            case ZDO_STATE_CHANGE:
               SampleApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
               if (SampleApp_NwkState == DEV_ZB_COORD)
               {
                  HalLedBlink(HAL_LED_2, 2, 50, 500);
                  osal_start_timerEx(
                     SampleApp_TaskID,
                     SAMPLEAPP_PERIODIC_EVT,
                     SAMPLEAPP_PERIODIC_TIMEOUT);
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
   // (setup in SampleApp_Init()).
   if (events & SAMPLEAPP_PERIODIC_EVT)
   {
      osal_start_timerEx(
         SampleApp_TaskID,
         SAMPLEAPP_PERIODIC_EVT,
         SAMPLEAPP_PERIODIC_TIMEOUT);

      // Check live list for every 10s
      if (++timerTick == 10)
      {
         timerTick = 0;
         resetLiveList();
      }

      // return unprocessed events
      return (events ^ SAMPLEAPP_PERIODIC_EVT);
   }

   if (events & SAMPLEAPP_WTD_EVT)
   {
     WatchdogClear();

     osal_start_timerEx(SampleApp_TaskID, SAMPLEAPP_WTD_EVT, SAMPLEAPP_WTD_TIMEOUT);

     WatchdogEnable( WATCHDOG_INTERVAL_32768 );
     return (events ^ SAMPLEAPP_WTD_EVT);
   }

   // Discard unknown events
   return 0;
}
