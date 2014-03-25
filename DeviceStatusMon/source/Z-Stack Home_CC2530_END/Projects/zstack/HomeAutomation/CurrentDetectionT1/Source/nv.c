#include "OSAL_Nv.h"
#include "OSAL.h"
#include "string.h"

#include "nv.h"

// The largest number of nvs that can be stored.
#define LARGEST_NV_TIME  180

#define NV_CONFIG_DATA  0x1000
#define NV_RECOVER_DATA 0x1020
#define NV_ELEM_LENGTH  (sizeof(uint32))

extern tRfData rfData[NV_NUM_STORE];

uint32 lastNvTime = 0;
uint32 nvMsgNum = 0;
uint32 recoverMsgNum = 0;
uint8  serialNumber[SN_LEN] = {0};

void nv_read_config(void)
{
   uint8 nv_buffer[NV_CONFIG_LEN];

   osal_nv_item_init(NV_CONFIG_DATA, NV_CONFIG_LEN, NULL);
   osal_nv_read(NV_CONFIG_DATA, 0, NV_CONFIG_LEN, nv_buffer);
   memcpy(serialNumber, nv_buffer, SN_LEN);
   memcpy(&lastNvTime, nv_buffer + SN_LEN, sizeof(lastNvTime));
   memcpy(&recoverMsgNum, nv_buffer + SN_LEN + sizeof(lastNvTime), sizeof(recoverMsgNum));

   if (recoverMsgNum == 0xFFFFFFFF)
   {
      recoverMsgNum = 0;
   }

   if (lastNvTime == 0xFFFFFFFF)
   {
      lastNvTime = 0;
   }
}

void nv_write_config(void)
{
   uint8 nv_buffer[NV_CONFIG_LEN];
   uint32 curTime = lastNvTime + osal_GetSystemClock();

   memcpy(nv_buffer, serialNumber, SN_LEN);
   memcpy(nv_buffer + SN_LEN, &curTime, sizeof(curTime));
   memcpy(nv_buffer + SN_LEN + sizeof(curTime), &recoverMsgNum, sizeof(recoverMsgNum));

   osal_nv_item_init(NV_CONFIG_DATA, NV_CONFIG_LEN, NULL);
   osal_nv_write(NV_CONFIG_DATA, 0, NV_CONFIG_LEN, nv_buffer);
}

void nv_reset_config(void)
{
   uint8 nv_buffer[NV_CONFIG_LEN];
   uint32 curTime = 0;
   recoverMsgNum = 0;
   memset(serialNumber, 0, SN_LEN);
   memcpy(nv_buffer, serialNumber, SN_LEN);
   memcpy(nv_buffer + SN_LEN, &curTime, sizeof(curTime));
   memcpy(nv_buffer + SN_LEN + sizeof(curTime), &recoverMsgNum, sizeof(recoverMsgNum));

   osal_nv_item_init(NV_CONFIG_DATA, NV_CONFIG_LEN, NULL);
   osal_nv_write(NV_CONFIG_DATA, 0, NV_CONFIG_LEN, nv_buffer);
}

void nv_write_msg(void)
{
   if (LARGEST_NV_TIME < recoverMsgNum + nvMsgNum)
   {
      return;
   }

   for(uint32 i =0; i != nvMsgNum; ++i)
   {
      osal_nv_item_init(
            NV_RECOVER_DATA + recoverMsgNum + i,
            NV_LEN,
            NULL);

      osal_nv_write(
            NV_RECOVER_DATA + recoverMsgNum + i,
            0,
            NV_LEN,
            rfData[i].data);
   }

   recoverMsgNum += nvMsgNum;
   nvMsgNum = 0;

   nv_write_config();
}

void nv_read_msg(uint32 msgIndex, uint8* nvBuf)
{
   osal_nv_item_init(NV_RECOVER_DATA + msgIndex, NV_LEN, NULL);
   osal_nv_read(NV_RECOVER_DATA + msgIndex, 0, NV_LEN, nvBuf);
}