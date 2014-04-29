#include "uip_dhcpc.h"
#include "uip_tcpapp.h"
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

static uint8 last_uip_msg[ELEMENT_SIZE];

uint8_t ipAddr[IPADDR_LEN] = {10, 144, 3, 104};

static uint8 buffer[ELEMENT_SIZE] = {0};

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

static void FormatHexUint8Str(uint8 *dst, uint8 * src)
{
   uint8 i = 0;
   uint8 * ptr = src;

   for (i = 0; i < (sizeof(uint8) * 2); ptr++)
   {
      uint8 ch;
      ch = (*ptr >> 4) & 0x0F;
      dst[i++] = ch + (( ch < 10 ) ? '0' : '7');
      ch = *ptr & 0x0F;
      dst[i++] = ch + (( ch < 10 ) ? '0' : '7');
   }
}

uint8 httpBuffer_Head[] = "POST /power/data HTTP/1.1\r\nHost:abc:9090\r\nContent-Type:application/x-www-form-urlencoded\r\nContent-Length:95\r\n\r\n";
uint8 httpBuffer_Body[] = "tag=01&len=50&g1sn=0123456789abcdef&sn=0123456789abcdef&time=12345678&ad1=1234&ad2=5678&live=01";

ConverList converlist[] =
{
    {httpBuffer_Body + 0x13,       aExtendedAddress + 0,  FormatHexUint32Str},
    {httpBuffer_Body + 0x13 + 0x8, aExtendedAddress + 4,  FormatHexUint32Str},
    {httpBuffer_Body + 0x27,       buffer + 0,            FormatHexUint32Str},
    {httpBuffer_Body + 0x27 + 0x8, buffer + 4,            FormatHexUint32Str},
    {httpBuffer_Body + 0x3d,       buffer + 8,            FormatHexUint32Str},
    {httpBuffer_Body + 0x4a,       buffer + 12,           FormatHexUint8Str },
    {httpBuffer_Body + 0x4c,       buffer + 13,           FormatHexUint8Str },
    {httpBuffer_Body + 0x53,       buffer + 14,           FormatHexUint8Str },
    {httpBuffer_Body + 0x55,       buffer + 15,           FormatHexUint8Str },
    {httpBuffer_Body + 0x5d,       buffer + 16,           FormatHexUint8Str },
};

uint8 converlistNum = sizeof(converlist)/sizeof(converlist[0]);

void converBuf2HttpStr(void)
{
   uint8 i = 0;

   for (i = 0; i < converlistNum; i++)
   {
     converlist[i].converFunc(converlist[i].dstBuf, converlist[i].srcBuf);
   }

   return;
}


static void _uip_send_http(void)
{
   uint8 debugBuf[207] = {0};
   uint8 headLen = (sizeof(httpBuffer_Head)/sizeof(httpBuffer_Head[0]) - 1);
   uint8 bodyLen = (sizeof(httpBuffer_Body)/sizeof(httpBuffer_Body[0]) - 1);

   converBuf2HttpStr();
   memcpy(debugBuf, httpBuffer_Head, headLen);
   memcpy(debugBuf + headLen, httpBuffer_Body, bodyLen);

   uip_send(debugBuf, headLen + bodyLen);
}

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
#if 0
      FreeQueuePop(&queue, buffer);
      _uip_send(buffer);
#endif
      FreeQueuePop(&queue, buffer);
      _uip_send_http();
   }
   else
   {
      if (nv_read_msg(buffer))
      {
         _uip_send(buffer);
      }
   }

   if (uip_newdata())
   {
      if ((IPADDR_CMD_LEN == uip_datalen()) && (IPADDR_CMD == ((uint8*)uip_appdata)[0]))
      {
        memcpy(ipAddr, ((uint8*)uip_appdata + 1), IPADDR_LEN);
        tcp_client_reconnect();
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
