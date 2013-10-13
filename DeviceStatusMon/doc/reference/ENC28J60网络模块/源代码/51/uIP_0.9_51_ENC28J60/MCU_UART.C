/*  Copyright (C) 2008 HeXing. All rights reserved.
	MP3_Player+USB_Disk V0.1 Edit by DianShiJin.cn STUDIO 2008.01
*/
//#include <stdio.h>

#include "reg51.H"
#include "MCU_UART.H"

code char hex[16]  = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

void init_uart()
{  
//	if (fre == 1200)	 //only support 1200 Baud rate
//	{   
//		CKCON |=0x04;
		TMOD = 0x20;
		TH1  = 0xFD;
		TL1  = 0xFD;
		SCON = 0x50;
		PCON = PCON|0x80;
		TR1  = 1;
//		TI = 1;
//		printf("Hello world!\r\n");
// 	}
}

//Send String Tor Uart
void printu(char * str)	
{	
    idata char *ct = str;

	while (*ct != '\0')
	{
		if (*ct == '\n')
		{
			SBUF = 13;
			while (!TI);
			TI = 0;
		}
		SBUF=*ct;
		while (!TI);
		TI = 0;
		ct++;
	}
}

//
void printuf(char *str, unsigned char cb) 
{
	idata char *ct = str;
	idata char cx1;
	idata char cx2;

	while (*ct != '\0')
	{
		if (*ct == '%')
		{	
			if (*(ct + 1) == 'x')              
			{
				ct += 2;
				cx1 = cb / 16;
				cx2 = cb % 16;
				SBUF = hex[cx1];
				while (!TI);
				TI = 0;
				SBUF = hex[cx2];
				while (!TI);
				TI = 0;
				continue;
			}

			if (*(ct + 1) == 'c')              
			{
				ct += 2;
				SBUF = cb;
				while (!TI);
				TI = 0;
				continue;
			}
			
		}
		if (*ct == '\n')
		{
			SBUF = 13;
			while (!TI);
			TI = 0;
		}
		SBUF = *ct;
		while (!TI);
		TI = 0;
		ct++;
	}
}
