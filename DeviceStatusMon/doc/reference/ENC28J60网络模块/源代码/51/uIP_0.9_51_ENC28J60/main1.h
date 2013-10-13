#ifndef _INCLUDES_H
#define _INCLUDES_H

#include <REG52.H>
//#include <REGX52.H>	 
//#define LCD  //��ʹ��LCD������������

#include "struct.h"
#include "ethernet.h"
#include "udp.h"
#include "enc28j60.h"
#include "ip.h"
#include "arp.h"
#include "icmp.h"
//#include "tcp.h"
//#include "lcd.h"

#define LOW(uint) (uint&0xFF)
#define HIGH(uint) ((uint>>8)&0xFF)

#define MAX_RXTX_BUFFER		120 //1518




void initUart(void);

void server_process ( void );
//void client_process ( void );
//��ѯ��ʽ���͵����ַ�
void send(unsigned d1);
//�ж�ʵʼ��
void IntInit(void);
void delay_ms(int t1);
void delay_us(int t1);

extern void memcpy( BYTE * dest,BYTE * source, WORD len );

//#include "tcp.c"

#endif
