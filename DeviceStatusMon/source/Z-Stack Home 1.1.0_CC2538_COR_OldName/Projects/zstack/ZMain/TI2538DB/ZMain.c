/******************************************************************************
  Filename:       ZMain.c
  Revised:        $Date$
  Revision:       $Revision$

  Description:    Startup and task processing code for ZStack
  Notes:          This version targets the Texas Instruments CC2538


  Copyright 2012-2013 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License"). You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product. Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
******************************************************************************/

/******************************************************************************
 * INCLUDES
 */

#include "ZComDef.h"
#include "OSAL.h"
#include "OSAL_Nv.h"
#include "OnBoard.h"
#include "ZMAC.h"
#include "string.h"

#ifndef NONWK
  #include "AF.h"
#endif

/* Hal */
#include "hal_drivers.h"
#include "hal_lcd.h"
#include "hal_systick.h"

#include "enc28j60.h"
#include "uip.h"
#include "uip_arp.h"
#include "clock_arch.h"
#include "uip_tcpapp.h"
#include "Watchdog.h"

#define BUF ((struct uip_eth_hdr *)&uip_buf[0])

/******************************************************************************
 * LOCAL FUNCTIONS
 */
#ifdef LCD_SUPPORTED
static void zmain_dev_info( void );
#endif

static void zmain_ext_addr( void );

#if defined ZCL_KEY_ESTABLISH
static void zmain_cert_init( void );
#endif

#ifdef LCD_SUPPORTED
static void zmain_lcd_init( void );
#endif

u8_t arptimer ;

static void pollEnc28j60(void)
{
   u8_t i;
   u16_t j;

   /* Let the tapdev network device driver read an entire IP packet
      into the uip_buf. If it must wait for more than 0.5 seconds, it
      will return with the return value 0. If so, we know that it is
      time to call upon the uip_periodic(). Otherwise, the tapdev has
      received an IP packet that is to be processed by uIP. */
   uip_len = dev_poll();

   for (j = 0; j < 500; j++);

   if (uip_len == 0)
   {
      for (i = 0; i < UIP_CONNS; i++)
      {
         uip_periodic(i);

         /* If the above function invocation resulted in data that
            should be sent out on the network, the global variable
            uip_len is set to a value > 0. */
         if(uip_len > 0)
         {
            uip_arp_out();
            dev_send();
         }
      }

#if UIP_UDP
      for (i = 0; i < UIP_UDP_CONNS; i++)
      {
         uip_udp_periodic(i);
         /* If the above function invocation resulted in data that
            should be sent out on the network, the global variable
            uip_len is set to a value > 0. */
         if (uip_len > 0)
         {
            uip_arp_out();
            dev_send();
         }
      }
#endif /* UIP_UDP */

      /* Call the ARP timer function every 10 seconds. */
      if (++(arptimer) == 20)
      {
         uip_arp_timer();
         arptimer = 0;
      }

   }
   else
   {
      if (BUF->type == htons(UIP_ETHTYPE_IP))
      {
         uip_arp_ipin();
         uip_input();

         /* If the above function invocation resulted in data that
            should be sent out on the network, the global variable
            uip_len is set to a value > 0. */
         if (uip_len > 0)
         {
            uip_arp_out();
            dev_send();
         }
      }
      else if (BUF->type == htons(UIP_ETHTYPE_ARP))
      {
         // LED = 0;
         uip_arp_arpin();

         /* If the above function invocation resulted in data that
            should be sent out on the network, the global variable
            uip_len is set to a value > 0. */
         if(uip_len > 0)
         {
            dev_send();
         }
      }
   }
}

extern char dhcpdone;
/******************************************************************************
 * @fn      main
 *
 * @brief   First function called after startup.
 *
 * @param   None
 *
 * @return  Don't care
 */
