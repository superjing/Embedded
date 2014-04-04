#include "livelist.h"
#include "comdef.h"
#include <string.h>

static tLiveList liveList;

void initLiveList( void )
{
   memset(&liveList, 0, sizeof(tLiveList));
}

int findLiveList(uint8 * sn, int * emptyIndex)
{
   int i = 0;

   *emptyIndex = -1;

   for (; i < MAX_END_NUM; i++)
   {
      if (!liveList.LiveElem[i].liveInUse)
      {
         if (*emptyIndex == -1)
         {
            *emptyIndex = i;
            continue;
         }
      }

      if (0 == memcmp(sn, liveList.LiveElem[i].sn, SN_LEN))
      {
         return i;
      }
   }

   return -1;
}

void setLiveStatus(int index, bool alive)
{
   liveList.LiveElem[index].liveStatus = alive;
}

bool insertLiveList(int index, uint8 * sn)
{
   if (index >= MAX_END_NUM)
   {
      return false;
   }
   memcpy(liveList.LiveElem[index].sn, sn , SN_LEN);
   setLiveStatus(index, true);
   liveList.LiveElem[index].liveInUse = true;
   ++liveList.liveCount;
   return true;
}

void resetLiveList(void)
{
   uint8 i = 0;

   for (; i < MAX_END_NUM; i++)
   {
      if (liveList.LiveElem[i].liveInUse && !liveList.LiveElem[i].liveStatus)
      {
         --liveList.liveCount;
         liveList.LiveElem[i].liveInUse = false;
      }

      liveList.LiveElem[i].liveStatus = false;
   }

}
