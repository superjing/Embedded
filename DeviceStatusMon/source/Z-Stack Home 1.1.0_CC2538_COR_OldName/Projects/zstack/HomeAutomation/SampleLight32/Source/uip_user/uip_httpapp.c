#include <string.h>
#include <stdio.h>

#include "hal_types.h"
#include "uip_httpapp.h"
#include "Queue.h"
#include "OnBoard.h"
#include "uip.h"
#include "livelist.h"

static uint8 last_uip_msg[ELEMENT_SIZE];

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

static uint8 odd_even_check(uint8 * buffer, uint8 bufLen)
{
  int numOfOne = 0;

  for (uint8 i = 0; i < bufLen; i++)
  {
    for (uint8 j = 0; j < 8; j++)
    {
      if (buffer[i] & (1 << j))
      {
         ++numOfOne;
      }
    }
  }

  return (uint8)((numOfOne % 2)?(1):(0));
}

static uint8 set_crc_result(uint8 * buffer)
{
  return (odd_even_check(aExtendedAddress, SN_LEN) ^ odd_even_check(buffer, ELEMENT_SIZE));
}

typedef void (*PFUNC)(uint8 *, uint8 *);

typedef struct ConverListStr
{
  uint8 * dstBuf;
  uint8   offset;
  PFUNC   converFunc;
}tConverList;

static uint8 httpBuffer_Head[] = "POST /power/data HTTP/1.1\r\nHost:abc:9090\r\nContent-Type:application/x-www-form-urlencoded\r\nContent-Length:110\r\n\r\n";
static uint8 httpBuffer_Body[] = "tag=EE&len=52&type=01&g1sn=004B1200CB4CA603&sn=0000000000000014&time=00010AC7&ad1=0017&ad2=0017&live=01&crc=01";

static tConverList converlist[] =
{
    // base : aExtendedAddress
    {httpBuffer_Body + 0x1b,       0,  FormatHexUint32Str},
    {httpBuffer_Body + 0x1b + 0x8, 4,  FormatHexUint32Str},
    // base : buffer reveived from T1
    {httpBuffer_Body + 0x2f,       0,  FormatHexUint32Str},
    {httpBuffer_Body + 0x2f + 0x8, 4,  FormatHexUint32Str},
    {httpBuffer_Body + 0x45,       8,  FormatHexUint32Str},
    {httpBuffer_Body + 0x52,       12, FormatHexUint8Str },
    {httpBuffer_Body + 0x54,       13, FormatHexUint8Str },
    {httpBuffer_Body + 0x5b,       14, FormatHexUint8Str },
    {httpBuffer_Body + 0x5d,       15, FormatHexUint8Str },
    {httpBuffer_Body + 0x65,       16, FormatHexUint8Str },
    // base : cdc result
    {httpBuffer_Body + 0x6c,       0,  FormatHexUint8Str },
};

static void converBuf2HttpStr(uint8 * aExtendedAddress, uint8 * buffer, uint8 crc_result)
{
   uint8 i = 0;

   converlist[i].converFunc(converlist[i].dstBuf, aExtendedAddress + converlist[0].offset);
   ++i;
   
   converlist[i].converFunc(converlist[i].dstBuf, aExtendedAddress + converlist[i].offset);
   ++i;
   
   for (i = 2; i < sizeof(converlist)/sizeof(converlist[0]) - 1; ++i)
   {
     converlist[i].converFunc(converlist[i].dstBuf, buffer + converlist[i].offset);
   }
   
   converlist[i].converFunc(converlist[i].dstBuf, &crc_result);
   return;
}

#define HEAD_LEN (sizeof(httpBuffer_Head)/sizeof(httpBuffer_Head[0]) - 1)
#define BODY_LEN (sizeof(httpBuffer_Body)/sizeof(httpBuffer_Body[0]) - 1)

void _uip_send_http(uint8 * buffer)
{
   uint8 dataBuf[HEAD_LEN + BODY_LEN + 1];

   converBuf2HttpStr(aExtendedAddress, buffer, set_crc_result(buffer));
   
   memcpy(dataBuf, httpBuffer_Head, HEAD_LEN);
   memcpy(dataBuf + HEAD_LEN, httpBuffer_Body, BODY_LEN);

   uip_send(dataBuf, HEAD_LEN + BODY_LEN);
   
   if (last_uip_msg != buffer)
   {
      memcpy(last_uip_msg, buffer, ELEMENT_SIZE);
   }
}

void _uip_send_rexmit_http(void)
{
   _uip_send_http(last_uip_msg);
}