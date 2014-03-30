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

#include "CurrentDetectionG1.h"
#include "CurrentDetectionG1Hw.h"

#include "string.h"

/*********************************************************************
 * GLOBAL VARIABLES
 */

// This list should be filled with Application specific Cluster IDs.
const cId_t CurrentDetectionG1_ClusterList[CTDT_MAX_CLUSTERS] =
{
   CTDT_PERIODIC_CLUSTERID,
};

const SimpleDescriptionFormat_t CurrentDetectionG1_HeartBeatSimpleDesc =
{
   HEARTBEAT_ENDPOINT,
   CTDT_PROFID,
   CTDT_DEVICEID,
   CTDT_DEVICE_VERSION,
   CTDT_FLAGS,
   CTDT_MAX_CLUSTERS,
   (cId_t *)CurrentDetectionG1_ClusterList,
   0,
   NULL
};

endPointDesc_t CurrentDetectionG1_HeartBeatEpDesc;

LockFreeQueue queue;

// Task ID for internal task/event processing
uint8 CurrentDetectionG1_TaskID;

// This variable will be received when
// CurrentDetectionG1_Init() is called.
devStates_t CurrentDetectionG1_NwkState;

// This is the unique message ID (counter)
uint8 CurrentDetectionG1_TransID;

uint8 LedFlag = 0;

#ifdef DEBUG_TRACE
afAddrType_t CurrentDetectionG1_Periodic_DstAddr;
bool CurrentDetectionG1_SendMessage(void);
#endif
/*********************************************************************
 * @fn CurrentDetectionG1_Init
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
void CurrentDetectionG1_Init(uint8 task_id)
{
   CurrentDetectionG1_TaskID = task_id;
   CurrentDetectionG1_NwkState = DEV_INIT;
   CurrentDetectionG1_TransID = 0;

   // Device hardware initialization can be added here or in main() (Zmain.c).
   // If the hardware is application specific - add it here.
   // If the hardware is other parts of the device add it in main().

#if defined ( BUILD_ALL_DEVICES )
   // The "Demo" target is setup to have BUILD_ALL_DEVICES and HOLD_AUTO_START
   // We are looking at a jumper (defined in CurrentDetectionG1Hw.c) to be jumpered
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

#ifdef DEBUG_TRACE
   CurrentDetectionG1_Periodic_DstAddr.addrMode = (afAddrMode_t)AddrBroadcast;
   CurrentDetectionG1_Periodic_DstAddr.endPoint = HEARTBEAT_ENDPOINT;
   CurrentDetectionG1_Periodic_DstAddr.addr.shortAddr = 0xFFFF;
#endif

   CurrentDetectionG1_HeartBeatEpDesc.endPoint = HEARTBEAT_ENDPOINT;
   CurrentDetectionG1_HeartBeatEpDesc.task_id = &CurrentDetectionG1_TaskID;
   CurrentDetectionG1_HeartBeatEpDesc.simpleDesc
      = (SimpleDescriptionFormat_t *)&CurrentDetectionG1_HeartBeatSimpleDesc;
   CurrentDetectionG1_HeartBeatEpDesc.latencyReq = noLatencyReqs;
   afRegister(&CurrentDetectionG1_HeartBeatEpDesc);

   initLiveList();
   InitFreeQueque(&queue);

}

/*********************************************************************
 * @fn CurrentDetectionG1_ProcessEvent
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
uint16 CurrentDetectionG1_ProcessEvent( uint8 task_id, uint16 events )
{
   static int timerTick = 0;

   afIncomingMSGPacket_t *MSGpkt;
   (void)task_id; // Intentionally unreferenced parameter

   if ( events & SYS_EVENT_MSG )
   {
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( CurrentDetectionG1_TaskID );
      while (MSGpkt)
      {
         switch (MSGpkt->hdr.event)
         {
               // Received when a messages is received (OTA) for this endpoint
            case AF_INCOMING_MSG_CMD:
               if ((MSGpkt->endPoint == HEARTBEAT_ENDPOINT)
                && (ELEMENT_SIZE == MSGpkt->cmd.DataLength))
               {
                  int emptyIndex = -1;
                  int index = findLiveList(MSGpkt->cmd.Data, &emptyIndex);
                  HalLedBlink(HAL_LED_2, 2, 50, 500);

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
                  }

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
               }
               break;

               // Received whenever the device changes state in the network
            case ZDO_STATE_CHANGE:
               CurrentDetectionG1_NwkState = (devStates_t)(MSGpkt->hdr.status);
               if (CurrentDetectionG1_NwkState == DEV_ZB_COORD)
               {
                  // Turn on Green LED after CC2538 in network as coord.
                  HalLedSet(HAL_LED_1, HAL_LED_MODE_ON);
                  osal_start_timerEx(
                     CurrentDetectionG1_TaskID,
                     CTDT_PERIODIC_EVT,
                     CTDT_PERIODIC_TIMEOUT);
               }
               break;

            default:
               break;
         }

         // Release the memory
         osal_msg_deallocate( (uint8 *)MSGpkt );

         // Next - if one is available
         MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( CurrentDetectionG1_TaskID );
      }

      // return unprocessed events
      return (events ^ SYS_EVENT_MSG);
   }

   // Send a message out - This event is generated by a timer
   // (setup in CurrentDetectionG1_Init()).
   if (events & CTDT_PERIODIC_EVT)
   {
      osal_start_timerEx(
         CurrentDetectionG1_TaskID,
         CTDT_PERIODIC_EVT,
         CTDT_PERIODIC_TIMEOUT);

      // Check live list for every 10s
      if (++timerTick == 10)
      {
         timerTick = 0;
         resetLiveList();
      }

      // return unprocessed events
      return (events ^ CTDT_PERIODIC_EVT);
   }
   // Discard unknown events
   return 0;
}


bool CurrentDetectionG1_SendMessage(void)
{
   uint8 buf[10] = {0};
   if (AF_DataRequest(
            &CurrentDetectionG1_Periodic_DstAddr,
            &CurrentDetectionG1_HeartBeatEpDesc,
            1,
            10,
            buf,
            &CurrentDetectionG1_TransID,
            AF_DISCV_ROUTE,
            AF_DEFAULT_RADIUS) == afStatus_SUCCESS)
   {
      return true;
   }

   return false;
}