int main( void )
{
  // Turn off interrupts
  osal_int_disable( INTS_ALL );

  // Initialization for board related stuff such as LEDs
  HAL_BOARD_INIT();

  // Initialize board I/O
  InitBoard( OB_COLD );

  // Initialze HAL drivers
  HalDriverInit();

  // Initialize NV System
  osal_nv_init( NULL );

  // Initialize the MAC
  ZMacInit();

  // Determine the extended address
  zmain_ext_addr();

#if defined ZCL_KEY_ESTABLISH
  // Initialize the Certicom certificate information.
  zmain_cert_init();
#endif

  // Initialize basic NV items
  zgInit();

#ifndef NONWK
  // Since the AF isn't a task, call it's initialization routine
  afInit();
#endif

  // Initialize the operating system
  osal_init_system();

  // Allow interrupts
  osal_int_enable( INTS_ALL );

  SysTickSetup();

  // Final board initialization
  InitBoard( OB_READY );

  /* Display the device info on the LCD */
#ifdef LCD_SUPPORTED
  zmain_dev_info();
  zmain_lcd_init();
#endif

#ifndef PROTOTYPE
  GPIOPinTypeGPIOOutput(GPIO_B_BASE, GPIO_PIN_1);
  GPIOPinTypeGPIOOutput(GPIO_B_BASE, GPIO_PIN_2);
  GPIOPinTypeGPIOOutput(GPIO_B_BASE, GPIO_PIN_3);
  GPIOPinTypeGPIOOutput(GPIO_D_BASE, GPIO_PIN_2);
  GPIOPinTypeGPIOOutput(GPIO_A_BASE, GPIO_PIN_3);
  GPIOPinWrite(GPIO_B_BASE, GPIO_PIN_3, 0);
  GPIOPinWrite(GPIO_B_BASE, GPIO_PIN_2, GPIO_PIN_2);
#endif

  arptimer = 0;

  dev_init();

  uip_arp_init();

  // Initialize the uIP TCP/IP stack.
  uip_init();

  // Use zigbee IEEE Mac address as dhcp Mac address
  dhcpc_init(aExtendedAddress + 2, 6);

  WatchdogEnable( WATCHDOG_INTERVAL_32768 );

  for(;;)
  {
    osal_run_system();
    pollEnc28j60();
    if (dhcpdone)
    {
      tcp_ipaddr_init();
      tpc_app_init();
      dhcpdone = 0;
    }
  }

  //osal_start_system(); // No Return from here
} // main()

/******************************************************************************
 * @fn      zmain_ext_addr
 *
 * @brief   Execute a prioritized search for a valid extended address and write
 *          the results into the OSAL NV memory. Temporary IEEE address is not
 *          saved to NV.
 *
 * @param   none
 *
 * @return  none
 */
static void zmain_ext_addr( void )
{
  uint8 nullAddr[Z_EXTADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint8 writeNV = TRUE;

  // First check whether a non-erased extended address exists in the OSAL NV.
  if ((SUCCESS != osal_nv_item_init(ZCD_NV_EXTADDR, Z_EXTADDR_LEN, NULL))  ||
      (SUCCESS != osal_nv_read(ZCD_NV_EXTADDR, 0, Z_EXTADDR_LEN, aExtendedAddress)) ||
      (osal_memcmp(aExtendedAddress, nullAddr, Z_EXTADDR_LEN)))
  {
    // Attempt to read the extended address from the location in the last flash
    // page where the commissioning tools know to reserve it.
    if (!osal_memcmp((uint8 *)HAL_FLASH_IEEE_ADDR, nullAddr, Z_EXTADDR_LEN))
    {
      (void)osal_memcpy(aExtendedAddress, (uint8 *)HAL_FLASH_IEEE_ADDR, Z_EXTADDR_LEN);
    }
    else
    {
      // Disable prefetch when reading from Information Page.
      uint32 fctl = HWREG(FLASH_CTRL_FCTL);
      HWREG(FLASH_CTRL_FCTL) = fctl & ~(FLASH_CTRL_FCTL_PREFETCH_ENABLE);

      // Attempt to read the extended address from the Information Page.
      if (!osal_memcmp((uint8 *)HAL_INFO_IEEE_ADDR, nullAddr, Z_EXTADDR_LEN))
      {
        (void)osal_memcpy(aExtendedAddress, (uint8 *)HAL_INFO_IEEE_ADDR, Z_EXTADDR_LEN);
      }
      else  // No valid extended address was found.
      {
        uint8 idx;

#if !defined ( NV_RESTORE )
        writeNV = FALSE;  // Make a temporary IEEE address, not saved in NV
#endif

       /* Create a sufficiently random extended address for expediency.
        * Note: this is only valid/legal in a test environment and
        *       must never be used for a commercial product.
        */
        for (idx = 0; idx < (Z_EXTADDR_LEN - 2);)
        {
          uint16 randy = osal_rand();
          aExtendedAddress[idx++] = LO_UINT16(randy);
          aExtendedAddress[idx++] = HI_UINT16(randy);
        }
        // Next-to-MSB identifies ZigBee device type.
#if ZG_BUILD_COORDINATOR_TYPE && !ZG_BUILD_JOINING_TYPE
        aExtendedAddress[idx++] = 0x10;
#elif ZG_BUILD_RTRONLY_TYPE
        aExtendedAddress[idx++] = 0x20;
#else
      aExtendedAddress[idx++] = 0x30;
#endif
      // MSB has historical signficance.
        aExtendedAddress[idx] = 0xF8;
      }

      // Restore flash control to previous state
      HWREG(FLASH_CTRL_FCTL) = fctl;
    }

    if (writeNV)
    {
      (void)osal_nv_write(ZCD_NV_EXTADDR, 0, Z_EXTADDR_LEN, aExtendedAddress);
    }
  }

  // Set the MAC PIB extended address according to results from above.
  (void)ZMacSetReq(MAC_EXTENDED_ADDRESS, aExtendedAddress);
}

#if defined ZCL_KEY_ESTABLISH
/******************************************************************************
 * @fn      zmain_cert_init
 *
 * @brief   Initialize the Certicom certificate information.
 *
 * @param   none
 *
 * @return  none
 */
static void zmain_cert_init( void )
{
  uint8 certData[ZCL_KE_IMPLICIT_CERTIFICATE_LEN];
  uint8 nullData[ZCL_KE_IMPLICIT_CERTIFICATE_LEN] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
  };

  (void)osal_nv_item_init(ZCD_NV_IMPLICIT_CERTIFICATE, ZCL_KE_IMPLICIT_CERTIFICATE_LEN, NULL);
  (void)osal_nv_item_init(ZCD_NV_DEVICE_PRIVATE_KEY, ZCL_KE_DEVICE_PRIVATE_KEY_LEN, NULL);

  // First, check whether non-null certificate data already exists in the OSAL NV.
  // To save on code space, just use the CA_PUBLIC_KEY as the bellwether for all three.
  if ((SUCCESS != osal_nv_item_init(ZCD_NV_CA_PUBLIC_KEY, ZCL_KE_CA_PUBLIC_KEY_LEN, NULL))   ||
      (SUCCESS != osal_nv_read(ZCD_NV_CA_PUBLIC_KEY, 0, ZCL_KE_CA_PUBLIC_KEY_LEN, certData)) ||
      (osal_memcmp(certData, nullData, ZCL_KE_CA_PUBLIC_KEY_LEN)))
  {
    // If the certificate data is not NULL, use it to update the corresponding NV items.
    if (!osal_memcmp((uint8 *)HAL_FLASH_CA_PUBLIC_KEY_ADDR, nullData, ZCL_KE_CA_PUBLIC_KEY_LEN))
    {
      (void)osal_memcpy(certData, (uint8 *)HAL_FLASH_CA_PUBLIC_KEY_ADDR, ZCL_KE_CA_PUBLIC_KEY_LEN);
      (void)osal_nv_write(ZCD_NV_CA_PUBLIC_KEY, 0, ZCL_KE_CA_PUBLIC_KEY_LEN, certData);

      (void)osal_memcpy(certData, (uint8 *)HAL_FLASH_IMPLICIT_CERT_ADDR, ZCL_KE_IMPLICIT_CERTIFICATE_LEN);
      (void)osal_nv_write(ZCD_NV_IMPLICIT_CERTIFICATE, 0, ZCL_KE_IMPLICIT_CERTIFICATE_LEN, certData);

      (void)osal_memcpy(certData, (uint8 *)HAL_FLASH_DEV_PRIVATE_KEY_ADDR, ZCL_KE_DEVICE_PRIVATE_KEY_LEN);
      (void)osal_nv_write(ZCD_NV_DEVICE_PRIVATE_KEY, 0, ZCL_KE_DEVICE_PRIVATE_KEY_LEN, certData);
    }
  }
}
#endif

