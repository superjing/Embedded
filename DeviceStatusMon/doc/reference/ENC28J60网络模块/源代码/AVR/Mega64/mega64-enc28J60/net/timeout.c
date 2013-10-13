#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include "avr_compat.h"

#include <util/delay.h>


void delay_ms(unsigned int ms)	// ��ʱһ��ms
{
        // ���ñ������Դ��ĺ꺯��������Լ���д����ʱ������˵���Ӿ�ȷ�����ұ�����ֲ
        while(ms){
                _delay_ms(0.96);
                ms--;
        }
}
