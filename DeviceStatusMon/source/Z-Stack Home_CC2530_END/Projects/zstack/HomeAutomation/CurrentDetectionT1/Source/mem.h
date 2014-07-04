#ifndef _MEM_H
#define _MEM_H

#include "hal_types.h"

// When the recovery message number reaches to this marco, do the nv opration.
#define MAX_RECOVER_MSG_IN_MEN  (30)

// 4bytes time, 2bytes AD1, 2bytes AD2
#define NV_LEN  (8)

uint8  nv_mem_number(void);
uint8* nv_mem_read(void);
uint8* nv_mem_read_last(void);
void   nv_mem_next(void);
bool nv_mem_write(uint8* data);
bool nv_mem_full(void);
#endif
