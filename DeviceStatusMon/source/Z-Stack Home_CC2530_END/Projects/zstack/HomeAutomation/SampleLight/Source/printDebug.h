#ifndef _PRINT_DEBUG_H
#define _PRINT_DEBUG_H

#include "hal_types.h"

#ifdef DEBUG_TRACE
void PrintRecover(uint8* buf, bool result);
void ShowHeartBeatInfo(uint8 *buf);
void ShowRssiInfo(void);
#else
#define PrintRecover(buf, result)
#define ShowHeartBeatInfo(serialNumber, curTime)
#define ShowRssiInfo()
#endif

#endif
