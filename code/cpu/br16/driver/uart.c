#include "uart.h"
#include "typedef.h"

void uart_init(u32 fre)
{
    PORTA_DIR |= BIT(7);
    PORTA_DIR &= ~BIT(6);
    CLK_CON1 &= ~(3 << 10);
    CLK_CON1 |= 2 << 10;
    UT1_BAUD = fre / 4 - 1;
    /* UT1_BAUD = fre / 4 - 1; */
    UT1_CON = BIT(13) | BIT(12) | BIT(0);
}

void putbyte(char a)
{
    if(a == '\n')
    {
        UT1_BUF = '\r';
        __asm__ volatile ("csync");

        while((UT1_CON & BIT(15)) == 0);
    }

    UT1_BUF = a;
    __asm__ volatile ("csync");

    while((UT1_CON & BIT(15)) == 0);     //TX IDLE
}

int putchar(int a)
{
    if(a == '\n')
    {
        putbyte(0x0d);
        putbyte(0x0a);
    }
    else
    {
        putbyte(a);
    }

    return a;
}

int getchar()
{
	if((UT1_CON & BIT(14))){
		UT1_CON |= BIT(12);
		return UT1_BUF;
	}

	return 0;
}





void put_u4hex(u8 dat)
{
    dat = 0xf & dat;

    if(dat > 9)
    {
        putbyte(dat - 10 + 'A');
    }
    else
    {
        putbyte(dat + '0');
    }
}

void put_u64hex(u64 dat)
{
    put_u4hex(dat >> (28 + 32));
    put_u4hex(dat >> (28 + 28));
                              
    put_u4hex(dat >> (28 + 24));
    put_u4hex(dat >> (28 + 20));
                              
    put_u4hex(dat >> (28 + 16));
    put_u4hex(dat >> (28 + 12));

    put_u4hex(dat >> (28 + 8));
    put_u4hex(dat >> (28 + 4));

    put_u4hex(dat >> 28);
    put_u4hex(dat >> 24);

    put_u4hex(dat >> 20);
    put_u4hex(dat >> 16);

    put_u4hex(dat >> 12);
    put_u4hex(dat >> 8);

    put_u4hex(dat >> 4);
    put_u4hex(dat);
    putchar(' ');
}
void put_u32hex(u32 dat)
{
    put_u4hex(dat >> 28);
    put_u4hex(dat >> 24);

    put_u4hex(dat >> 20);
    put_u4hex(dat >> 16);

    put_u4hex(dat >> 12);
    put_u4hex(dat >> 8);

    put_u4hex(dat >> 4);
    put_u4hex(dat);
    putchar(' ');
}

void put_u16hex(u16 dat)
{
    put_u4hex(dat >> 12);
    put_u4hex(dat >> 8);

    put_u4hex(dat >> 4);
    put_u4hex(dat);
}

void put_u8hex(u8 dat)
{
    put_u4hex(dat >> 4);
    put_u4hex(dat);
}


void printf_buf(u8 *buf, u32 len)
{
    u32 i ;

    putchar('\n');
    put_u32hex(buf);
    putchar('\n');
	putchar('{');
    for(i = 0 ; i < len ; i++)
    {
        if(i && (i % 16) == 0)
        {
            putbyte('\n') ;
        }

        put_u8hex(buf[i]) ;
		if (i+1 < len){
			putchar(',');
		}
    }
	putchar('}');

	putbyte('\n') ;
	putbyte('\n') ;
}

void printf_buf_1(u8 *buf, u32 len)
{
    u32 i ;

    putchar('\n');
    put_u32hex(buf);
    putchar('\n');
	putchar('{');
    for(i = 0 ; i < len ; i++)
    {
        if(i && (i % 16) == 0)
        {
            putbyte('\n') ;
        }

		puts("0x");
        put_u8hex(buf[i]) ;
		if (i+1 < len){
			putchar(',');
		}
    }
	putchar('}');

	putbyte('\n') ;
	putbyte('\n') ;
}

