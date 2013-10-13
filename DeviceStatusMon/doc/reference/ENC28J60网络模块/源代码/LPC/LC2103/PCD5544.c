/*

                         \\\|///
                       \\  - -  //
                        (  @ @  )
+---------------------oOOo-(_)-oOOo-------------------------+
|                                                           |
|                        PCD5544.c                          |
|                     by Xiaoran Liu                        |
|                        2008.3.16                          |
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
// 引入相关芯片的头文件 
#include <LPC2103.h>
#include "PCD5544.h"

const unsigned char ASC2[]={          
0x00, 0x00, 0x00, 0x00, 0x00,   // sp     
0x00, 0x00, 0x2f, 0x00, 0x00,   // !     
0x00, 0x07, 0x00, 0x07, 0x00,   // "     
0x14, 0x7f, 0x14, 0x7f, 0x14,   // #     
0x24, 0x2a, 0x7f, 0x2a, 0x12,   // $     
0x62, 0x64, 0x08, 0x13, 0x23,   // %     
0x36, 0x49, 0x55, 0x22, 0x50,   // &     
0x00, 0x05, 0x03, 0x00, 0x00,   // ’     
0x00, 0x1c, 0x22, 0x41, 0x00,   // (     
0x00, 0x41, 0x22, 0x1c, 0x00,   // )     
0x14, 0x08, 0x3E, 0x08, 0x14,   // *     
0x08, 0x08, 0x3E, 0x08, 0x08,   // +     
0x00, 0x00, 0xA0, 0x60, 0x00,   // ,     
0x08, 0x08, 0x08, 0x08, 0x08,   // -     
0x00, 0x60, 0x60, 0x00, 0x00,   // .     
0x20, 0x10, 0x08, 0x04, 0x02,   // /     
0x3E, 0x51, 0x49, 0x45, 0x3E,   // 0     
0x00, 0x42, 0x7F, 0x40, 0x00,   // 1     
0x42, 0x61, 0x51, 0x49, 0x46,   // 2     
0x21, 0x41, 0x45, 0x4B, 0x31,   // 3     
0x18, 0x14, 0x12, 0x7F, 0x10,   // 4     
0x27, 0x45, 0x45, 0x45, 0x39,   // 5     
0x3C, 0x4A, 0x49, 0x49, 0x30,   // 6     
0x01, 0x71, 0x09, 0x05, 0x03,   // 7     
0x36, 0x49, 0x49, 0x49, 0x36,   // 8     
0x06, 0x49, 0x49, 0x29, 0x1E,   // 9     
0x00, 0x36, 0x36, 0x00, 0x00,   // :     
0x00, 0x56, 0x36, 0x00, 0x00,   // ;     
0x08, 0x14, 0x22, 0x41, 0x00,   // <     
0x14, 0x14, 0x14, 0x14, 0x14,   // =     
0x00, 0x41, 0x22, 0x14, 0x08,   // >     
0x02, 0x01, 0x51, 0x09, 0x06,   // ?     
0x32, 0x49, 0x59, 0x51, 0x3E,   // @     
0x7C, 0x12, 0x11, 0x12, 0x7C,   // A     
0x7F, 0x49, 0x49, 0x49, 0x36,   // B     
0x3E, 0x41, 0x41, 0x41, 0x22,   // C     
0x7F, 0x41, 0x41, 0x22, 0x1C,   // D     
0x7F, 0x49, 0x49, 0x49, 0x41,   // E     
0x7F, 0x09, 0x09, 0x09, 0x01,   // F     
0x3E, 0x41, 0x49, 0x49, 0x7A,   // G     
0x7F, 0x08, 0x08, 0x08, 0x7F,   // H     
0x00, 0x41, 0x7F, 0x41, 0x00,   // I     
0x20, 0x40, 0x41, 0x3F, 0x01,   // J     
0x7F, 0x08, 0x14, 0x22, 0x41,   // K     
0x7F, 0x40, 0x40, 0x40, 0x40,   // L     
0x7F, 0x02, 0x0C, 0x02, 0x7F,   // M     
0x7F, 0x04, 0x08, 0x10, 0x7F,   // N     
0x3E, 0x41, 0x41, 0x41, 0x3E,   // O     
0x7F, 0x09, 0x09, 0x09, 0x06,   // P     
0x3E, 0x41, 0x51, 0x21, 0x5E,   // Q     
0x7F, 0x09, 0x19, 0x29, 0x46,   // R     
0x46, 0x49, 0x49, 0x49, 0x31,   // S     
0x01, 0x01, 0x7F, 0x01, 0x01,   // T     
0x3F, 0x40, 0x40, 0x40, 0x3F,   // U     
0x1F, 0x20, 0x40, 0x20, 0x1F,   // V     
0x3F, 0x40, 0x38, 0x40, 0x3F,   // W     
0x63, 0x14, 0x08, 0x14, 0x63,   // X     
0x07, 0x08, 0x70, 0x08, 0x07,   // Y     
0x61, 0x51, 0x49, 0x45, 0x43,   // Z     
0x00, 0x7F, 0x41, 0x41, 0x00,   // [     
0x55, 0x2A, 0x55, 0x2A, 0x55,   // 55     
0x00, 0x41, 0x41, 0x7F, 0x00,   // ]     
0x04, 0x02, 0x01, 0x02, 0x04,   // ^     
0x40, 0x40, 0x40, 0x40, 0x40,   // _     
0x00, 0x01, 0x02, 0x04, 0x00,   // ’     
0x20, 0x54, 0x54, 0x54, 0x78,   // a     
0x7F, 0x48, 0x44, 0x44, 0x38,   // b     
0x38, 0x44, 0x44, 0x44, 0x20,   // c     
0x38, 0x44, 0x44, 0x48, 0x7F,   // d     
0x38, 0x54, 0x54, 0x54, 0x18,   // e     
0x08, 0x7E, 0x09, 0x01, 0x02,   // f     
0x18, 0xA4, 0xA4, 0xA4, 0x7C,   // g     
0x7F, 0x08, 0x04, 0x04, 0x78,   // h     
0x00, 0x44, 0x7D, 0x40, 0x00,   // i     
0x40, 0x80, 0x84, 0x7D, 0x00,   // j     
0x7F, 0x10, 0x28, 0x44, 0x00,   // k     
0x00, 0x41, 0x7F, 0x40, 0x00,   // l     
0x7C, 0x04, 0x18, 0x04, 0x78,   // m     
0x7C, 0x08, 0x04, 0x04, 0x78,   // n     
0x38, 0x44, 0x44, 0x44, 0x38,   // o     
0xFC, 0x24, 0x24, 0x24, 0x18,   // p     
0x18, 0x24, 0x24, 0x18, 0xFC,   // q     
0x7C, 0x08, 0x04, 0x04, 0x08,   // r     
0x48, 0x54, 0x54, 0x54, 0x20,   // s     
0x04, 0x3F, 0x44, 0x40, 0x20,   // t     
0x3C, 0x40, 0x40, 0x20, 0x7C,   // u     
0x1C, 0x20, 0x40, 0x20, 0x1C,   // v     
0x3C, 0x40, 0x30, 0x40, 0x3C,   // w     
0x44, 0x28, 0x10, 0x28, 0x44,   // x     
0x1C, 0xA0, 0xA0, 0xA0, 0x7C,   // y     
0x44, 0x64, 0x54, 0x4C, 0x44,   // z     
0x00, 0x08, 0x36, 0x41, 0x00,   // {     
0x00, 0x00, 0x7F, 0x00, 0x00,   // |     
0x00, 0x41, 0x36, 0x08, 0x00,   // }     
0x08, 0x10, 0x08, 0x04, 0x08    // ~     
};     

/*----------------------------------------------------------*\
 | Delay                                                    |
\*----------------------------------------------------------*/
void  Delay(unsigned int  dly) {
	unsigned int  i;

	for(; dly>0; dly--) 
		for(i=0; i<5000; i++);
	}
