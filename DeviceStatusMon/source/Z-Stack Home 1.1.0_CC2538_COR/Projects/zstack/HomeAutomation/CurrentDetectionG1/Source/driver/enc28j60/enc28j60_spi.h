#ifndef ENC28j60_SPI_H
#define ENC28j60_SPI_H

#include "bsp.h"
#include "ssi.h"
#include "gpio.h"

#ifdef PROTOTYPE
#define SPI_CS_GPIO_BASE     GPIO_B_BASE
#define SPI_CS_GPIO_PIN      GPIO_PIN_5
#else
#define SPI_CS_GPIO_BASE     GPIO_A_BASE
#define SPI_CS_GPIO_PIN      GPIO_PIN_3
#endif

void enc28j60_spi_cs(uint8_t high);
void enc28j60_spi_write(uint8_t dat);
uint8_t enc28j60_spi_read(void);
void enc28j60_spi_init(void);

#endif
