#ifndef _NV_H
#define _NV_H

#include "hal_types.h"

// Config Data
#define SN_LEN      (8)
#define TIME_LEN    (4)
#define MSG_NUM_LEN (1)
#define DELTA_LEN   (2)
#define RATE_LEN    (2)

// 4bytes time_high, 4bytes time_low, 2bytes AD1, 2bytes AD2
#define NV_LEN  (12)

#define NV_CONFIG_LEN (SN_LEN + TIME_LEN * 2 + MSG_NUM_LEN * 2 + DELTA_LEN + RATE_LEN)

#define DELTA_DEFAULT  (1)
#define RATE_DEFAULT   (1)

extern uint32 lastNvTime_h;
extern uint32 lastNvTime_l;
extern uint8  serialNumber[SN_LEN];
extern uint16 delta;
extern uint16 heartbitRate;

void nv_next(void);
uint8 nv_number(void);
void nv_read_config(void);
void nv_write_config(void);
void nv_reset_config(void);
void nv_write_msg(void);
bool nv_read_last_msg(uint8* nvBuf);
bool nv_read_msg(uint8* nvBuf);

uint32 get_osal_SystemClock(void);
void reset_osal_SystemClock(void);
#endif
