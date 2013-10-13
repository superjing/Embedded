/*       ษออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
         บ    TITLE:  RTL8019AS ethernet routines for 8051 and      บ
         บ            Keil C51 v7.00 port of Adam Dunkels' uIP v0.9 บ
         บ FILENAME:  etherdev.c                                    บ
         บ REVISION:  VER 0.0                                       บ
         บ REV.DATE:  21-01-05                                      บ
         บ  ARCHIVE:                                                บ
         บ   AUTHOR:  Copyright (c) 2005, Murray R. Van Luyn.       บ
         บ   Refer to National Semiconductor DP8390 App Note 874.   บ
         ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ        */

/*  ฺฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
    ณ  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS  ณ 
    ณ  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED  ณ 
    ณ  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ณ 
    ณ  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY    ณ 
    ณ  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL ณ 
    ณ  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE  ณ 
    ณ  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS      ณ 
    ณ  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,       ณ 
    ณ  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING          ณ 
    ณ  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS ณ 
    ณ  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.       ณ 
    ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤู  */


#include "rtl8019as.h"

/*  ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
    บ                            Private defines.                         บ
    ศอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ  */
#define ETH_CPU_CLOCK      ETH_CPU_XTAL / 12    // 8051 clock rate (X1 mode)

// Delay routine timing parameters
#define ETH_DELAY_CONST    9.114584e-5          // Delay routine constant
#define ETH_DELAY_MULTPLR  (unsigned char)(ETH_DELAY_CONST * ETH_CPU_CLOCK)

// X1 CPU mode timing parameters
#define ETH_T0_CLOCK             ETH_CPU_XTAL / 12 // Timer 0 mode 1 clock rate
#define ETH_T0_INT_RATE          24                // Timer 0 intrupt rate (Hz)
#define ETH_T0_RELOAD            65536 - (ETH_T0_CLOCK / ETH_T0_INT_RATE)

// Packet transmit & receive buffer configuration
#define ETH_TX_PAGE_START  0x40    // 0x4000 Tx buffer is  6 * 256 = 1536 bytes
#define ETH_RX_PAGE_START  0x46    // 0x4600 Rx buffer is 26 * 256 = 6656 bytes
#define ETH_RX_PAGE_STOP   0x60    // 0x6000

#define ETH_ADDR_PORT_MASK 0x1F                 // 00011111y
#define ETH_DATA_PORT_MASK 0xFF                 // 11111111y

#define ETH_MIN_PACKET_LEN 0x3C


/*  ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
    บ                       Private Function Prototypes                   บ
    ศอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ  */
static void etherdev_reg_write(unsigned char reg, unsigned char wr_data);
static unsigned char etherdev_reg_read(unsigned char reg);
static void etherdev_delay_ms(unsigned int count);
static unsigned int etherdev_poll(void);
//static void etherdev_timer0_isr(void) interrupt 1 using 1;


/*  ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
    บ                          Private Macro Defines                      บ
    ศอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ  */
// Manipulate PS1 & PS0 in CR to select RTL8019AS register page. 
#define ETHERDEV_SELECT_REG_PAGE(page)                                      \
          do                                                                \
          {                                                                 \
              etherdev_reg_write(CR, etherdev_reg_read(CR) & ~(PS1 | PS0)); \
              etherdev_reg_write(CR, etherdev_reg_read(CR) | (page << 6));  \
          } while(0)


/*  ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
    บ                              Global Variables                       บ
    ศอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ  */
static unsigned char tick_count = 0;


// NIC is mapped from address 0x8000 - 0x9F00
#define MEMORY_MAPPED_RTL8019_OFFSET 0x8800// 0x8FE0

/*  ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
    บ                     Private Function Implementation                 บ
    ศอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ  */

/*  ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
    บ                                                                     บ
    บ                          etherdev_reg_write()                       บ
    บ                                                                     บ
    บ                                                                     บ
    ศอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ  */
static void etherdev_reg_write(unsigned char reg, unsigned char wr_data)
{

  unsigned char xdata *tmp;
//  tmp=MEMORY_MAPPED_RTL8019_OFFSET + (((unsigned char)(RTL_ADDRESS)) << 8);
  tmp=MEMORY_MAPPED_RTL8019_OFFSET + reg;
  *tmp=wr_data;
/*
    // Select register address.
    ETH_ADDR_PORT &= ~ETH_ADDR_PORT_MASK; 
    ETH_ADDR_PORT |= reg;

    // Output register data to port.
    ETH_DATA_PORT = wr_data;

    // Clock register data into RTL8019AS.
    // IOR & IOW are both active low.
    IOW = 0;
    IOW = 1;

    // Set register data port as input again.
    ETH_DATA_PORT = ETH_DATA_PORT_MASK;
*/
    return;
} 


