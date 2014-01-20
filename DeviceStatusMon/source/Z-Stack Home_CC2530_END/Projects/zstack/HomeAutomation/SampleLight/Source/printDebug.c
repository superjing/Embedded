#include "printDebug.h"
#include "MT_Uart.h"
#include "string.h"

#ifdef DEBUG_TRACE

void PrintRecover(uint8* buf, bool result)
{
   HalUARTWrite(0, "Recover :\n", 10);
   ShowHeartBeatInfo(buf, buf + 4);
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

void ShowHeartBeatInfo(uint8 *serialNumber, uint8 *curTime)
{
   uint8 curTimeStr[10] = {0};

   curTimeStr[0] = '0';
   curTimeStr[1] = 'x';
   FormatHexUint32Str(curTimeStr + 2, curTime);

   HalUARTWrite(0, "sn is:", 6);
   HalUARTWrite(0, serialNumber, 4);
   HalUARTWrite(0, "\n", 1);
   HalUARTWrite(0, "curTime is:", 11);
   HalUARTWrite(0, curTimeStr, 10);
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
