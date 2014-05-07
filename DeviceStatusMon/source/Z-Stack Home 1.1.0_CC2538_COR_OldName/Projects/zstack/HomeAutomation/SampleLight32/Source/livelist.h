#ifndef LIVE_LIST_H

#include "hal_types.h"

#define SN_LEN        8
#define MAX_END_NUM   32

typedef struct tLiveElem
{
  uint8 sn[SN_LEN];
  uint8 sendGapCount;
  bool liveStatus;
  bool liveInUse;
}tLiveElem;

typedef struct tLiveList
{
   tLiveElem LiveElem[MAX_END_NUM];
   uint8 liveCount;
}tLiveList;

void initLiveList(void);
int findLiveList(uint8* sn, int* emptyIndex);
void setLiveStatus(int index, bool alive);
bool insertLiveList(int index, uint8 * sn);
void resetLiveList(void);
void setSendGapCount(int index);
uint8 getSendGapCount(int index);
void clearSendGapCount(int index);

#endif
