#include <string.h>
#include <stdio.h>

#include "hal_types.h"
#include "uip_httpapp.h"
#include "Queue.h"
#include "OnBoard.h"
#include "uip.h"
#include "livelist.h"

static uint8 last_uip_msg[ELEMENT_SIZE];

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

static void FormatHexUint32Str(uint8 *dst, uint8 * src)
{
   FormatHexUint8Str(dst, src);
   FormatHexUint8Str(dst + 2, src + 1);
   FormatHexUint8Str(dst + 4, src + 2);
   FormatHexUint8Str(dst + 6, src + 3);
}

static void FormatHexUint16Str(uint8 *dst, uint8 * src)
{
   FormatHexUint8Str(dst, src);
   FormatHexUint8Str(dst + 2, src + 1);
}

static void FormatSnStr(uint8 *dst, uint8 * src)
{
   FormatHexUint32Str(dst, src);
   FormatHexUint32Str(dst + 8, src + 4);
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

static uint8 httpHeartBitBuffer[] =
   "POST /power/data HTTP/1.1\r\n"
   "Host:abc:9090\r\n"
   "Content-Type:application/x-www-form-urlencoded\r\n"
   "Content-Length:110\r\n\r\n"
   "tag=EE&len=52&type=01&g1sn=004B1200CB4CA603&sn=0000000000000014&time=00010AC7&ad1=0017&ad2=0017&live=01&crc=01";

static uint8 httpCommandResp[] =
   "POST /power/response HTTP/1.1\r\n"
   "Host:abc:9090\r\n"
   "Content-Type:application/x-www-form-urlencoded\r\n"
   "Content-Length:63\r\n\r\n"
   "tag=EA&value=0000&g1sn=004B1200CB4CA603&sn=0000000000000014";

enum
{
  // POST /power/data HTTP/1.1\r\nHost:abc:9090\r\nContent-Type:application/x-www-form-urlencoded\r\nContent-Length:110\r\n\r\n
  kHeartBitHttpHeadLen = 112,
  kHeartBitHttpLen = sizeof(httpHeartBitBuffer) - 1,
  
  kHeartBitHttpGsnIndex = kHeartBitHttpHeadLen + 27,
  kHeartBitHttpTsnIndex = kHeartBitHttpHeadLen + 47,
  kHeartBitHttpTimeStampIndex = kHeartBitHttpHeadLen + 69,
  kHeartBitHttpValue1Index = kHeartBitHttpHeadLen + 82,
  kHeartBitHttpValue2Index = kHeartBitHttpHeadLen + 91,
  kHeartBitHttpLiveIndex = kHeartBitHttpHeadLen + 101,
  kHeartBitHttpCdcIndex = kHeartBitHttpHeadLen + 108
};

enum
{
  // POST /power/response HTTP/1.1\r\nHost:abc:9090\r\nContent-Type:application/x-www-form-urlencoded\r\nContent-Length:63\r\n\r\n
  kCommandRespHeadLen = 115,
  kCommandRespHttpLen = sizeof(httpCommandResp) - 1,

  kCommandRespHttpTagIndex = kCommandRespHeadLen + 4,
  kCommandRespHttpValueIndex = kCommandRespHeadLen + 13,
  kCommandRespHttpGsnIndex = kCommandRespHeadLen + 23,
  kCommandRespHttpTsnIndex = kCommandRespHeadLen + 43,
};

static void converBuf2HeartBitHttpStr(uint8 * aExtendedAddress, uint8 * buffer, uint8 crc_result)
{
   FormatSnStr(httpHeartBitBuffer + kHeartBitHttpGsnIndex, aExtendedAddress);
   FormatSnStr(httpHeartBitBuffer + kHeartBitHttpTsnIndex, buffer);
   FormatHexUint32Str(httpHeartBitBuffer + kHeartBitHttpTimeStampIndex, buffer + 8);
   FormatHexUint16Str(httpHeartBitBuffer + kHeartBitHttpValue1Index, buffer + 12);
   FormatHexUint16Str(httpHeartBitBuffer + kHeartBitHttpValue2Index, buffer + 14);
   FormatHexUint8Str(httpHeartBitBuffer + kHeartBitHttpLiveIndex, buffer + 16);
   FormatHexUint8Str(httpHeartBitBuffer + kHeartBitHttpCdcIndex, &crc_result);
}

static void converBuf2CommandRespHttpStr(
      uint8 * aExtendedAddress, uint8 * buffer)
{
   uint8 command = buffer[ELEMENT_SIZE  - 1] & 0x7F;

   FormatHexUint8Str(httpCommandResp + kCommandRespHttpTagIndex, &command);
   FormatHexUint16Str(httpCommandResp + kCommandRespHttpValueIndex, buffer + 14);
   FormatSnStr(httpCommandResp + kCommandRespHttpGsnIndex, aExtendedAddress);
   FormatSnStr(httpCommandResp + kCommandRespHttpTsnIndex, buffer);
}

void _uip_send_http(uint8 * buffer)
{
   if (buffer[ELEMENT_SIZE  - 1] & 0x80)
   {
       converBuf2CommandRespHttpStr(aExtendedAddress, buffer);
       uip_send(httpCommandResp, kCommandRespHttpLen);
   }
   else
   {
      converBuf2HeartBitHttpStr(aExtendedAddress, buffer, set_crc_result(buffer));
      uip_send(httpHeartBitBuffer, kHeartBitHttpLen);
   }

   if (last_uip_msg != buffer)
   {
      memcpy(last_uip_msg, buffer, ELEMENT_SIZE);
   }
}

void _uip_send_rexmit_http(void)
{
   _uip_send_http(last_uip_msg);
}