#include "uip_dhcpc.h"
#include "uip_tcpapp.h"
#include "uip.h"
#include "queue.h"
#include "SampleApp.h"
#include "OSAL.h"
#include "nv.h"

#include <string.h>
#include <stdio.h>

#ifdef DEBUG_TRACE
#define RF_MSG_SIZE 30
#else
#define RF_MSG_SIZE ELEMENT_SIZE
#endif

extern LockFreeQueue queue;

static uint8 last_uip_msg[ELEMENT_SIZE];

#ifdef DEBUG_TRACE
static void FormatHexUint32Str(uint8 *dst, uint8 * src)
{
   uint8 i = 0;
   uint8 * ptr = src;

   for (i = 0; i < (sizeof(uint32) * 2); ptr++)
   {
      uint8 ch;
      ch = (*ptr >> 4) & 0x0F;
      dst[i++] = ch + (( ch < 10 ) ? '0' : '7');
      ch = *ptr & 0x0F;
      dst[i++] = ch + (( ch < 10 ) ? '0' : '7');
   }
}
#endif

static void _uip_send(uint8 * buffer)
{
#ifdef DEBUG_TRACE
   uint8 debugBuf[RF_MSG_SIZE];

   memcpy(debugBuf, "sn=", 3);
   memcpy(debugBuf + 3, buffer, 4);
   memcpy(debugBuf + 7, " time=0x", 8);
   FormatHexUint32Str(debugBuf + 15, buffer + 4);
   memcpy(debugBuf + 23, " ", 1);
   memcpy(debugBuf + 24, buffer + 8, 4);
   memcpy(debugBuf + 28, "\n\0", 2);
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
   uip_ipaddr(&ipaddr, 10, 144, 3, 104);
   uip_connect(&ipaddr, HTONS(1234));
}

void tcp_client_demo_appcall_user(void)
{
   uint8_t buffer[ELEMENT_SIZE];

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
      _uip_send(buffer);
   }
   else
   {
      if (nv_read_msg(buffer))
      {
         _uip_send(buffer);
      }
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
      case HTONS(1234):
         tcp_client_demo_appcall_user();
         break;
   }

   switch(uip_conn->rport)
   {
      case HTONS(1234):
         tcp_client_demo_appcall_user();
         break;
   }
}
