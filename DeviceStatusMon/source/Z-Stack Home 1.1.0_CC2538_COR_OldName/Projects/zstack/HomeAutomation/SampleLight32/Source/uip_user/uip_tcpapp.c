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

#define TCP_PORT 9090

extern LockFreeQueue queue;
extern uint8 gCorSendGap;

static uint8_t ipAddr[IPADDR_LEN] = {10, 144, 3, 104};

static void tcp_client_reconnect(void)
{
   uip_ipaddr_t ipaddr;
   uip_ipaddr(&ipaddr, ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);
   uip_connect(&ipaddr, HTONS(9090));
   uip_listen(HTONS(9090));
}

void tcp_client_appcall_user(void)
{
   uint8 buffer[ELEMENT_SIZE];
   if (uip_aborted() || uip_timedout() || uip_closed())
   {
      tcp_client_reconnect();
      return;
   }

   if (uip_rexmit())
   {
      _uip_send_rexmit_http();
      return;
   }

   if (QueueLength(&queue) != 0)
   {
      FreeQueuePop(&queue, buffer);
      _uip_send_http(buffer);
   }
   else
   {
      if (nv_read_msg(buffer))
      {
         _uip_send_http(buffer);
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
      case HTONS(TCP_PORT):
         tcp_client_appcall_user();
         break;
   }

   switch(uip_conn->rport)
   {
      case HTONS(TCP_PORT):
         tcp_client_appcall_user();
         break;
   }
}

#define SET_GAP_COUNT_CMD 0xEF
#define SET_GAP_COUNT_CMD_LEN (sizeof(SET_GAP_COUNT_CMD) + sizeof(gCorSendGap))

void ip_cmd_process(uint8 cmdLen, uint8 * cmd)
{
  if ((5 == cmdLen) && (0 == memcmp("hello", cmd, 5)))
  {
    uip_send("I get hello", 11);
    return;
  }

  if ((SET_GAP_COUNT_CMD_LEN == cmdLen) && (SET_GAP_COUNT_CMD == cmd[0]))
  {
    gCorSendGap = cmd[1];
    return;
  }
}
