#ifndef _NV_H
#define _NV_H

#include "hal_types.h"

#define SN_LEN 8
// write threee nv msg at one time
#define NV_NUM_STORE  3

#define NV_LEN         6
#define NV_CONFIG_LEN 16

extern uint32 lastNvTime;
extern uint32 nvMsgNum;
extern uint32 recoverMsgNum;
extern uint8 serialNumber[SN_LEN];

typedef struct tRfData
{
  uint8 data[NV_LEN];
}tRfData;

void nv_read_config(void);
void nv_write_config(void);
void nv_reset_config(void);
void nv_write_msg(void);
void nv_read_msg(uint32 msgIndex, uint8* curTime);
#endif
