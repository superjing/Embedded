#include "livelist.h"
#include "comdef.h"
#include "OSAL_Timers.h"
#include <string.h>

static tLiveList liveList;

void initLiveList(void)
{
   memset(&liveList, 0, sizeof(tLiveList));
}

static bool insertLiveList(int index, uint8 * sn)
{
   if (index >= MAX_END_NUM)
   {
      return false;
   }

   memcpy(liveList.LiveElem[index].sn, sn , SN_LEN);
   
   ++liveList.liveCount;
   return true;
}

bool setLiveStatus(uint8 * sn, afAddrType_t * srcAddr)
{
   int insertIndex = -1;
   int i = 0;
   
   for (; i < MAX_END_NUM; ++i)
   {
      if (liveList.LiveElem[i].liveStatus)
      {
         if (0 == memcmp(sn, liveList.LiveElem[i].sn, SN_LEN))
         {
            break;
         }
      }
      
      if (insertIndex == -1)
      {
         insertIndex = i;
      }
   }
   
   if (i == MAX_END_NUM)
   {
      if (!insertLiveList(insertIndex, sn))
      {
         return false;
      }
      i = insertIndex;
   }
   
   if (!liveList.LiveElem[i].liveStatus)
   {
      memcpy(&(liveList.LiveElem[i].srcAddr), srcAddr, sizeof(afAddrType_t));
      liveList.LiveElem[i].liveStatus = true;
   }
   liveList.LiveElem[i].timestamp = osal_GetSystemClock();
   
   return true;
}

void refreshLiveList(uint32 currentTime, uint32 timeout)
{
   for (uint8 i = 0; i < MAX_END_NUM; ++i)
   {
      if (!liveList.LiveElem[i].liveStatus)
      {
         continue;
      }
      
      if (currentTime - liveList.LiveElem[i].timestamp > timeout)
      {
         //rest the element in the live list if timeout
         memset(&liveList.LiveElem[i], 0, sizeof(tLiveElem));
         --liveList.liveCount;
      }
   }
}

afAddrType_t * findSrcAddr(uint8 * sn)
{
   int i = 0;
   
   for (; i < MAX_END_NUM; ++i)
   {
      if (liveList.LiveElem[i].liveStatus 
          && (memcmp(sn, liveList.LiveElem[i].sn, SN_LEN) == 0))
      {
          return &(liveList.LiveElem[i].srcAddr);
      }
   }
   return NULL;
}