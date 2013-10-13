#include "main.h"
#include "spi.h"
#include "mcu_uart.h"

char code zxm[]={
    	 0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,
     //字形 0   1    2    3    4    5    6    7    8   9 
          0x88,0x83,0xc6,0xa1,0x86,0x8e,0xff,0xbf,0xff};
    //字形 A   B    C   D    E    F       	-
char code zwm[]={0x01,0x02,0x04,0x08,0x10}; 

extern BYTE digit[4];
extern union FLAG1 flag1;
extern WORD_BYTES ip_identfier;
extern BYTE  code avr_mac[6];	  //
extern BYTE  code avr_ip[4];
extern BYTE  code server_mac[6],server_ip[4];

void main1(void)
{
	//  BYTE mac[6];
	BYTE led =0;//
	ip_identfier.word =1;// 时间窗初值
	init_uart();   
    printu("starting...\r\n");
	enc28j60_init();
	//lcd_string("IP:172.31.220.1 PORT:3000 ENCCART");
	flag1.bits.syn_is_sent=0;
	while(1)
	{
		 // server process response for arp, icmp, http
		 INTN =1;
		 while(INTN==0)
		{
			  server_process();
			  //printu("--------------\r\n");
		}
	}
}


