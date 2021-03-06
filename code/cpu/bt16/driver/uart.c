#include "uart.h"
#include "typedef.h"

void uart_init(u32 fre)
{
    //CLK_CON1 |= (1<<12);           //  uart clk sel;
    SFR(CLK_CON1, 10, 2, 2);
    SFR(IOMC0, 6, 2, 0);
	UT0_BAUD = fre / 4 - 1;
	UT0_CON |= BIT(0);       //rx_ie , CLR RX PND
	UT0_CON &= ~BIT(2);
    PORTA_DIR &= ~BIT(8);
    PORTA_DIR |= BIT(9);
}

void putbyte(char a)
{
    if(a == '\n')
    {
        UT0_BUF = '\r';
        while((UT0_CON & BIT(15)) == 0);
    }

    UT0_BUF = a;

    while((UT0_CON & BIT(15)) == 0);     //TX IDLE
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
	if((UT0_CON & BIT(14))){
		UT0_CON |= BIT(12);
		return UT0_BUF;
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