/*----------------------------------------------------------*\
 | PCD5544 SSP Initialize                                       |
\*----------------------------------------------------------*/
void  SSP_Init(void) {
	PCONP |= 1<<10;
    SSPCR0 = (0x01 << 8) |              // SCR  设置SPI时钟分频
             (0x01 << 7) |              // CPHA 时钟输出相位,仅SPI模式有效 
             (0x01 << 6) |              // CPOL 时钟输出极性,仅SPI模式有效
             (0x00 << 4) |              // FRF  帧格式 00=SPI,01=SSI,10=Microwire,11=保留
             (0x07 << 0);               // DSS  数据长度,0000-0010=保留,0011=4位,0111=8位,1111=16位

    SSPCR1 = (0x00 << 3) |              // SOD  从机输出禁能,1=禁止,0=允许
             (0x00 << 2) |              // MS   主从选择,0=主机,1=从机
             (0x01 << 1) |              // SSE  SSP使能,1=允许SSP与其它设备通信
             (0x00 << 0);               // LBM  回写模式
             
    SSPCPSR = 0x58;                     // PCLK分频值
    SSPIMSC = 0x07;                     // 中断屏蔽寄存器
    SSPICR  = 0x03;                     // 中断清除寄存器
	}
/*----------------------------------------------------------*\
 | PCD5544 Write Byte                                       |
\*----------------------------------------------------------*/
void  LCD_Write(unsigned char byte) {
	//IOCLR=LCD_CS;

	//SSPSR=0;
	SSPDR = byte;
	while( !(SSPSR&0x01) );		// 等待SPIF置位，即等待数据发送完毕

	//IOSET=LCD_CS;
	Delay(1);
	}

