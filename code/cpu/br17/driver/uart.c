#include "uart.h"
#include "typedef.h"

#define __DEBUG

#define UART0   0
#define UART1   1
#define UART2   2
#ifdef __DEBUG
#define DEBUG_UART   UART0 /*select uart port*/
#else
#define DEBUG_UART   0xFF  /*uart invalid*/
#endif


#if (DEBUG_UART == UART0)
    #define UART_TXPA5_RXPA6
    /* #define  UART_TXPB6_RXPB7*/
    /* #define UART_TXPC2_RXPC3 */
    //#define UART_TXPA14_RXPA15
#endif

#if (DEBUG_UART == UART1)
    /* #define  UART_TXPB0_RXPB1*/
    //#define UART_TXPC0_RXPC1
    //#define UART_TXPA1_RXPA2
    #define UART_USBP_USBM
#endif

#if (DEBUG_UART == UART2)
    //#define UART_TXPA3_RXPA4
    /* #define UART_TXPA9_RXPA10 */
    //#define UART_TXPB9_RXPB10
    #define UART_TXPC4_RXPC5
#endif


void uart_init(u32 baud)
{
	SFR(CLK_CON1, 10, 2, 1); //use PLL48M clk

#if (DEBUG_UART == UART0)
    #ifdef UART_TXPA5_RXPA6
        IOMC0 &= ~(BIT(7)|BIT(6));
        PORTA_OUT |= BIT(5) ;
        PORTA_DIR |= BIT(6) ;
        PORTA_DIR &=~BIT(5) ;
    #endif/*UART_TXPA8_RXPA9*/

    #ifdef UART_TXPB6_RXPB7
        IOMC0 &= ~(BIT(7)|BIT(6));
        IOMC0 |= BIT(6);
        PORTB_OUT |= BIT(6) ;
        PORTB_DIR |= BIT(7) ;
        PORTB_DIR &=~BIT(6) ;
    #endif/*UART_TXPB6_RXPB7*/

    #ifdef UART_TXPC2_RXPC3
        IOMC0 &=~(BIT(7)|BIT(6));
        IOMC0 |= BIT(7);
        PORTC_OUT |= BIT(2) ;
        PORTC_DIR |= BIT(3) ;
        PORTC_DIR &=~BIT(2) ;
    #endif/*UART_TXPC2_RXPC3*/

    #ifdef UART_TXPA14_RXPA15
        IOMC0 |= BIT(7)|BIT(6);
        PORTA_OUT |= BIT(14) ;
        PORTA_DIR |= BIT(15) ;
        PORTA_DIR &=~BIT(14) ;
    #endif/*UART_TXPA14_RXPA15*/

    UT0_BAUD = baud / 4;
    UT0_CON = BIT(13) | BIT(12) | BIT(0);       //rx_ie , CLR RX PND

#elif (DEBUG_UART == UART1)
    #ifdef UART_TXPB0_RXPB1
        IOMC1 &= ~(BIT(3)|BIT(2));
        PORTB_OUT |= BIT(0) ;
        PORTB_DIR |= BIT(1) ;
        PORTB_DIR &=~BIT(0) ;
    #endif/*UART_TXPB0_RXPB1*/

    #ifdef UART_TXPC0_RXPC1
        IOMC1 &= ~(BIT(3)|BIT(2));
        IOMC1 |= BIT(2);
        PORTC_OUT |= BIT(0) ;
        PORTC_DIR |= BIT(1) ;
        PORTC_DIR &=~BIT(0) ;
    #endif/*UART_TXPC0_RXPC1*/

    #ifdef UART_TXPA1_RXPA2
        IOMC1 &= ~(BIT(3)|BIT(2));
        IOMC1 |= BIT(3);
        PORTA_OUT |= BIT(1) ;
        PORTA_DIR |= BIT(2) ;
        PORTA_DIR &=~BIT(1) ;
    #endif/*UART_TXPA1_RXPA2*/

    #ifdef UART_USBP_USBM
        IOMC1 |= BIT(3)|BIT(2);
        USB_CON0 |= BIT(0)|BIT(1);
        USB_IO_CON = 0;
        USB_IO_CON |= BIT(0); //tx dp
        USB_IO_CON |= BIT(3);//rx dm
        USB_IO_CON &= ~BIT(2);//tx dp
    #endif/*UART_USBP_USBM*/

    UT1_BAUD = baud / 4;
    UT1_CON = BIT(13) | BIT(12) | BIT(0);       //rx_ie , CLR RX PND

#elif (DEBUG_UART == UART2)
    #ifdef UART_TXPA3_RXPA4
        IOMC1 &= ~(BIT(15)|BIT(14));
        PORTA_OUT |= BIT(3) ;
        PORTA_DIR |= BIT(4) ;
        PORTA_DIR &=~BIT(3) ;
    #endif/*UART_TXPA3_RXPA4*/

    #ifdef UART_TXPA9_RXPA10
        IOMC1 &= ~(BIT(15)|BIT(14));
        IOMC1 |= BIT(14);
        PORTA_OUT |= BIT(9) ;
        PORTA_DIR |= BIT(10) ;
        PORTA_DIR &=~BIT(9) ;
    #endif/*UART_TXPA9_RXPA10*/

    #ifdef UART_TXPB9_RXPB10
        IOMC1 &= ~(BIT(15)|BIT(14));
        IOMC1 |= BIT(15);
        PORTB_OUT |= BIT(9) ;
        PORTB_DIR |= BIT(10) ;
        PORTB_DIR &=~BIT(9) ;
    #endif/*UART_TXPB9_RXPB10*/

    #ifdef UART_TXPC4_RXPC5
        IOMC1 |= BIT(15)|BIT(14);
        PORTC_OUT |= BIT(4) ;
        PORTC_DIR |= BIT(5) ;
        PORTC_DIR &=~BIT(4) ;
    #endif/*UART_TXPC4_RXPC5*/

    UT2_BAUD = baud / 4;
    UT2_CON = BIT(13) | BIT(12) | BIT(0);       //rx_ie , CLR RX PND
#endif

    //set_uart_map(DEBUG_UART);
}

