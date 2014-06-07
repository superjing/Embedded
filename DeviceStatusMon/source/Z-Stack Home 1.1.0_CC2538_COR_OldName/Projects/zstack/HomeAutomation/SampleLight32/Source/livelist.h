#ifndef LIVE_LIST_H

#include "hal_types.h"
#include "AF.h"

#define SN_LEN        8
#define MAX_END_NUM   32

typedef struct tLiveElem
{
  uint8 sn[SN_LEN];
  uint32 timestamp;
  bool liveStatus;
  afAddrType_t srcAddr;
}tLiveElem;

typedef struct tLiveList
{
   tLiveElem LiveElem[MAX_END_NUM];
   uint8 liveCount;
}tLiveList;

void initLiveList(void);
void refreshLiveList(uint32 currentTime, uint32 timeout);
bool setLiveStatus(uint8 * sn, afAddrType_t * srcAddr);
afAddrType_t * findSrcAddr(uint8 * sn);
#endif