#ifdef LCD_SUPPORTED
/******************************************************************************
 * @fn      zmain_dev_info
 *
 * @brief   Displays the IEEE address (MSB to LSB) on the LCD.
 *
 * @param   none
 *
 * @return  none
 */
static void zmain_dev_info( void )
{
  uint8 i;
  uint8 *xad;
  uint8 lcd_buf[(Z_EXTADDR_LEN*2)+1];

  // Display the extended address.
  xad = aExtendedAddress + Z_EXTADDR_LEN - 1;

  for (i = 0; i < Z_EXTADDR_LEN*2; xad--)
  {
    uint8 ch;
    ch = (*xad >> 4) & 0x0F;
    lcd_buf[i++] = ch + (( ch < 10 ) ? '0' : '7');
    ch = *xad & 0x0F;
    lcd_buf[i++] = ch + (( ch < 10 ) ? '0' : '7');
  }
  lcd_buf[Z_EXTADDR_LEN*2] = '\0';
  HalLcdWriteString( "IEEE: ", HAL_LCD_LINE_2 );
  HalLcdWriteString( (char*)lcd_buf, HAL_LCD_LINE_3 );
}
#endif

#ifdef LCD_SUPPORTED
/*********************************************************************
 * @fn      zmain_lcd_init
 * @brief   Initialize LCD at start up.
 * @return  none
 *********************************************************************/
static void zmain_lcd_init( void )
{
#ifdef SERIAL_DEBUG_SUPPORTED
  {
    HalLcdWriteString( "TexasInstruments", HAL_LCD_LINE_1 );

#if defined( MT_MAC_FUNC )
#if defined( ZDO_COORDINATOR )
      HalLcdWriteString( "MAC-MT Coord", HAL_LCD_LINE_2 );
#else
      HalLcdWriteString( "MAC-MT Device", HAL_LCD_LINE_2 );
#endif // ZDO
#elif defined( MT_NWK_FUNC )
#if defined( ZDO_COORDINATOR )
      HalLcdWriteString( "NWK Coordinator", HAL_LCD_LINE_2 );
#else
      HalLcdWriteString( "NWK Device", HAL_LCD_LINE_2 );
#endif // ZDO
#endif // MT_FUNC
  }
#endif // SERIAL_DEBUG_SUPPORTED
}
#endif

/******************************************************************************
 */
