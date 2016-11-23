/* #include "h4_uart_interface.h" */
#include "hci.h"
#include "ble/hci_transport.h"
#include "thread.h"
#include "ble/ble_h4_transport.h"
#include "hcitest.h"

#define UART_BUF        UT2_BUF
#define UART_CON        UT2_CON
#define UART_BAUD       UT2_BAUD 
#define UART_INDEX      UART2_INIT

/********************************************************************************/
/*
 *                   H4 Uart Tx (ACL RX & Event From Controller)
 */
static void __uart_putbyte(u8 data)
{
    UART_BUF = data;
    __asm__ volatile ("csync"); //PI32特有

    while((UART_CON & BIT(15)) == 0);
}

static void __uart_tx(u8 *buf, u16 len)
{
    while(len)
    {

#ifdef H4_FLOW_CONTROL
        if(HCI_UART_RTS_READ())
        {
            puts("hci:device busy\n");
            while(HCI_UART_RTS_READ())
            {
                hcitest_time_dly_n10ms(1);
            }
            puts("hci:device idle\n");
        }
#endif
        __uart_putbyte(*buf);
        len--;
        buf++;
    }
}

static int h4_uart_notify_host(u8 packet_type, u8 *packet, int len)
{
    __uart_tx(&packet_type, 1); 
    __uart_tx(packet, len);
}
REGISTER_H4_HOST(h4_uart_notify_host);

/********************************************************************************/
/*
 *                   H4 Uart Rx (ACL TX & Command From Host)
 */

#define UART_RX_MAX_SIZE        0x20


struct uart_rx{
    char buf[UART_RX_MAX_SIZE];
    u8 wr_index;
    u8 rd_index;
};

#define UART_RX_BUF_IS_EXCEED(x)    (x >= UART_RX_MAX_SIZE)

static struct uart_rx   uart_rx_t;

static void __data_reset(void)
{
    uart_rx_t.wr_index = uart_rx_t.rd_index = 0;
    memset(uart_rx_t.buf, 0x0, UART_RX_MAX_SIZE);
}

static void __data_push(char data)
{
    if (UART_RX_BUF_IS_EXCEED(uart_rx_t.wr_index))
    {
        puts("UART_RX_BUF_IS_EXCEED\n");
        /*-TODO-*/
        //option : do idle
        return;
    }
    uart_rx_t.buf[uart_rx_t.wr_index++] = data; 
}

static int __data_pop(char *buf, int len)
{
    if (len > uart_rx_t.wr_index)
    {
        puts("UART_RX_BUF not enough\n");
        return 0;
    }

    memcpy(buf, uart_rx_t.buf[uart_rx_t.rd_index], len);
    uart_rx_t.rd_index += len;

    return len;
}

void h4_uart_data_rxloop(void)
{
    u8 packet_type;
    u8 rd_offset;

    rd_offset = uart_rx_t.rd_index;

    packet_type = uart_rx_t.buf[rd_offset];
    if ((packet_type != HCI_COMMAND_DATA_PACKET)
        || (packet_type != HCI_ACL_DATA_PACKET))
    {
        return;
    }

    rd_offset += 1;

    u8 packet_length[2];
    u8 packet[0x100];
    
    switch (packet_type)
    {
    case HCI_COMMAND_DATA_PACKET:
        memcpy(packet_length, uart_rx_t.buf[rd_offset], 1);
        rd_offset += 1;
        if (packet_length < uart_rx_t.wr_index)
        {
            __data_pop(packet, (packet_length + rd_offset));
            ble_h4_send_packet(packet_type, packet, packet_length);
            CPU_INT_DIS();
            __data_reset();
            CPU_INT_EN();
        }
        break;
    case HCI_ACL_DATA_PACKET:
        memcpy(packet_length, uart_rx_t.buf[rd_offset], 2);
        rd_offset += 2;
        if (packet_length < uart_rx_t.wr_index)
        {
            __data_pop(packet, (packet_length + rd_offset));
            ble_h4_send_packet(packet_type, packet, packet_length);
            CPU_INT_DIS();
            __data_reset();
            CPU_INT_EN();
        }
        break;

    default:
        puts("Unknow packet type\n");
        break;
    }
}


static void uart_irq_handle(void)
{
    u8 value;

    if((UART_CON & BIT(14)) != 0) //check rx
    {
        UART_CON |= BIT(12); //clr rx pd
        value = UART_BUF;
        
        putchar(value);
        __data_push(value);    
    }

    ILAT0_CLR = BIT(UART_INDEX);
}
REG_INIT_HANDLE(uart_irq_handle);


void h4_uart_init(void)
{
    puts("hci_uart: TX--PA10，RX--PA11\n");
    IOMC1 &= ~(BIT(15)|BIT(14));
    IOMC1 |= BIT(14);
    PORTA_OUT |= BIT(10) ;
    PORTA_DIR |= BIT(11) ;
    PORTA_DIR &=~BIT(10) ;

    //初始化，配置RX中断，波特率115200
    UART_BAUD = (48000000/115200)/4 -1;//115200
    UART_CON = BIT(13) | BIT(12) ; //rx_ie , CLR RX PND

    __data_reset();
    INTALL_HWI(UART_INDEX, uart_irq_handle, 3) ; //注 册中断
    UART_CON |= (BIT(3) | BIT(0)) ;//enable rx isr
}
