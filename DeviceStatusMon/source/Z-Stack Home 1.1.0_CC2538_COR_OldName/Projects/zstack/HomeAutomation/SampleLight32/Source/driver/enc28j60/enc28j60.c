#include "enc28j60.h"

#include "uip.h"

#define TOTAL_HEADER_LENGTH (40 + 0x0E)

static uint8_t  Enc28j60Bank;
static uint16_t NextPacketPtr;

static void delay_100ns(void)
{
   asm("nop");
   asm("nop");
   asm("nop");
   asm("nop");
}

void delay_ms(int t1)
{
   int i;
   while(t1--)
   {
      for(i=10;i;--i)
      {
         delay_100ns();
      }
   }
}

//*******************************************************************************************
//
// Function : enc28j60ReadOp
// Description :
//
//*******************************************************************************************
static uint8_t enc28j60ReadOp(uint8_t op, uint8_t address)
{

   uint8_t dat1;
   // activate CS
   enc28j60_spi_cs(0);
   // issue read command
   delay_100ns();
   enc28j60_spi_write(op | (address & ADDR_MASK));
   dat1 = enc28j60_spi_read();
   // do dummy read if needed (for mac and mii, see datasheet page 29)
   if(address & 0x80)
   {
      dat1 = enc28j60_spi_read();
   }
   // release CS
   enc28j60_spi_cs(1);
   return(dat1);
}

static void enc28j60WriteOp(uint8_t op, uint8_t address, uint8_t mydat)
{
   uint8_t opcode = op | (address & ADDR_MASK);
   enc28j60_spi_cs(0);
   // issue write command
   delay_100ns();
   enc28j60_spi_write( opcode);
   // write data
   enc28j60_spi_write(mydat);
   enc28j60_spi_cs(1);
   delay_100ns();
}

static void enc28j60SetBank(uint8_t address)
{
   // set the bank (if needed)
   if((address & BANK_MASK) != Enc28j60Bank)
   {
      // set the bank
      enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, (ECON1_BSEL1|ECON1_BSEL0));
      enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, (address & BANK_MASK)>>5);
      Enc28j60Bank = (address & BANK_MASK);
   }
}

static uint8_t enc28j60Read(uint8_t address)
{
   // select bank to read
   enc28j60SetBank(address);
   // do the read
   return enc28j60ReadOp(ENC28J60_READ_CTRL_REG, address);
}

static void enc28j60Write(uint8_t address, uint8_t mydat)
{
   // select bank to write
   enc28j60SetBank(address);
   // do the write
   enc28j60WriteOp(ENC28J60_WRITE_CTRL_REG, address, mydat);
}

static void enc28j60PhyWrite(uint8_t address, uint16_t mydat)
{
   // set the PHY register address
   enc28j60Write(MIREGADR, address);
   // write the PHY data
   enc28j60Write(MIWRL, mydat & 0x00ff);
   enc28j60Write(MIWRH, mydat >> 8);
   // wait until the PHY write completes
   while(enc28j60Read(MISTAT) & MISTAT_BUSY)
   {
      delay_100ns();
   }
}

uint16 enc28j60PhyRead(uint8_t address)
{
   uint16 data = 0;
   // set the PHY register address
   enc28j60Write(MIREGADR, address);

   enc28j60Write(MICMD, MICMD_MIIRD);
   enc28j60Write(MISTAT, MISTAT_BUSY);

   while(enc28j60Read(MISTAT) & MISTAT_BUSY)
   {
      delay_100ns();
   }

   enc28j60Write(MICMD, MICMD_MIIRD);

   data = enc28j60Read(MIWRH);
   data = data<<8;
   data = data |enc28j60Read(MIWRL);

   return data;
}

static void enc28j60ReadBuffer(uint16_t len, uint8_t* dat)
{
   // assert CS
   enc28j60_spi_cs(0);
   // issue read command
   delay_100ns();
   enc28j60_spi_write(ENC28J60_READ_BUF_MEM);
   while(len--)
   {
      *dat++ = enc28j60_spi_read();
   }
   // release CS
   enc28j60_spi_cs(1);
}

