#ifndef LIVE_LIST_H

#include "hal_types.h"

#define SN_LEN        8
#define MAX_END_NUM   32

typedef struct tLiveElem
{
  uint8 sn[SN_LEN];
  uint8 sendGapCount;
  uint32 timestamp;
  bool liveStatus;
}tLiveElem;

typedef struct tLiveList
{
   tLiveElem LiveElem[MAX_END_NUM];
   uint8 liveCount;
}tLiveList;

void  initLiveList(void);
void  resetLiveList(uint32 currentTime, uint32 timeout);
uint8 * setLiveStatus(uint8 * sn);
#endif
