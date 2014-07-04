#include "mem.h"
#include "string.h"

typedef struct tRfData
{
  uint8 data[NV_LEN];
}tRfData;

static tRfData rfData[MAX_RECOVER_MSG_IN_MEN];

static uint8 _beginIndex = 0;
static uint8 _endIndex = 0;

uint8  nv_mem_number(void)
{
  if (_endIndex >= _beginIndex)
  {
     return _endIndex - _beginIndex;
  }
  return _endIndex + MAX_RECOVER_MSG_IN_MEN - _beginIndex;
}

uint8* nv_mem_read(void)
{
   if (nv_mem_number() == 0)
   {
      return NULL;
   }
   return rfData[_beginIndex].data;
}

uint8* nv_mem_read_last(void)
{
   if (nv_mem_number() == 0)
   {
      return NULL;
   }
   
   if (_endIndex == 0)
   {
      return rfData[MAX_RECOVER_MSG_IN_MEN - 1].data;
   }
   else
   {
      return rfData[_endIndex - 1].data;
   }
}

void nv_mem_next(void)
{
   if (++_beginIndex == MAX_RECOVER_MSG_IN_MEN)
   {
      _beginIndex = 0;
   }
}

bool nv_mem_write(uint8* data)
{
   if (nv_mem_full())
   {
      return FALSE;
   }
   
   memcpy(rfData[_endIndex].data, data, NV_LEN);
   
   if (++_endIndex == MAX_RECOVER_MSG_IN_MEN)
   {
      _endIndex = 0;
   }
   return TRUE;
}

bool nv_mem_full(void)
{
   return (nv_mem_number() == MAX_RECOVER_MSG_IN_MEN - 1);
}
