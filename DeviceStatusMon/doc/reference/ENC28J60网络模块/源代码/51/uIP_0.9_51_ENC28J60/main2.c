#include "main.h"
#include "spi.h"
#include "mcu_uart.h"

//BYTE xdata rxtx_buffer1[MAX_RXTX_BUFFER];

void main2(void)
{
	u16_t plen;
	init_uart();
	printu("starting............\r\n");

	enc28j60_init();
//	flag1.bits.syn_is_sent=0;
	while(1)
	{
		 // server process response for arp, icmp, http
		 INTN =1;
		 while(!INTN)
		{
			  plen = enc28j60_packet_receive(uip_buf, UIP_BUFSIZE);
			  printuf("plen = %x", plen>>8);
			  printuf("%x\r\n", plen);
		}
	}

}
