/*

                         \\\|///
                       \\  - -  //
                        (  @ @  )
+---------------------oOOo-(_)-oOOo-------------------------+
|                 智林STM32开发板试验程序                   |
|                 Timer2 PWM 输出方式试验                   |
|                 刘笑然 by Xiaoran Liu                     |
|                        2008.4.16                          |
|                                                           |
|           智林测控技术研究所 ZERO research group          |
|                      www.the0.net                         |
|                             Oooo                          |
+-----------------------oooO--(   )-------------------------+
                       (   )   ) /
                        \ (   (_/
                         \_)     

*/
/*----------------------------------------------------------*\
 |  引入相关芯片的头文件                                    |
\*----------------------------------------------------------*/
#include <stdio.h>
#include <stm32f10x_lib.h>    // STM32F10x Library Definitions
#include "STM32_Init.h"       // STM32 Initialization
#include "spi.h"
#include "enc28j60.h"
#include "simple_server.h"
//#include "TFT018.h"
/*----------------------------------------------------------*\
 | HARDWARE DEFINE                                          |
\*----------------------------------------------------------*/
#define LED             ( 1 << 5 )              // PB5: LED D2

#define BP2             0x2000                     // PC13: BP2
#define BP3             0x0001                     // PA0 : BP3

#define UP              0x0800                     // PB11: UP
#define RIGHT           0x1000                     // PB12: RIGHT
#define LEFT            0x2000                     // PB13: LEFT
#define DOWN            0x4000                     // PB14: DOWN
#define OK              0x8000                     // PB15: OK

#define JOYSTICK        0xF800                     // JOYSTICK ALL KEYS
/*----------------------------------------------------------*\
 | SOFTWARE DATA                                            |
\*----------------------------------------------------------*/
/*----------------------------------------------------------*\
 |  Delay                                                   |
 |  延时 Inserts a delay time.                              |
 |  nCount: 延时时间                                        |
 |  nCount: specifies the delay time length.                |
\*----------------------------------------------------------*/
void Delay(vu32 nCount) {
  for(; nCount != 0; nCount--);
  }
/*----------------------------------------------------------*\
 | SendChar                                                 |
 | Write character to Serial Port.                          |
\*----------------------------------------------------------*/
int SendChar (int ch)  {

  while (!(USART1->SR & USART_FLAG_TXE));
  USART1->DR = (ch & 0x1FF);

  return (ch);
}
/*----------------------------------------------------------*\
 | GetKey                                                   |
 | Read character to Serial Port.                           |
\*----------------------------------------------------------*/
int GetKey (void)  {

  while (!(USART1->SR & USART_FLAG_RXNE));

  return ((int)(USART1->DR & 0x1FF));
}

const unsigned char enc28j60_MAC[6] = {0x11, 0x02, 0x03, 0x04, 0x05, 0x66};
/*----------------------------------------------------------*\
 | MIAN ENTRY                                               |
\*----------------------------------------------------------*/
int main (void)
{
    int rev = 0;

  stm32_Init ();                                // STM32 setup
//  GPIOD->ODR &= ~(1<<9);//GPIOA->BRR = ENC28J60_CS;
//  GPIOD->ODR |= 1<<9;//GPIOA->BSRR = ENC28J60_CS;
  printf ("SPI1_Init starting...\r\n");
  SPI1_Init();
  printf ("enc28j60 init...\r\n");
  //enc28j60Init((unsigned char *)enc28j60_MAC);  


    simple_server();

    enc28j60Init((unsigned char *)enc28j60_MAC);

    rev = enc28j60getrev();

    return rev;

/*
  for(;;) {
    unsigned char c;

    printf ("Press a key. ");
    c = getchar ();
    printf ("\r\n");
    printf ("You pressed '%c'.\r\n\r\n", c);
    }
*/
  }
/*----------------------------------------------------------*\
 | END OF FILE                                              |
\*----------------------------------------------------------*/
