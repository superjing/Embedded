#ifndef _NV_H
#define _NV_H

#include "hal_types.h"

// Config Data
#define SN_LEN      (8)
#define TIME_LEN    (4)
#define MSG_NUM_LEN (4)
#define DELTA_LEN   (2)
#define RATE_LEN    (2)

// When the recovery message number reaches to this marco, do the nv opration.
#define MAX_RECOVER_MSG_IN_MEN  (3)

// 4bytes time, 2bytes AD1, 2bytes AD2
#define NV_LEN  (8)

#define NV_CONFIG_LEN (SN_LEN + TIME_LEN + MSG_NUM_LEN + DELTA_LEN + RATE_LEN)

#define DELTA_DEFAULT  (20)
#define RATE_DEFAULT   (1)

extern uint32 lastNvTime;
extern uint32 recoverMsgNumInMem;
extern uint32 recoverMsgNumInNv;
extern uint8  serialNumber[SN_LEN];
extern uint16 delta;
extern uint16 heartbitRate;

typedef struct tRfData
{
  uint8 data[NV_LEN];
}tRfData;

void nv_read_config(void);
void nv_write_config(void);
void nv_reset_config(void);
void nv_write_msg(void);
bool nv_read_last_msg(uint8* nvBuf);
#endif
