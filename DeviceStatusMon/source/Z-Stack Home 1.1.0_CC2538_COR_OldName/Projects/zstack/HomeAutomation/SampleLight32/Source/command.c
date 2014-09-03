#include "AF.h"
#include "SampleApp.h"
#include "command.h"
#include "livelist.h"
#include "uip_tcpapp.h"
#include "OnBoard.h"
#include <string.h>

enum
{
   kSetDelta   = 0x75,
   kSetRate    = 0x76,
   kSetIpAddr  = 0x77
};

#define COMMAND_FULL_LENGTH   11
#define COMMAND_ID_LEN        1
#define SET_IP_COMMAND_LENGTH (SN_LEN + COMMAND_ID_LEN + IPADDR_LEN)

extern endPointDesc_t SampleApp_HeartBeatEpDesc;
extern uint8 SampleApp_TransID;

static void send_command_to_T1(uint8 length, uint8 * buffer)
{
   afAddrType_t * desAddr = NULL;

   if (length != COMMAND_FULL_LENGTH)
   {
      return;
   }

   desAddr = findSrcAddr(buffer);

   if (desAddr == NULL)
   {
      return;
   }

   AF_DataRequest(
         desAddr,
         &SampleApp_HeartBeatEpDesc,
         SAMPLEAPP_PERIODIC_CLUSTERID,
         length - SN_LEN,
         buffer + SN_LEN,
         &SampleApp_TransID,
         AF_DISCV_ROUTE,
         AF_DEFAULT_RADIUS);

   return;
}

void process_ip_command(uint8 length, uint8* buffer)
{
   if (length <= SN_LEN)
   {
      return;
   }

   switch(buffer[SN_LEN])
   {
   case kSetDelta:
   case kSetRate:
      send_command_to_T1(length, buffer);
      break;

   case kSetIpAddr:
      if (memcmp(aExtendedAddress, buffer, SN_LEN) == 0)
      {
        tcp_reset_ip_addr(length - (SN_LEN + COMMAND_ID_LEN), buffer + SN_LEN + COMMAND_ID_LEN);
      }
     break;

   default:
      return;
   }

   return;
}
