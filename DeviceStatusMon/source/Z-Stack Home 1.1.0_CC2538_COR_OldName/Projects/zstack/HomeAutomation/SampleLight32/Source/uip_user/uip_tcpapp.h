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

#endif
