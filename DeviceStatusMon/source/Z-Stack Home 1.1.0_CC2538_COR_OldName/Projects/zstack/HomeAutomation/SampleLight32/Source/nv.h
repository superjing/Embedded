#ifndef _NV_H
#define _NV_H

#include "hal_types.h"

void nv_write_msg(uint8 * rfMsg);
bool nv_read_msg(uint8 * rfMsg);
void nv_write_ipaddr(uint8 * ipAddr);
void nv_read_ipaddr(uint8 * ipAddr);
#endif