/*  ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
    บ                                                                     บ
    บ                          etherdev_reg_read()                        บ
    บ                                                                     บ
    บ                                                                     บ
    ศอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ  */
static unsigned char etherdev_reg_read(unsigned char reg)
{
  unsigned char xdata *tmp;
//  tmp=MEMORY_MAPPED_RTL8019_OFFSET + (((unsigned char)(RTL_ADDRESS)) << 8);
  tmp=MEMORY_MAPPED_RTL8019_OFFSET + reg;
  return *tmp;
/*
    unsigned char rd_data;

    // Select register address.
    ETH_ADDR_PORT &= ~ETH_ADDR_PORT_MASK;
    ETH_ADDR_PORT |= reg;

    // Enable register data output from RTL8019AS.
    IOR = 0;

    // Read register data from port.
    rd_data = ETH_DATA_PORT;

    // Disable register data output from RTL8019AS.
    IOR = 1;    

    return rd_data;
*/
} 


/*  ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
    บ                                                                     บ
    บ                          etherdev_delay_ms()                        บ
    บ                                                                     บ
    บ  1 to 255+ ms delay.                                                บ
    บ                                                                     บ
    ศอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ  */
static void etherdev_delay_ms(unsigned int count)
{

    for(count *= ETH_DELAY_MULTPLR; count > 0; count--) continue;

    return;
}


/*  ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
    บ                                                                     บ
    บ                           etherdev_timer0_isr()                     บ
    บ                                                                     บ
    บ This function is invoked each 1/24th of a second and updates a      บ
    บ 1/24th of a second tick counter.                                    บ
    บ                                                                     บ
    ศอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ  */
static void etherdev_timer0_isr(void) interrupt 1 using 1
{
    // Reload timer/ counter 0 for 24Hz periodic interrupt.   
    TH0 = ETH_T0_RELOAD >> 8;
    TL0 = ETH_T0_RELOAD;

    // Increment 24ths of a second counter.
    tick_count++;

    return;
}


/*  ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
    บ                     Public Function Implementation                  บ
    ศอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ  */

/*  ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
    บ                                                                     บ
    บ                            etherdev_init()                          บ
    บ                                                                     บ
    บ  Returns: 1 on success, 0 on failure.                               บ
    บ  Refer to National Semiconductor DP8390 App Note 874, July 1993.    บ
    บ                                                                     บ
    ศอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ  */
sfr P4 = 0xe8;

