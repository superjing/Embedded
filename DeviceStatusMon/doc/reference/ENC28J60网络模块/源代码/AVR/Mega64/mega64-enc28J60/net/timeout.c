#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include "avr_compat.h"

#include <util/delay.h>


void delay_ms(unsigned int ms)	// 延时一个ms
{
        // 调用编译器自带的宏函数，相比自己编写的延时函数来说更加精确，而且便于移植
        while(ms){
                _delay_ms(0.96);
                ms--;
        }
}
