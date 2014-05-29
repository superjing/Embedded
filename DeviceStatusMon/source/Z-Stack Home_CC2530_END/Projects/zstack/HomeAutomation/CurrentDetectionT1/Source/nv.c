#include "OSAL_Nv.h"
#include "OSAL.h"
#include "string.h"

#include "nv.h"

// The largest number of nvs that can be stored.
#define LARGEST_NV_TIME  180

#define NV_CONFIG_DATA  0x1000
#define NV_RECOVER_DATA 0x1020
#define NV_ELEM_LENGTH  (sizeof(uint32))

extern tRfData rfData[MAX_RECOVER_MSG_IN_MEN];

uint32 lastNvTime = 0;
uint32 recoverMsgNumInMem = 0;
uint32 recoverMsgNumInNv = 0;
uint8  serialNumber[SN_LEN] = {0};

//todo send from tcp
uint32 delta = DELTA_DEFUALT;

void nv_read_config(void)
{
   uint8 nv_buffer[NV_CONFIG_LEN];

   osal_nv_item_init(NV_CONFIG_DATA, NV_CONFIG_LEN, NULL);
   osal_nv_read(NV_CONFIG_DATA, 0, NV_CONFIG_LEN, nv_buffer);
   memcpy(serialNumber, nv_buffer, SN_LEN);
   memcpy(&lastNvTime, nv_buffer + SN_LEN, TIME_LEN);
   memcpy(&recoverMsgNumInNv, nv_buffer + SN_LEN + TIME_LEN, MSG_NUM_LEN);
   memcpy(&delta, nv_buffer + SN_LEN + TIME_LEN + MSG_NUM_LEN, DELTA_LEN);
   
   if (recoverMsgNumInNv == 0xFFFFFFFF)
   {
      recoverMsgNumInNv = 0;
   }

   if (lastNvTime == 0xFFFFFFFF)
   {
      lastNvTime = 0;
   }
   
   if (delta == 0xFFFFFFFF)
   {
      delta = DELTA_DEFUALT;
   }
}

void nv_write_config(void)
{
   uint8 nv_buffer[NV_CONFIG_LEN];
   uint32 curTime = lastNvTime + osal_GetSystemClock();

   memcpy(nv_buffer, serialNumber, SN_LEN);
   memcpy(nv_buffer + SN_LEN, &curTime, TIME_LEN);
   memcpy(nv_buffer + SN_LEN + TIME_LEN, &recoverMsgNumInNv, MSG_NUM_LEN);
   memcpy(nv_buffer + SN_LEN + TIME_LEN + MSG_NUM_LEN, &delta, DELTA_LEN);

   osal_nv_item_init(NV_CONFIG_DATA, NV_CONFIG_LEN, NULL);
   osal_nv_write(NV_CONFIG_DATA, 0, NV_CONFIG_LEN, nv_buffer);
}

void nv_reset_config(void)
{
   uint8 nv_buffer[NV_CONFIG_LEN];
   uint32 curTime = 0;
   delta = DELTA_DEFUALT;
   recoverMsgNumInNv = 0;
   memset(serialNumber, 0, SN_LEN);
   memcpy(nv_buffer, serialNumber, SN_LEN);
   memcpy(nv_buffer + SN_LEN, &curTime, TIME_LEN);
   memcpy(nv_buffer + SN_LEN + TIME_LEN, &recoverMsgNumInNv, MSG_NUM_LEN);
   memcpy(nv_buffer + SN_LEN + TIME_LEN + MSG_NUM_LEN, &delta, DELTA_LEN);  

   osal_nv_item_init(NV_CONFIG_DATA, NV_CONFIG_LEN, NULL);
   osal_nv_write(NV_CONFIG_DATA, 0, NV_CONFIG_LEN, nv_buffer);
}

void nv_write_msg(void)
{
   if (LARGEST_NV_TIME < recoverMsgNumInNv + recoverMsgNumInMem)
   {
      return;
   }

   for(uint32 i =0; i != recoverMsgNumInMem; ++i)
   {
      osal_nv_item_init(
            NV_RECOVER_DATA + recoverMsgNumInNv + i,
            NV_LEN,
            NULL);

      osal_nv_write(
            NV_RECOVER_DATA + recoverMsgNumInNv + i,
            0,
            NV_LEN,
            rfData[i].data);
   }

   recoverMsgNumInNv += recoverMsgNumInMem;
   recoverMsgNumInMem = 0;

   nv_write_config();
}


bool nv_read_last_msg(uint8* nvBuf)
{
   if (recoverMsgNumInNv == 0)
   {
      return false;
   }
   osal_nv_item_init(NV_RECOVER_DATA + recoverMsgNumInNv - 1, NV_LEN, NULL);
   osal_nv_read(NV_RECOVER_DATA + recoverMsgNumInNv - 1, 0, NV_LEN, nvBuf);
   return true;
}