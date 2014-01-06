#include "uip_dhcpc.h"
#include "uip_tcpapp.h"
#include "uip.h"
#include "queue.h"
#include "SampleApp.h"
#include "OSAL.h"
#include <string.h>
#include <stdio.h>

uint32 uip_msg_flag = 0;

#define RF_MSG_SIZE  100

extern LockFreeQueue queue;

static uint8 uip_msg[UIP_MSG_SIZE] = {0};
uint8 uip_msg_length = 0;
uint8 * uip_msg_data = uip_msg;


static char rf_msg[RF_MSG_SIZE];
static int rf_msg_length  = 0;

static char last_uip_msg[RF_MSG_SIZE];
static int last_uip_msg_length  = 0;

void uip_msg_store(const uint8 *data, int len)
{
  memcpy(uip_msg, data, len);
  uip_msg_length = len;
}

void uip_rf_store(const uint8 *data, int len)
{
  memcpy(rf_msg, data, len);
  rf_msg_length = len;
}

static void send_uip_packet(LockFreeQueue * queue)
{
   uint8_t  buffer[ELEMENT_NUM_TO_SEND * ELEMENT_SIZE];

   FreeQueuePop(queue, buffer);
   uip_send(buffer, ELEMENT_NUM_TO_SEND * ELEMENT_SIZE);

   memcpy(last_uip_msg, buffer, ELEMENT_NUM_TO_SEND * ELEMENT_SIZE);
   last_uip_msg_length = (ELEMENT_NUM_TO_SEND * ELEMENT_SIZE);
}

static void tcp_client_reconnect(void)
{
   uip_ipaddr_t ipaddr;
   uip_ipaddr(&ipaddr, 10, 144, 3, 104);
   uip_connect(&ipaddr, HTONS(1234));
}

void tcp_client_demo_appcall_user(void)
{
  if (uip_aborted())
  {
      tcp_client_reconnect();
  }

  if (uip_timedout())
  {
  }

  if (uip_closed())
  {
      tcp_client_reconnect();
  }

  if (uip_rexmit())
  {
    uip_send(last_uip_msg, last_uip_msg_length);
    return;
  }

  if (QueueLength(&queue) >= ELEMENT_NUM_TO_SEND)
  {
    send_uip_packet(&queue);
  }

  if (uip_newdata())
  {
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