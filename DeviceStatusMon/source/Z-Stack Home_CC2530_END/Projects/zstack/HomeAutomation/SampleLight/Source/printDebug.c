#include "printDebug.h"
#include "MT_Uart.h"
#include "string.h"

#ifdef DEBUG_TRACE

void PrintRecover(uint8* buf, bool result)
{
   HalUARTWrite(0, "Recover :\n", 10);
   ShowHeartBeatInfo(buf);
   HalUARTWrite(0, buf + 8, 4);
   if (result)
   {
      HalUARTWrite(0, "OK\n", 3);
   }
   else
   {
      HalUARTWrite(0, "Err\n", 4);
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

static void FormatHexUint8Str(uint8 *dst, uint8 * src)
{
   uint8 i = 0;
   uint8 * ptr = src;

   for (i = 0; i < (sizeof(8) * 2); ptr++)
   {
      uint8 ch;
      ch = (*ptr >> 4) & 0x0F;
      dst[i++] = ch + (( ch < 10 ) ? '0' : '7');
      ch = *ptr & 0x0F;
      dst[i++] = ch + (( ch < 10 ) ? '0' : '7');
   }
}

void ShowHeartBeatInfo(uint8 *buf)
{
   uint8 temp[10] = {0};

   temp[0] = '0';
   temp[1] = 'x';
   FormatHexUint32Str(temp + 2, buf + 4);

   HalUARTWrite(0, "sn is:", 6);
   HalUARTWrite(0, buf, 4);
   HalUARTWrite(0, " curTime is:", 12);
   HalUARTWrite(0, temp, 10);
   
   FormatHexUint8Str(temp + 2, buf + 8);
   HalUARTWrite(0, " high:", 6);
   HalUARTWrite(0, temp, 4);
   
   FormatHexUint8Str(temp + 2, buf + 9);
   HalUARTWrite(0, " low:", 5);
   HalUARTWrite(0, temp, 4);
   
   HalUARTWrite(0, "\n", 1);
}

/*Show Received Signal Strength Indication*/
extern int8 macRssi;

void ShowRssiInfo(void)
{
   int8 rssi = 0;
   uint8 str[8] = {0};

   /* read latest RSSI value */
   rssi = macRssi;

   if (rssi < 0)
   {
      str[0] = '-';
      rssi = 0 - rssi;
   }
   else
   {
      str[0] = '+';
   }
   str[1] = rssi/10 + '0';
   str[2] = rssi%10 + '0';

   memcpy(str + 3, "dbm\n", 4);

   HalUARTWrite(0, "rssi is:", 8);
   HalUARTWrite(0, str, 7);
}
#endif
