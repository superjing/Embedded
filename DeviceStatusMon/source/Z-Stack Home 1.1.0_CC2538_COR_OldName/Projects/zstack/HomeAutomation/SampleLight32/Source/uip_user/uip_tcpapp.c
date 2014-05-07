#include "uip_dhcpc.h"
#include "uip_tcpapp.h"
#include "uip_httpapp.h"
#include "uip.h"
#include "queue.h"
#include "SampleApp.h"
#include "OSAL.h"
#include "nv.h"
#include "OnBoard.h"

#include <string.h>
#include <stdio.h>

#ifdef DEBUG_TRACE
#define RF_MSG_SIZE 53
#else
#define RF_MSG_SIZE ELEMENT_SIZE
#endif

extern LockFreeQueue queue;
extern uint8 CorSendGap;

static uint8 last_uip_msg[ELEMENT_SIZE];

uint8_t ipAddr[IPADDR_LEN] = {10, 144, 3, 104};

extern uint8 buffer[ELEMENT_SIZE];

static void _uip_send(uint8 * buffer)
{
#ifdef DEBUG_TRACE
   uint8 debugBuf[RF_MSG_SIZE];

   memcpy(debugBuf, "s=0x", 4);
   FormatHexUint32Str(debugBuf + 4, buffer);
   FormatHexUint32Str(debugBuf + 12, buffer + 4);
   memcpy(debugBuf + 20, " t=0x", 5);
   FormatHexUint32Str(debugBuf + 25, buffer + 8);
   memcpy(debugBuf + 33, " A=0x", 5);
   FormatHexUint8Str(debugBuf + 38, buffer + 12);
   FormatHexUint8Str(debugBuf + 40, buffer + 13);
   memcpy(debugBuf + 42, " B=0x", 5);
   FormatHexUint8Str(debugBuf + 47, buffer + 14);
   FormatHexUint8Str(debugBuf + 49, buffer + 15);
   memcpy(debugBuf + 51, "\n\0", 2);
   uip_send(debugBuf, RF_MSG_SIZE);
#else
   uip_send(buffer, RF_MSG_SIZE);
#endif
   if (last_uip_msg != buffer)
   {
      memcpy(last_uip_msg, buffer, ELEMENT_SIZE);
   }
}

static void tcp_client_reconnect(void)
{
   uip_ipaddr_t ipaddr;
   uip_ipaddr(&ipaddr, ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);
   uip_connect(&ipaddr, HTONS(9090));
   uip_listen(HTONS(9090));
}

void tcp_client_demo_appcall_user(void)
{
   if (uip_aborted() || uip_timedout() || uip_closed())
   {
      tcp_client_reconnect();
      return;
   }

   if (uip_rexmit())
   {
      _uip_send(last_uip_msg);
      return;
   }

   if (QueueLength(&queue) != 0)
   {
      FreeQueuePop(&queue, buffer);
      _uip_send_http();
   }
   else
   {
      if (nv_read_msg(buffer))
      {
         _uip_send_http();
      }
   }

   if (uip_newdata())
   {
      ip_cmd_process(uip_datalen(), (uint8*)uip_appdata);
   }
}

void tpc_app_init(void)
{
   tcp_client_reconnect();
}

void tpc_app_call(void)
{
   switch(uip_conn->lport)
   {
      case HTONS(9090):
         tcp_client_demo_appcall_user();
         break;
   }

   switch(uip_conn->rport)
   {
      case HTONS(9090):
         tcp_client_demo_appcall_user();
         break;
   }
}

void ip_cmd_process(uint8 cmdLen, uint8 * cmd)
{
  if ((5 == cmdLen) && (0 == memcmp("hello", cmd, 5)))
  {
    uip_send("I get hello", 11);
  }

  if ((2 == cmdLen) && (0xEF == cmd[0]))
  {
    CorSendGap = cmd[1];
  }
}
