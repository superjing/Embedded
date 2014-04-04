#ifndef SAMPLEAPP_H
#define SAMPLEAPP_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "ZComDef.h"
      
#define HEARTBEAT_ENDPOINT           10
  
#define SAMPLEAPP_PROFID             0x0F08
#define SAMPLEAPP_DEVICEID           0x0001
#define SAMPLEAPP_DEVICE_VERSION     0
#define SAMPLEAPP_FLAGS              0

#define SAMPLEAPP_MAX_CLUSTERS       1
#define SAMPLEAPP_PERIODIC_CLUSTERID 1

// Send Message Timeout
#define SAMPLEAPP_PERIODIC_TIMEOUT   1000
#define SAMPLEAPP_PERIODIC_EVT       0x0001
  
extern uint8 SampleApp_TaskID;
extern void SampleApp_Init(uint8 task_id);
extern UINT16 SampleApp_ProcessEvent( uint8 task_id, uint16 events );

#ifdef __cplusplus
}
#endif

#endif /* SAMPLEAPP_H */
