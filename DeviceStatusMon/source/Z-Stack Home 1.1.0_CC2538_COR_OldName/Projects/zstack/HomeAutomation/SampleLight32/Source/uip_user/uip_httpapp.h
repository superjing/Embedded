#ifndef _UIP_HTTPAPP_H
#define _UIP_HTTPAPP_H

typedef void (*PFUNC)(uint8 *, uint8 *);

typedef struct ConverListStr
{
	uint8 * dstBuf;
	uint8 * srcBuf;
	PFUNC   converFunc;
}ConverList;

void _uip_send_http(void);

#endif