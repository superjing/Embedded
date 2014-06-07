#include "AF.h"
#include "SampleApp.h"
#include "command.h"
#include "livelist.h"

enum
{
   kSetDelta = 0x75,
   kSetRate = 0x76
};

#define COMMAND_FULL_LENGTH 11

extern endPointDesc_t SampleApp_HeartBeatEpDesc;
extern uint8 SampleApp_TransID;

void process_ip_command(uint8 length, uint8* buffer)
{
   afAddrType_t * desAddr = NULL;
   
   if (length <= SN_LEN)
   {
      return;
   }

   switch(buffer[SN_LEN])
   {
   case kSetDelta:
   case kSetRate:
      if (length != COMMAND_FULL_LENGTH)
      {
         return;
      }
      break;

   default:
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
}