sbit   RST8019  = P4 ^ 0;
bit rtl8019as_init(void)
{
    // Set IOR & IOW as they're active low.
//    IOR = 1;
 //   IOW = 1;

    // Set register data port as input.
//    ETH_DATA_PORT = ETH_DATA_PORT_MASK;

  xdata unsigned int COUNT;
  for(COUNT=0;COUNT<1000;COUNT++);
  RST8019=1;
//Delay_10ms(1);
  for(COUNT=0;COUNT<5000;COUNT++);
  RST8019=0;
  for(COUNT=0;COUNT<2000;COUNT++);

#ifdef ETH_DEBUG
    init_sio_poll();
#endif /* ETH_DEBUG */

    // Configure RTL8019AS ethernet controller.

    // Keil startup code takes 4ms to execute (18.432MHz, X1 mode).
    // That leaves plenty of time for the RTL8019AS to read it's
    // configuration in from the 9346 EEPROM before we get here.

    // Select RTL8019AS register page 0.
    ETHERDEV_SELECT_REG_PAGE(0);

    // Check if RTL8019AS fully reset.
    if(!(etherdev_reg_read(ISR) & RST))
    {
        return 0;
    }

    // Select RTL8019AS register page 3.
    ETHERDEV_SELECT_REG_PAGE(3);

    // Temporarily disable config register write protection.
    etherdev_reg_write(_9346CR, EEM1 | EEM0);

    // Disable boot ROM & select 10BaseT with TP/CX auto-detect.
    etherdev_reg_write(CONFIG2, BSELB);

    // Select half-duplex, awake, power-up & LED_TX/ LED_RX/ LED_LNK behaviour.
    etherdev_reg_write(CONFIG3, LEDS0);

    // Re-enable config register write protection.
    etherdev_reg_write(_9346CR, 0x00);

    // Select RTL8019AS register page 0.
    ETHERDEV_SELECT_REG_PAGE(0);

    // Stop RTL8019AS, select page 0 and abort DMA operation.
    etherdev_reg_write(CR, RD2 | STP);

    // Initialise data configuration register. 
    // FIFO threshold 8 bytes, no loopback, don't use auto send packet.
    etherdev_reg_write(DCR, FT1 | LS);

    // Reset remote byte count registers.
    etherdev_reg_write(RBCR0, 0x00);
    etherdev_reg_write(RBCR1, 0x00);

    // Receive configuration register to monitor mode.
    etherdev_reg_write(RCR, MON);

    // Initialise transmit configuration register to loopback internally.
    etherdev_reg_write(TCR, LB0);

    // Clear interrupt status register bits by writing 1 to each.
    etherdev_reg_write(ISR, 0xFF);

    // Mask all interrupts in mask register.
    etherdev_reg_write(IMR, 0x00);

    // Set transmit page start.
    etherdev_reg_write(TPSR, ETH_TX_PAGE_START);

    // Set receive buffer page start.
    etherdev_reg_write(PSTART, ETH_RX_PAGE_START);

    // Initialise last receive buffer read pointer.
    etherdev_reg_write(BNRY, ETH_RX_PAGE_START);

    // Set receive buffer page stop.
    etherdev_reg_write(PSTOP, ETH_RX_PAGE_STOP);

    // Select RTL8019AS register page 1.
    etherdev_reg_write(CR, RD2 | PS0 | STP);

    // Initialise current packet receive buffer page pointer
    etherdev_reg_write(CURR, ETH_RX_PAGE_START);

    // Set physical address
    etherdev_reg_write(PAR0, UIP_ETHADDR0);
    etherdev_reg_write(PAR1, UIP_ETHADDR1);
    etherdev_reg_write(PAR2, UIP_ETHADDR2);
    etherdev_reg_write(PAR3, UIP_ETHADDR3);
    etherdev_reg_write(PAR4, UIP_ETHADDR4);
    etherdev_reg_write(PAR5, UIP_ETHADDR5);

    // Select RTL8019AS register page 0 and abort DMA operation.
    etherdev_reg_write(CR, RD2 | STP);

    // Restart RTL8019AS. 
    etherdev_reg_write(CR, RD2 | STA);

    // Initialise transmit configuration register for normal operation.
    etherdev_reg_write(TCR, 0x00);

    // Receive configuration register to accept broadcast packets.
    etherdev_reg_write(RCR, AB);


    // Initialize Timer 0 to generate a periodic 24Hz interrupt. 

    // Stop timer/ counter 0.                                         
    TR0  = 0;          

    // Set timer/ counter 0 as mode 1 16 bit timer.      
    TMOD &= 0xF0;
    TMOD |= 0x01;

    // Preload for 24Hz periodic interrupt.    
    TH0 = ETH_T0_RELOAD >> 8; 
    TL0 = ETH_T0_RELOAD;

    // Restart timer/ counter 0 running.
    TR0 = 1;

    // Enable timer/ counter 0 overflow interrupt.            
    ET0 = 1;

    // Enable global interrupt.
    EA = 1;

    return 1;
}


/*  ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
    บ                                                                     บ
    บ                           etherdev_send()                           บ
    บ                                                                     บ
    บ Send the packet in the uip_buf and uip_appdata buffers using the    บ
    บ RTL8019AS ethernet card.                                            บ
    บ                                                                     บ
    ศอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ  */
