#ifndef _tcp_app_h_
#define _tcp_app_h_

#include "hal_types.h"

void tpc_app_init(void);
void tpc_app_call(void);
void tcp_client_demo_appcall_user(void);

struct example_state {
   enum {WELCOME_SENT, WELCOME_ACKED} state;
};

#ifndef UIP_APPCALL
//#define UIP_APPCALL     tpc_app_call
#define UIP_APPCALL     tcp_client_demo_appcall_user
#endif

#define UIP_APPSTATE_SIZE  (sizeof(struct example_state))

#define UIP_MSG_SIZE 100

typedef struct example_state uip_tcp_appstate_t;

extern void uip_rf_store(const uint8 * msg, int size);

extern uint32 uip_msg_flag;

extern uint8 uip_msg_length;

extern uint8 * uip_msg_data;

#define MSG_ALARM_MASK    ((uint32)0x01)
#define MSG_HEARTBIT_MASK ((uint32)0x02)

#define SET_MSG_FLAG(mask) (uip_msg_flag |= mask)
#define CLR_MSG_FLAG(mask) (uip_msg_flag &= (~mask))
#define IS_MSG_MASK(mask)  (uip_msg_flag & mask)

#endif