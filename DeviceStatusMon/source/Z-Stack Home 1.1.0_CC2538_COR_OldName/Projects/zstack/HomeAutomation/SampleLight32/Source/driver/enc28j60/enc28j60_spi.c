#include "enc28j60_spi.h"

void enc28j60_spi_cs(uint8_t high)
{
   if (high)
   {
      GPIOPinWrite(SPI_CS_GPIO_BASE, SPI_CS_GPIO_PIN, SPI_CS_GPIO_PIN);
   }
   else
   {
      GPIOPinWrite(SPI_CS_GPIO_BASE, SPI_CS_GPIO_PIN, 0);
   }
}

void enc28j60_spi_write(uint8_t data)
{
  uint32_t value;
  SSIDataPut(BSP_SPI_SSI_BASE, data);
  while (SSIBusy(BSP_SPI_SSI_BASE));
  SSIDataGet(BSP_SPI_SSI_BASE, &value);
}

uint8_t enc28j60_spi_read(void)
{
  uint32_t value;
  SSIDataPut(BSP_SPI_SSI_BASE, 0xCC);
  while (SSIBusy(BSP_SPI_SSI_BASE));
  SSIDataGet(BSP_SPI_SSI_BASE, &value);
  return (uint8_t)value;
}
/*
   -------------------------------------------------------
   3V   |   P2.1   | P1.5(SCLK) | P1.6(MOSI) | P1.7£¨MISO£©
   -------------------------------------------------------
   GND  |   P2.2   | P1.4(CSN)  |     RST    |
   -------------------È±    ¿Ú----------------------------
   */

void enc28j60_spi_init(void)
{
  bspSpiInit(BSP_SPI_CLK_SPD/8);

  //
  // Enable 3.3-V domain
  //
  //bsp3V3DomainEnable();

  //
  // Configure CSn and MODE(A0) as output high
  //
  //GPIOPinTypeGPIOOutput(GPIO_B_BASE, GPIO_PIN_5);

    // Added by xiazhituo Set Reset Pin High
  GPIOPinTypeGPIOOutput(GPIO_D_BASE, GPIO_PIN_2);
  GPIOPinWrite(GPIO_D_BASE, GPIO_PIN_2, GPIO_PIN_2);

  GPIOPinTypeGPIOOutput(SPI_CS_GPIO_BASE,  SPI_CS_GPIO_PIN);
}