void rtl8019as_send(void)
{
    unsigned int i;
    unsigned char *ptr;

    ptr = uip_buf;

    // Setup for DMA transfer from uip_buf & uip_appdata buffers to RTL8019AS.

    // Select RTL8019AS register page 0 and abort DMA operation.
    etherdev_reg_write(CR, RD2 | STA);

    // Wait until pending transmit operation completes.
    while(etherdev_reg_read(CR) & TXP) continue;

    // Clear remote DMA complete interrupt status register bit.
    etherdev_reg_write(ISR, RDC);

    // Set remote DMA start address registers to indicate where to load packet.
    etherdev_reg_write(RSAR0, 0x00);
    etherdev_reg_write(RSAR1, ETH_TX_PAGE_START);

    // Set remote DMA byte count registers to indicate length of packet load.
    etherdev_reg_write(RBCR0, (unsigned char)(uip_len & 0xFF));
    etherdev_reg_write(RBCR1, (unsigned char)(uip_len >> 8));

    // Initiate DMA transfer of uip_buf & uip_appdata buffers to RTL8019AS.
    etherdev_reg_write(CR, RD1 | STA);

    // DMA transfer packet from uip_buf & uip_appdata to RTL8019AS local
    // transmit buffer memory.
    for(i = 0; i < uip_len; i++)
    {
        if(i == 40 + UIP_LLH_LEN) 
        {
            ptr = (unsigned char *)uip_appdata;
        }

        etherdev_reg_write(RDMA, *ptr++);
    }

    // Wait until remote DMA operation complete.
    while(!(etherdev_reg_read(ISR) & RDC)) continue;

    // Abort/ complete DMA operation.
    etherdev_reg_write(CR, RD2 | STA);

    // Set transmit page start to indicate packet start.
    etherdev_reg_write(TPSR, ETH_TX_PAGE_START);

    // Ethernet packets must be > 60 bytes, otherwise are rejected as runts.
    if(uip_len < ETH_MIN_PACKET_LEN)
    {
        uip_len = ETH_MIN_PACKET_LEN;
    }

    // Set transmit byte count registers to indicate packet length.
    etherdev_reg_write(TBCR0, (unsigned char)(uip_len & 0xFF));
    etherdev_reg_write(TBCR1, (unsigned char)(uip_len >> 8));

    // Issue command for RTL8019AS to transmit packet from it's local buffer.
    etherdev_reg_write(CR, RD2 | TXP | STA);

    return;
}


/*  ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
    บ                                                                     บ
    บ                           etherdev_read()                           บ
    บ                                                                     บ
    บ This function will read an entire IP packet into the uip_buf.       บ
    บ If it must wait for more than 0.5 seconds, it will return with      บ
    บ the return value 0. Otherwise, when a full packet has been read     บ
    บ into the uip_buf buffer, the length of the packet is returned.      บ
    บ                                                                     บ
    ศอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ  */
unsigned int rtl8019as_read(void)
{    
    unsigned int bytes_read;

    /* tick_count threshold should be 12 for 0.5 sec bail-out
       One second (24) worked better for me, but socket recycling
       is then slower. I set UIP_TIME_WAIT_TIMEOUT 60 in uipopt.h
       to counter this. Retransmission timing etc. is affected also. */
    while ((!(bytes_read = etherdev_poll())) && (tick_count < 12)) continue;

    tick_count = 0;

    return bytes_read;
}


/*  ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
    บ                                                                     บ
    บ                           etherdev_poll()                           บ
    บ                                                                     บ
    บ Poll the RTL8019AS ethernet device for an available packet.         บ
    บ                                                                     บ
    ศอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ  */
