/*

                         \\\|///
                       \\  - -  //
                        (  @ @  )
+---------------------oOOo-(_)-oOOo-------------------------+
|                                                           |
|                       PCF8833.c                           |
|                     by Xiaoran Liu                        |
|                        2005.3.16                          |
|                                                           |
|                    ZERO research group                    |
|                        www.the0.net                       |
|                                                           |
|                            Oooo                           |
+----------------------oooO--(   )--------------------------+
                      (   )   ) /
                       \ (   (_/
                        \_)     

*/
#ifndef __PCD5544_H__
#define __PCD5544_H__

/*
RST - P0.5
DC - P0.8
SDATA - P0.6
SCLK - P0.4
CS - P0.7
*/

#define	LCD_RST		0x00000020
#define	LCD_CS		0x00000080
#define	LCD_DC		0x00000100
//#define	LCD_DATA	0x00000040
#define	LCD_CLK		0x00000010

void LCD_Init(void);
void LCD_Cls(void);
void LCD_GotoXY(unsigned char x, unsigned char y);
void LCD_PrintStr(unsigned char *s);

#endif

