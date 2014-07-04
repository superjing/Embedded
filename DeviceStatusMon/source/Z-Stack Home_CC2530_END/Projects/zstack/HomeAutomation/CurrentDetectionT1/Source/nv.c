#include "OSAL_Nv.h"
#include "OSAL.h"

#include "nv.h"
#include "mem.h"
#include "string.h"

// The largest number of nvs that can be stored.
#define LARGEST_NV_TIME  180

#define NV_CONFIG_DATA  0x1000
#define NV_RECOVER_DATA 0x1020

uint32 lastNvTime = 0;
uint8  serialNumber[SN_LEN] = {0};

static uint8 _beginIndex = 0;
static uint8 _endIndex = 0;

uint16 delta = DELTA_DEFAULT;
uint16 heartbitRate = RATE_DEFAULT;

void nv_read_config(void)
{
   uint8 nv_buffer[NV_CONFIG_LEN];

   osal_nv_item_init(NV_CONFIG_DATA, NV_CONFIG_LEN, NULL);
   osal_nv_read(NV_CONFIG_DATA, 0, NV_CONFIG_LEN, nv_buffer);
   memcpy(serialNumber, nv_buffer, SN_LEN);
   memcpy(&lastNvTime, nv_buffer + SN_LEN, TIME_LEN);
   memcpy(&_beginIndex, nv_buffer + SN_LEN + TIME_LEN, MSG_NUM_LEN);
   memcpy(&_endIndex, nv_buffer + SN_LEN + TIME_LEN + MSG_NUM_LEN, MSG_NUM_LEN);
   memcpy(&delta, nv_buffer + SN_LEN + TIME_LEN + MSG_NUM_LEN + MSG_NUM_LEN, DELTA_LEN);
   memcpy(&heartbitRate, nv_buffer + SN_LEN + TIME_LEN + MSG_NUM_LEN + MSG_NUM_LEN + DELTA_LEN, RATE_LEN);

   if (_beginIndex == 0xFF)
   {
      _beginIndex = 0;
   }
   
   if (_endIndex == 0xFF)
   {
      _endIndex = 0;
   }

   if (lastNvTime == 0xFFFFFFFF)
   {
      lastNvTime = 0;
   }

   if (delta == 0xFFFF)
   {
      delta = DELTA_DEFAULT;
   }

   if (heartbitRate == 0xFFFF)
   {
      heartbitRate = RATE_DEFAULT;
   }
}

void nv_write_config(void)
{
   uint8 nv_buffer[NV_CONFIG_LEN];
   uint32 curTime = lastNvTime + osal_GetSystemClock();

   memcpy(nv_buffer, serialNumber, SN_LEN);
   memcpy(nv_buffer + SN_LEN, &curTime, TIME_LEN);
   memcpy(nv_buffer + SN_LEN + TIME_LEN, &_beginIndex, MSG_NUM_LEN);
   memcpy(nv_buffer + SN_LEN + TIME_LEN + MSG_NUM_LEN, &_endIndex, MSG_NUM_LEN);
   memcpy(nv_buffer + SN_LEN + TIME_LEN + MSG_NUM_LEN + MSG_NUM_LEN, &delta, DELTA_LEN);
   memcpy(nv_buffer + SN_LEN + TIME_LEN + MSG_NUM_LEN + MSG_NUM_LEN + DELTA_LEN, &heartbitRate, RATE_LEN);

   osal_nv_item_init(NV_CONFIG_DATA, NV_CONFIG_LEN, NULL);
   osal_nv_write(NV_CONFIG_DATA, 0, NV_CONFIG_LEN, nv_buffer);
}

void nv_reset_config(void)
{
   uint8 nv_buffer[NV_CONFIG_LEN];
   uint32 curTime = 0;
   delta = DELTA_DEFAULT;
   heartbitRate = RATE_DEFAULT;
   _beginIndex = 0;
   _endIndex = 0;
   memset(serialNumber, 0, SN_LEN);
   memcpy(nv_buffer, serialNumber, SN_LEN);
   memcpy(nv_buffer + SN_LEN, &curTime, TIME_LEN);
   memcpy(nv_buffer + SN_LEN + TIME_LEN, &_beginIndex, MSG_NUM_LEN);
   memcpy(nv_buffer + SN_LEN + TIME_LEN + MSG_NUM_LEN, &_endIndex, MSG_NUM_LEN);
   memcpy(nv_buffer + SN_LEN + TIME_LEN + MSG_NUM_LEN + MSG_NUM_LEN, &delta, DELTA_LEN);
   memcpy(nv_buffer + SN_LEN + TIME_LEN + MSG_NUM_LEN + MSG_NUM_LEN + DELTA_LEN, &heartbitRate, RATE_LEN);

   osal_nv_item_init(NV_CONFIG_DATA, NV_CONFIG_LEN, NULL);
   osal_nv_write(NV_CONFIG_DATA, 0, NV_CONFIG_LEN, nv_buffer);
}

void nv_next(void)
{
   if (++_beginIndex == LARGEST_NV_TIME)
   {
      _beginIndex = 0;
   }  
}

uint8 nv_number(void)
{
   if (_endIndex >= _beginIndex)
   {
      return _endIndex - _beginIndex;
   }
   return LARGEST_NV_TIME + _endIndex - _beginIndex;
}

void nv_write_msg(void)
{
   uint8  recoverMsgNumInMem = nv_mem_number();
   uint8  recoverMsgNumInNv = nv_number();
   
   if (LARGEST_NV_TIME <= recoverMsgNumInNv + recoverMsgNumInMem)
   {
      return;
   }

   for (uint32 i = 0; i != recoverMsgNumInMem; ++i)
   {  
      osal_nv_item_init(
            NV_RECOVER_DATA + _endIndex,
            NV_LEN,
            NULL);

      osal_nv_write(
            NV_RECOVER_DATA + _endIndex,
            0,
            NV_LEN,
            nv_mem_read());
      
      nv_mem_next();
     
      if (++_endIndex == LARGEST_NV_TIME)
      {
         _endIndex = 0;
      }
   }
   
   nv_write_config();
}

bool nv_read_last_msg(uint8* nvBuf)
{
   if (nv_number() == 0)
   {
      return false;
   }
   
   if (_endIndex == 0)
   {
      osal_nv_item_init(NV_RECOVER_DATA + LARGEST_NV_TIME - 1, NV_LEN, NULL);
      osal_nv_read(NV_RECOVER_DATA + LARGEST_NV_TIME - 1, 0, NV_LEN, nvBuf);
   }
   else
   {
      osal_nv_item_init(NV_RECOVER_DATA + _endIndex - 1, NV_LEN, NULL);
      osal_nv_read(NV_RECOVER_DATA + _endIndex - 1, 0, NV_LEN, nvBuf);  
   }
   return true;
}

bool nv_read_msg(uint8* nvBuf)
{
   if (nv_number() == 0)
   {
      return false;
   }
   osal_nv_item_init(NV_RECOVER_DATA + _beginIndex, NV_LEN, NULL);
   osal_nv_read(NV_RECOVER_DATA + _beginIndex, 0, NV_LEN, nvBuf);

   return true;
}