/*----------------------------------------------------------*\
 | PCD5544 Initialize                                       |
\*----------------------------------------------------------*/
void  LCD_Init(void) {
	//PINSEL0 &= 0xEFFFFFFF;	// 设置SPI管脚连接
	PINSEL0 |= 0x20000000;	// 设置SPI管脚连接
	//PINSEL1 &= 0xFFFFFCFF;	// 设置SPI管脚连接
	PINSEL1 |= 0x00000100;	// 设置SPI管脚连接

	IODIR |= (1<<21)|(1<<13)|(1<<19);
	IOCLR = (1<<19);

	//SSPCCR = 0x08;		// 设置SPI时钟分频
	//SSPCR = 0x38;		// 设置SPI接口模式，MSTR=1，CPOL=1，CPHA=1，LSBF=0
	SSP_Init();

	IOSET=(1<<19);

	IOCLR=(1<<21);
	IOCLR=(1<<13);			//准备写指令     
	/*
	LCD_Write(32+1);	//进入扩展指令   
	LCD_Write(128+38);	//设置Vop,相当于亮度       
	LCD_Write(4+3);		//设置温度系数,相当于对比度       
	LCD_Write(16 +3);	// 设置偏置,这句要与不要的实际效果好像一样   
	LCD_Write(32+0);	//进入基本指令    
	LCD_Write(12);		//使能芯片活动/垂直寻址    
	*/
	LCD_Write(0x21);//初始化Lcd,功能设定使用扩充指令
	LCD_Write(0xd0);//设定液晶偏置电压
	LCD_Write(0x20);//使用基本指令
	LCD_Write(0x0C);//设定显示模式，正常显示
	IOSET=(1<<21);
	Delay(1);
	}
/*----------------------------------------------------------*\
 | PCD5544 goto x,y                                         |
\*----------------------------------------------------------*/
void  LCD_GotoXY(unsigned char x, unsigned char y) {
	IOCLR = (1<<13);			//准备写指令     
	IOCLR = (1<<21);
	LCD_Write(128+x);
	LCD_Write(64+y);
	IOSET=(1<<21);
	Delay(1);
	}	
/*----------------------------------------------------------*\
 | PCD5544 clear screen                                     |
\*----------------------------------------------------------*/
void  LCD_Cls(void) {
	int i;
	LCD_GotoXY(0,0);
	IOSET = (1<<13);			//准备写数据
	IOCLR = (1<<21);
	for(i=0;i<504;i++)
		//LCD_Write(0x55);
		LCD_Write(0);
	IOSET=(1<<21);
	Delay(1);
	}	
/*----------------------------------------------------------*\
 | PCD5544 put a character                                  |
\*----------------------------------------------------------*/
void LCD_Putchar(unsigned char character) {		//显示ASCII值的字符     
	unsigned char i=0;     
	unsigned int No;     
	IOCLR=(1<<21);
	No=character-32;		//字模数据是由空格开始,空格字符的ASCII的值就是32     
	No=No*5;				//每个字符的字模是5个字节     
	IOSET=(1<<13);			//准备写数据
	while(i<5) {			//一个字符的字模是5个字节,就是5*8点阵      
		LCD_Write(ASC2[No]);       
		i++;     
		No++;     
		}     
	LCD_Write(0);			//每个字符之间空一列     
	IOSET=(1<<21);
	Delay(1);
	}       
/*----------------------------------------------------------*\
 | PCD5544 print string                                     |
\*----------------------------------------------------------*/
void LCD_PrintStr(unsigned char *s) {		//显示字符串
	while( *s ) {
	     LCD_Putchar( *s++ );
		 }
	}



