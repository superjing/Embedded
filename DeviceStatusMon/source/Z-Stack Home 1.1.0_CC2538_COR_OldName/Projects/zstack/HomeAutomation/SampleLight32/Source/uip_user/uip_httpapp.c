#include <string.h>
#include <stdio.h>

#include "hal_types.h"
#include "uip_httpapp.h"
#include "Queue.h"
#include "OnBoard.h"
#include "uip.h"
#include "livelist.h"

uint8 buffer[ELEMENT_SIZE] = {0};

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

uint8 crc_result = 0;

uint8 odd_even_check(uint8 * buffer, uint8 bufLen)
{
  int numOfOne = 0;

  for (uint8 i = 0; i < bufLen; i++)
  {
    for (uint8 j = 0; j < 8; j++)
    {
      if (buffer[i] & (1 << j))
      {
              numOfOne++;
      }
    }
  }

  return (uint8)((numOfOne % 2)?(1):(0));
}

void set_crc_result(void)
{
  crc_result = (odd_even_check(aExtendedAddress, SN_LEN) ^ odd_even_check(buffer, ELEMENT_SIZE));
}

uint8 httpBuffer_Head[] = "POST /power/data HTTP/1.1\r\nHost:abc:9090\r\nContent-Type:application/x-www-form-urlencoded\r\nContent-Length:110\r\n\r\n";
uint8 httpBuffer_Body[] = "tag=EE&len=52&type=01&g1sn=004B1200CB4CA603&sn=0000000000000014&time=00010AC7&ad1=0017&ad2=0017&live=01&crc=01";
uint8 httpBuffer_Body_Ack[] = "tag=01&len=50&g1sn=0123456789abcdef&sn=0123456789abcdef&time=12345678&ad1=1234&ad2=5678&live=01";

ConverList converlist[] =
{
    {httpBuffer_Body + 0x1b,       aExtendedAddress + 0,  FormatHexUint32Str},
    {httpBuffer_Body + 0x1b + 0x8, aExtendedAddress + 4,  FormatHexUint32Str},
    {httpBuffer_Body + 0x2f,       buffer + 0,            FormatHexUint32Str},
    {httpBuffer_Body + 0x2f + 0x8, buffer + 4,            FormatHexUint32Str},
    {httpBuffer_Body + 0x45,       buffer + 8,            FormatHexUint32Str},
    {httpBuffer_Body + 0x52,       buffer + 12,           FormatHexUint8Str },
    {httpBuffer_Body + 0x54,       buffer + 13,           FormatHexUint8Str },
    {httpBuffer_Body + 0x5b,       buffer + 14,           FormatHexUint8Str },
    {httpBuffer_Body + 0x5d,       buffer + 15,           FormatHexUint8Str },
    {httpBuffer_Body + 0x65,       buffer + 16,           FormatHexUint8Str },
    {httpBuffer_Body + 0x6c,       (uint8 *)&crc_result,  FormatHexUint8Str },
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

void _uip_send_http(void)
{
   uint8 debugBuf[221] = {0};
   uint8 headLen = (sizeof(httpBuffer_Head)/sizeof(httpBuffer_Head[0]) - 1);
   uint8 bodyLen = (sizeof(httpBuffer_Body)/sizeof(httpBuffer_Body[0]) - 1);

   set_crc_result();
   converBuf2HttpStr();
   memcpy(debugBuf, httpBuffer_Head, headLen);
   memcpy(debugBuf + headLen, httpBuffer_Body, bodyLen);

   uip_send(debugBuf, headLen + bodyLen);
}