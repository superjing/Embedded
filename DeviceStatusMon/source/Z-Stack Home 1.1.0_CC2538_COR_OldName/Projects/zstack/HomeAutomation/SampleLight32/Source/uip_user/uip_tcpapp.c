#include "uip_dhcpc.h"
#include "uip_tcpapp.h"
#include "uip_httpapp.h"
#include "uip.h"
#include "queue.h"
#include "OSAL.h"
#include "nv.h"
#include "livelist.h"
#include "command.h"
#include "OnBoard.h"
#include "hal_led.h"

#include <string.h>
#include <stdio.h>

#ifdef DEBUG_TRACE
#define RF_MSG_SIZE 53
#else
#define RF_MSG_SIZE ELEMENT_SIZE
#endif

#define HTTP_RESPONSE_HEAD_LEN  (353)

#define TCP_PORT 9090

extern LockFreeQueue queue;

static uint8_t ipAddr[IPADDR_LEN] = {10, 144, 3, 104};

static void tcp_client_reconnect(void)
{
   uip_ipaddr_t ipaddr;
   uip_ipaddr(&ipaddr, ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);
   uip_connect(&ipaddr, HTONS(9090));
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
      process_ip_command(uip_datalen(), (uint8*)uip_appdata);
   }
}

void tcp_server_appcall_user(void)
{
   if (uip_newdata())
   {
     //TODO: Check HTTP_RESPONSE_HEAD_LEN
      process_ip_command((uip_datalen() - HTTP_RESPONSE_HEAD_LEN), ((uint8*)uip_appdata + HTTP_RESPONSE_HEAD_LEN));
   }
}

void tpc_app_init(void)
{
   tcp_client_reconnect();

   //Only listen one time after get IP address
   uip_listen(HTONS(9090));
}

void tpc_app_call(void)
{
   switch(uip_conn->lport)
   {
      case HTONS(TCP_PORT):
         tcp_server_appcall_user();
         break;
   }

   switch(uip_conn->rport)
   {
      case HTONS(TCP_PORT):
         tcp_client_appcall_user();
         break;
   }
}
