#ifndef CURRENTDETECTIONG1_H
#define CURRENTDETECTIONG1_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "ZComDef.h"

#define HEARTBEAT_ENDPOINT           10

#define CTDT_PROFID             0x0F08
#define CTDT_DEVICEID           0x0001
#define CTDT_DEVICE_VERSION     0
#define CTDT_FLAGS              0

#define CTDT_MAX_CLUSTERS       1
#define CTDT_PERIODIC_CLUSTERID 1

// Send Message Timeout
#define CTDT_PERIODIC_TIMEOUT   1000
#define CTDT_PERIODIC_EVT       0x0001

extern uint8 CurrentDetectionG1_TaskID;
extern void CurrentDetectionG1_Init(uint8 task_id);
extern UINT16 CurrentDetectionG1_ProcessEvent( uint8 task_id, uint16 events );

#ifdef __cplusplus
}
#endif

#endif /* CURRENTDETECTIONG1_H */
