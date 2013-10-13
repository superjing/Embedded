#ifndef SPI_H
#define SPI_H
#include <reg52.h>
#include "spi.h"
//#include "struct.h"

sfr P4 = 0xe8;

sbit VCC1 = P2^0;// VCC1	NO USE
sbit SON = P1^6	;// MISO
sbit SIN = P1^5 ;// MOSI
sbit SCKN = P1^7 ; // SCK
sbit CSN = P1^3	;// 28J60-- CS
sbit RSTN = P3^5 ; //RST
sbit INTN = P3^3 ; // INT 

void WriteByte(u8_t temp);
u8_t ReadByte(void);

#endif
