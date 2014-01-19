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
extern uint32 nvMsgNum;
extern uint32 recoverMsgNum;
extern void nv_write_config(void);
extern void nv_read_msg(uint32 msgIndex, uint8* rfMsg);
extern uint8 rfMsg[MAX_TIME * MSG_ELEM_LENGTH];

static void FormatHexUint32Str(uint8 *dst, uint8 * src);
static void uip_send_debug(uint8 *buffer);

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
  /*for clean warning*/
  rf_msg_length = rf_msg_length;
}

static void send_uip_packet(LockFreeQueue * queue)
{
   uint8_t  buffer[ELEMENT_NUM_TO_SEND * ELEMENT_SIZE];

   FreeQueuePop(queue, buffer);
#ifdef DEBUG_TRACE
   uip_send_debug(buffer);
#else
   uip_send(buffer, ELEMENT_NUM_TO_SEND * ELEMENT_SIZE);
   memcpy(last_uip_msg, buffer, ELEMENT_NUM_TO_SEND * ELEMENT_SIZE);
   last_uip_msg_length = (ELEMENT_NUM_TO_SEND * ELEMENT_SIZE);
#endif
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

  /**send nv msg**/
  uint8 buf[MSG_ELEM_LENGTH] = {0};
  if (nvMsgNum == 0)
  {
    if (recoverMsgNum != 0)
    {
      nv_read_msg(recoverMsgNum - 1, buf);
#ifdef DEBUG_TRACE
      uip_send_debug(buf);
#else
      uip_send(buf, ELEMENT_NUM_TO_SEND * ELEMENT_SIZE);
#endif
      --recoverMsgNum;

      if (recoverMsgNum == 0)
      {
        nv_write_config();
      }
    }
  }
  else
  {
    memcpy(buf, rfMsg + (nvMsgNum - 1)*MSG_ELEM_LENGTH, MSG_ELEM_LENGTH);
#ifdef DEBUG_TRACE
    uip_send_debug(buf);
#else
    uip_send(buf, ELEMENT_NUM_TO_SEND * ELEMENT_SIZE);
#endif
    --nvMsgNum;
  }
  /**send nv msg end **/

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

static void uip_send_debug(uint8 *buffer)
{
   uint8 debugBuf[32] = {0};
   memcpy(debugBuf, "sn=", 3);
   memcpy(debugBuf + 3, buffer, 4);
   memcpy(debugBuf + 7, " time=0x", 8);
   FormatHexUint32Str(debugBuf + 15, buffer + 4);
   memcpy(debugBuf + 23, " ", 1);
   memcpy(debugBuf + 24, buffer + 8, 4);
   memcpy(debugBuf + 28, "\n\0", 2);
   uip_send(debugBuf, 32);
   memcpy(last_uip_msg, debugBuf, 32);
   last_uip_msg_length = 32;
}