static void enc28j60WriteBuffer(uint16_t len, uint8_t* dat)
{
   // assert CS
   enc28j60_spi_cs(0);
   // issue write command
   enc28j60_spi_write(ENC28J60_WRITE_BUF_MEM);
   // while(!(SPSR & (1<<SPIF)));
   while(len--)
   {
      enc28j60_spi_write(*dat++);
   }
   // release CS
   enc28j60_spi_cs(1);
}

static void enc28j60PacketSend(uint16_t len, uint8_t* packet)
{
   // Set the write pointer to start of transmit buffer area
   enc28j60Write(EWRPTL, TXSTART_INIT & 0xFF);
   enc28j60Write(EWRPTH, TXSTART_INIT>>8);
   // Set the TXND pointer to correspond to the packet size given
   enc28j60Write(ETXNDL, (TXSTART_INIT+len));
   enc28j60Write(ETXNDH, (TXSTART_INIT+len)>>8);
   // write per-packet control byte
   enc28j60WriteOp(ENC28J60_WRITE_BUF_MEM, 0, 0x00);
   // TODO, fix this up
   if( uip_len <= TOTAL_HEADER_LENGTH )
   {
      // copy the packet into the transmit buffer
      enc28j60WriteBuffer(len, packet);
   }
   else
   {
      len -= TOTAL_HEADER_LENGTH;
      enc28j60WriteBuffer(TOTAL_HEADER_LENGTH, packet);
      enc28j60WriteBuffer(len, (unsigned char *)uip_appdata);
   }
   // send the contents of the transmit buffer onto the network
   enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);
}

static uint16_t enc28j60PacketReceive(uint16_t maxlen, uint8_t* packet)
{
   uint16_t rxstat,len;
   if( !(enc28j60Read(EIR) & EIR_PKTIF) )
   {
      if (enc28j60Read(EPKTCNT) == 0)
      {
         return 0;
      }
   }
   // Set the read pointer to the start of the received packet
   enc28j60Write(ERDPTL, (NextPacketPtr));
   enc28j60Write(ERDPTH, (NextPacketPtr)>>8);
   // read the next packet pointer
   NextPacketPtr  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
   NextPacketPtr |= (enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8);
   // read the packet length
   len  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
   len |= (enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8);
   len -= 4; //以太帧最小46字节 减去4字节的FCS校验和 加上帧头14字节 共64字节
   //PrintHex(len);
   // read the receive status
   rxstat  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
   rxstat |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
   // limit retrieve length
   // (we reduce the MAC-reported length by 4 to remove the CRC)

   if (len > maxlen)
   {
      len = maxlen;
   }

   // copy the packet from the receive buffer
   enc28j60ReadBuffer(len, packet);
   // Errata workaround #13. Make sure ERXRDPT is odd
   uint16_t rs,re;
   rs = enc28j60Read(ERXSTH);
   rs <<= 8;
   rs |= enc28j60Read(ERXSTL);
   re = enc28j60Read(ERXNDH);
   re <<= 8;
   re |= enc28j60Read(ERXNDL);
   if (NextPacketPtr - 1 < rs || NextPacketPtr - 1 > re)
   {
      enc28j60Write(ERXRDPTL, (re));
      enc28j60Write(ERXRDPTH, (re)>>8);
   }
   else
   {
      enc28j60Write(ERXRDPTL, (NextPacketPtr-1));
      enc28j60Write(ERXRDPTH, (NextPacketPtr-1)>>8);
   }
   // decrement the packet counter indicate we are done with this packet
   enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);
   return len;
}

