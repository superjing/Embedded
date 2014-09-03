#include "Queue.h"
#include "OSAL_Nv.h"
#include "uip_tcpapp.h"
#include "nv.h"

#define NV_RECOVER_DATA 0x1020
#define NV_IP_ADDR      0x1000

//Record 32(devices) * 10(s) * 12 = 2 minutes * 32 devices msg
#define LARGEST_NV_TIME (MAX_ELEMENT_NUM * 12)

extern LockFreeQueue queue;

static uint32 recoverMsgNum = 0;

void nv_write_msg(uint8 * rfMsg)
{
   if (LARGEST_NV_TIME <= recoverMsgNum)
   {
      return;
   }

   osal_nv_item_init(
         NV_RECOVER_DATA + recoverMsgNum,
         ELEMENT_SIZE,
         NULL);

   osal_nv_write(
         NV_RECOVER_DATA + recoverMsgNum,
         0,
         ELEMENT_SIZE,
         rfMsg);

   ++recoverMsgNum;
}

bool nv_read_msg(uint8 * rfMsg)
{
   if (recoverMsgNum == 0)
   {
      return false;
   }

   osal_nv_item_init(
      NV_RECOVER_DATA + (recoverMsgNum - 1),
      ELEMENT_SIZE,
      NULL);

   osal_nv_read(
      NV_RECOVER_DATA + (recoverMsgNum - 1),
      0,
      ELEMENT_SIZE,
      rfMsg);

   --recoverMsgNum;

   return true;
}

void nv_write_ipaddr(uint8 * ipAddr)
{
   osal_nv_item_init(
      NV_IP_ADDR,
      IPADDR_LEN,
      NULL);

   osal_nv_write(
      NV_IP_ADDR,
      0,
      IPADDR_LEN,
      ipAddr);
}

void nv_read_ipaddr(uint8 * ipAddr)
{
   osal_nv_item_init(
      NV_IP_ADDR,
      IPADDR_LEN,
      NULL);

   osal_nv_read(
      NV_IP_ADDR,
      0,
      IPADDR_LEN,
      ipAddr);
}