static unsigned int etherdev_poll(void)
{
    unsigned int len = 0;

    // Check if there is a packet in the rx buffer.
    if(etherdev_reg_read(ISR) & PRX)
    {
        // Check if the rx buffer has overflowed.
        if(etherdev_reg_read(ISR) & OVW)
        {
            bit retransmit = 0;

            // If the receive buffer ring has overflowed we dump the whole
            // thing and start over. There is no way of knowing whether the
            // data it contains is uncorrupted, or will cause us grief.

            // Stop RTL8019AS and abort DMA operation.
            etherdev_reg_write(CR, RD2 | STP);

            // Reset remote byte count registers.
            etherdev_reg_write(RBCR0, 0x00);
            etherdev_reg_write(RBCR1, 0x00);

            // Wait for controller to halt after any current tx completes.
            while(!(etherdev_reg_read(ISR) & RST)) continue;

            // Check whether currently transmitting a packet.
            if(etherdev_reg_read(CR) & TXP)
            {
                // If neither a successful transmission nor a tx abort error 
                // has occured, then flag current tx packet for resend.
                if(!((etherdev_reg_read(ISR) & PTX)
                                          || (etherdev_reg_read(ISR) & TXE)))
                {
                    retransmit = 1;
                }
            }

            // Set transmit configuration register to loopback internally.
            etherdev_reg_write(TCR, LB0);

            // Restart the RTL8019AS.
            etherdev_reg_write(CR, RD2 | STA);

            // Re-initialise last receive buffer read pointer.
            etherdev_reg_write(BNRY, ETH_RX_PAGE_START);

            // Select RTL8019AS register page 1.
            ETHERDEV_SELECT_REG_PAGE(1);

            // Re-initialise current packet receive buffer page pointer.
            etherdev_reg_write(CURR, ETH_RX_PAGE_START);

            // Select RTL8019AS register page 0.
            ETHERDEV_SELECT_REG_PAGE(0);

            // Clear rx buffer overflow & packet received interrupt flags.
            etherdev_reg_write(ISR, PRX | OVW);

            // Re-itialise transmit configuration reg for normal operation.
            etherdev_reg_write(TCR, 0x00);
        
            if(retransmit)
            {
                // Retransmit packet in RTL8019AS local tx buffer.
                etherdev_reg_write(CR, RD2 | TXP | STA);
            }
        }
        else // Rx buffer has not overflowed, so read a packet into uip_buf.
        {
            unsigned int i;
            unsigned char next_rx_packet;
            unsigned char current;

            // Retrieve packet header. (status, next_ptr, length_l, length_h)

            // Clear remote DMA complete interrupt status register bit.
            etherdev_reg_write(ISR, RDC);

            // Set remote DMA start address registers to packet header.
            etherdev_reg_write(RSAR0, 0x00);
            etherdev_reg_write(RSAR1, etherdev_reg_read(BNRY));

            // Set remote DMA byte count registers to packet header length.
            etherdev_reg_write(RBCR0, 0x04);
            etherdev_reg_write(RBCR1, 0x00);

            // Initiate DMA transfer of packet header.
            etherdev_reg_write(CR, RD0 | STA);

            // Drop packet status. We don't use it.
            etherdev_reg_read(RDMA);

            // Save next packet pointer.
            next_rx_packet = etherdev_reg_read(RDMA);

            // Retrieve packet data length and subtract CRC bytes.
            len =  etherdev_reg_read(RDMA);
            len += etherdev_reg_read(RDMA) << 8;
            len -= 4;

            // Wait until remote DMA operation completes.
            while(!(etherdev_reg_read(ISR) & RDC)) continue;

            // Abort/ complete DMA operation.
            etherdev_reg_write(CR, RD2 | STA);


            // Retrieve packet data.

            // Check if incoming packet will fit into rx buffer.
            if(len <= UIP_BUFSIZE)
            {
                // Clear remote DMA complete interrupt status register bit.
                etherdev_reg_write(ISR, RDC);

                // Set remote DMA start address registers to packet data.
                etherdev_reg_write(RSAR0, 0x04);
                etherdev_reg_write(RSAR1, etherdev_reg_read(BNRY));

                // Set remote DMA byte count registers to packet data length.
                etherdev_reg_write(RBCR0, (unsigned char)(len & 0xFF));
                etherdev_reg_write(RBCR1, (unsigned char)(len >> 8));

                // Initiate DMA transfer of packet data.
                etherdev_reg_write(CR, RD0 | STA);

                // Read packet data directly into uip_buf.
                for(i = 0; i < len; i++)
                {
                    *(uip_buf + i) = etherdev_reg_read(RDMA);
                }

                // Wait until remote DMA operation complete.
                while(!(etherdev_reg_read(ISR) & RDC)) continue;

                // Abort/ complete DMA operation.
                etherdev_reg_write(CR, RD2 | STA);

            }
            else
            {
                // Incoming packet too big, so dump it.
                len = 0;
            }

            // Advance boundary pointer to next packet start.
            etherdev_reg_write(BNRY, next_rx_packet);

            // Select RTL8019AS register page 1.
            ETHERDEV_SELECT_REG_PAGE(1);

            // Retrieve current receive buffer page
            current = etherdev_reg_read(CURR);

            // Select RTL8019AS register page 0.
            ETHERDEV_SELECT_REG_PAGE(0);

            // Check if last packet has been removed from rx buffer.
            if(next_rx_packet == current)
            {
                // Clear packet received interrupt flag.
                etherdev_reg_write(ISR, PRX);
            }
        }
    }

    return len;
}