static void enc28j60_init(void)
{
   Enc28j60Bank = 0xFF;
   //SPI INIT
   enc28j60_spi_init();

   delay_ms(2000);

#ifndef PROTOTYPE
   GPIOPinWrite(GPIO_D_BASE, GPIO_PIN_2, 0);
   delay_ms(300);
   GPIOPinWrite(GPIO_D_BASE, GPIO_PIN_2, GPIO_PIN_2);
#else
   // perform system reset
   enc28j60WriteOp(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
#endif

   // check CLKRDY bit to see if reset is complete
   //while(!(enc28j60Read(ESTAT) & ESTAT_CLKRDY));
   // Errata workaround #2, CLKRDY check is unreliable, delay 1 mS instead
   while(!(enc28j60Read(ESTAT) & ESTAT_CLKRDY));
   // lamp test
   // enc28j60PhyWrite(PHLCON, 0x0AA2);

   // do bank 0 stuff
   // initialize receive buffer
   // 16-bit transfers, must write low byte first
   // set receive buffer start address
   NextPacketPtr = RXSTART_INIT;
   enc28j60Write(ERXSTL, RXSTART_INIT&0xFF);
   enc28j60Write(ERXSTH, RXSTART_INIT>>8);
   // set receive pointer address
   enc28j60Write(ERXRDPTL, RXSTART_INIT&0xFF);
   enc28j60Write(ERXRDPTH, RXSTART_INIT>>8);
   // set receive buffer end
   // ERXND defaults to 0x1FFF (end of ram)
   enc28j60Write(ERXNDL, RXSTOP_INIT&0xFF);
   enc28j60Write(ERXNDH, RXSTOP_INIT>>8);
   // set transmit buffer start

   // ETXST defaults to 0x0000 (beginnging of ram)
   enc28j60Write(ETXSTL, TXSTART_INIT&0xFF);
   enc28j60Write(ETXSTH, TXSTART_INIT>>8);


   enc28j60Write(ETXNDL, TXSTOP_INIT&0xFF);
   enc28j60Write(ETXNDH, TXSTOP_INIT>>8);

   enc28j60Write(ERXFCON, ERXFCON_UCEN|ERXFCON_CRCEN|ERXFCON_PMEN|ERXFCON_BCEN);
   enc28j60Write(EPMM0, 0x3f);
   enc28j60Write(EPMM1, 0x30);
   enc28j60Write(EPMCSL, 0xf9);
   enc28j60Write(EPMCSH, 0xf7);


   // do bank 2 stuff
   // enable MAC receive
   enc28j60Write(MACON1, MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);
   // bring MAC out of reset
   enc28j60Write(MACON2, 0x00);
   // enable automatic padding and CRC operations
   //enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, MACON3, MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN);
   enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, MACON3, MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN|MACON3_FULDPX);
   // set inter-frame gap (non-back-to-back)
   enc28j60Write(MAIPGL, 0x12);
   enc28j60Write(MAIPGH, 0x0C);
   // set inter-frame gap (back-to-back)
   enc28j60Write(MABBIPG, 0x15);
   // Set the maximum packet size which the controller will accept
   enc28j60Write(MAMXFLL, MAX_FRAMELEN&0xFF);
   enc28j60Write(MAMXFLH, MAX_FRAMELEN>>8);
   // do bank 3 stuff
   // write MAC address
   // NOTE: MAC address in ENC28J60 is byte-backward
   enc28j60Write(MAADR5, UIP_ETHADDR0); //0x12
   enc28j60Write(MAADR4, UIP_ETHADDR1);  //0x34
   enc28j60Write(MAADR3, UIP_ETHADDR2);  //56
   enc28j60Write(MAADR2, UIP_ETHADDR3);  //78
   enc28j60Write(MAADR1, UIP_ETHADDR4);  //90
   enc28j60Write(MAADR0, UIP_ETHADDR5);

   uint8_t value = enc28j60Read(MAADR0);

   enc28j60PhyWrite(PHCON1, PHCON1_PDPXMD);
   enc28j60PhyWrite(PHLCON,0x0476);


   // no loopback of transmitted frames
   enc28j60PhyWrite(PHCON2, PHCON2_HDLDIS);
   // switch to bank 0
   enc28j60SetBank(ECON1);
   // enable interrutps

   enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE|EIE_PKTIE|EIE_RXERIE);

   // enable packet reception
   enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);
}

void dev_init(void)
{
   enc28j60_init();
}

void dev_send(void)
{
   enc28j60PacketSend(uip_len, uip_buf);
}

uint16_t dev_poll(void)
{
   return enc28j60PacketReceive(UIP_BUFSIZE, uip_buf);
}