/*串口映射*/
void set_uart_map(u8 uart_index)
{
#ifdef __DEBUG
	//选择当前使用的UART进行映射
	switch(uart_index)
	{
		case UART0:
			IOMC1 &= ~(BIT(8)|BIT(9)|BIT(10));//选择UT0_TX
			break;
		case UART1:
		     IOMC1 &= ~(BIT(8)|BIT(9)|BIT(10));//选择UT1_TX
             IOMC1 |= BIT(8);
			break;
		case UART2:
			IOMC1 |= BIT(8)|BIT(9)|BIT(10);    //选择UT2_TX
			IOMC1 |= BIT(8)|BIT(9)|BIT(10);
			break;
		default:
			return;
	}
	//设置要作为串口的GPIO
    PORTA_DIR &= ~BIT(2);
    PORTA_DIE &= ~BIT(2);
    PORTA_PD  |= BIT(2);
    PORTA_PU  |= BIT(2);
#endif
}



#if (DEBUG_UART == UART0)
    void putbyte(char a)
    {
        if(a == '\n')
        {
            UT0_BUF = '\r';
            __asm__ volatile ("csync");
            while((UT0_CON & BIT(15)) == 0);
        }

        UT0_BUF = a;
        __asm__ volatile ("csync");

        while((UT0_CON & BIT(15)) == 0);     //TX IDLE
    }

	int getchar()
    {
        char c=0;

        if((UT0_CON & BIT(14))){
            c = UT0_BUF;
            UT0_CON |= BIT(12);
        }

        return c;
    }
    char uart_read()
    {
        char c=0;

        if((UT0_CON & BIT(14))){
            c = UT0_BUF;
            UT0_CON |= BIT(12);
        }

        return c;
    }

#elif (DEBUG_UART == UART1)
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

	int getchar()
    {
        char c=0;

        if((UT1_CON & BIT(14))){
            c = UT1_BUF;
            UT1_CON |= BIT(12);
        }

        return c;
    }
    char uart_read()
    {
        char c=0;

        if((UT1_CON & BIT(14))){
            c = UT1_BUF;
            UT1_CON |= BIT(12);
        }

        return c;
    }

#elif (DEBUG_UART == UART2)
    void putbyte(char a)
    {
        if(a == '\n')
        {
            UT2_BUF = '\r';
            __asm__ volatile ("csync");

            while((UT2_CON & BIT(15)) == 0);
        }

        UT2_BUF = a;
        __asm__ volatile ("csync");

        while((UT2_CON & BIT(15)) == 0);     //TX IDLE
    }

	int getchar()
    {
        char c=0;

        if((UT2_CON & BIT(14))){
            c = UT2_BUF;
            UT2_CON |= BIT(12);
        }

        return c;
    }
    char uart_read()
    {
        char c=0;

        if((UT2_CON & BIT(14))){
            c = UT2_BUF;
            UT2_CON |= BIT(12);
        }

        return c;
    }
#else
    void putbyte(char a)
    {
    }
	int getchar()
    {
        return 0;
    }
    char uart_read()
    {
        return 0;
    }
#endif

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
    putbyte('0');
    putbyte('x');
    put_u4hex(dat >> 28);
    put_u4hex(dat >> 24);

    put_u4hex(dat >> 20);
    put_u4hex(dat >> 16);

    put_u4hex(dat >> 12);
    put_u4hex(dat >> 8);

    put_u4hex(dat >> 4);
    put_u4hex(dat);
    putchar('\n');
}

void put_u32hex0(u32 dat)
{

    put_u4hex(dat >> 28);
    put_u4hex(dat >> 24);

    put_u4hex(dat >> 20);
    put_u4hex(dat >> 16);

    put_u4hex(dat >> 12);
    put_u4hex(dat >> 8);

    put_u4hex(dat >> 4);
    put_u4hex(dat);
}

void put_u64hex(u64 dat)
{
    putbyte('0');
    putbyte('x');
    put_u32hex0(dat>>32);
    put_u32hex0(dat);
    putchar('\n');
}
void put_u16hex(u16 dat)
{
    putbyte('0');
    putbyte('x');


    put_u4hex(dat >> 12);
    put_u4hex(dat >> 8);

    put_u4hex(dat >> 4);
    put_u4hex(dat);
    putchar(' ');
}

void put_u8hex(u8 dat)
{
    put_u4hex(dat >> 4);
    put_u4hex(dat);
    putchar(' ');
}

void printf_buf(u8 *buf, u32 len)
{

    u32 i ;

    for(i = 0 ; i < len ; i++)
    {
        if((i % 16) == 0)
        {
            putbyte('\n') ;
        }

        put_u8hex(buf[i]) ;
    }

    putbyte('\n') ;

}

