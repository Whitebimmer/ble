/* #include "h4_uart_interface.h" */
#include "hci.h"
#include "ble/hci_transport.h"
#include "thread.h"
#include "ble/ble_h4_transport.h"
#include "hcitest.h"

void h4_uart_clear(void);
void h4_uart_dump(void);

/********************************************************************************/
/*
 *                   H4 Uart Config
 */
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

#define UART_RX_MAX_SIZE        0x80


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
        printf("UART_RX_BUF_IS_EXCEED : %x\n", uart_rx_t.wr_index);
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

    memcpy(buf, uart_rx_t.buf + uart_rx_t.rd_index, len);
    uart_rx_t.rd_index += len;

    return len;
}

static int reset_timeout = 0;

void h4_uart_data_rxloop(void)
{
    u8 packet_type;
    u8 rd_offset;

    if (reset_timeout++ > 500000)
    {
        reset_timeout = 0;
        h4_uart_clear();
        /* puts("clear\n"); */
    }

    rd_offset = uart_rx_t.rd_index;

    packet_type = uart_rx_t.buf[rd_offset];
    if ((packet_type != HCI_COMMAND_DATA_PACKET)
        && (packet_type != HCI_ACL_DATA_PACKET))
    {
        /* puts("eror \n"); */
        return;
    }

    //packet type + ocf | ogf 
    if (uart_rx_t.wr_index <= 3)
    {
        return;
    }

    rd_offset += 3;

    u8 packet_length[2];
    u8 packet[0x100];
    
    switch (packet_type)
    {
    case HCI_COMMAND_DATA_PACKET:
        puts("HCI_COMMAND_DATA_PACKET\n");
        memcpy(packet_length, uart_rx_t.buf + rd_offset, 1);
        /* printf("%x / %x\n", packet_length[0], uart_rx_t.wr_index); */
        if (packet_length[0] < uart_rx_t.wr_index)
        {
            int len = packet_length[0] + rd_offset;

            //skip packet_type
            __data_pop(packet, 1);
            __data_pop(packet, len);
            printf_buf(packet, len);
            ble_h4_send_packet(packet_type, packet, len);
            CPU_INT_DIS();
            __data_reset();
            CPU_INT_EN();
        }
        break;
    case HCI_ACL_DATA_PACKET:
        puts("HCI_ACL_DATA_PACKET\n");
        memcpy(packet_length, uart_rx_t.buf + rd_offset, 2);
        printf("%x / %x\n", READ_BT_16(packet_length, 0), uart_rx_t.wr_index);
        if (READ_BT_16(packet_length, 0) < uart_rx_t.wr_index)
        {
            int len = READ_BT_16(packet_length, 0) + rd_offset;

            //skip packet_type
            __data_pop(packet, 1);
            __data_pop(packet, len);
            ble_h4_send_packet(packet_type, packet, len);
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

    if(UART_CON & BIT(14)) //check rx
    {
        UART_CON |= BIT(12); //clr rx pd
        value = UART_BUF;
        
        /* putchar(value); */
        reset_timeout = 0;
        __data_push(value);    
    }

    ILAT0_CLR = BIT(UART_INDEX);
}
REG_INIT_HANDLE(uart_irq_handle);


void h4_uart_init(void)
{
#if 0
    puts("hci_uart: TX--PA10，RX--PA11\n");
    IOMC1 &= ~(BIT(15)|BIT(14));
    IOMC1 |= BIT(14);
    PORTA_OUT |= BIT(9) ;
    PORTA_DIR |= BIT(10) ;
    PORTA_DIR &= ~BIT(9) ;
#else 
    puts("hci_uart: TX--PC4，RX--PC5\n");
    IOMC1 |= (BIT(15)|BIT(14));
    PORTC_OUT |= BIT(4) ;
    PORTC_DIR |= BIT(5) ;
    PORTC_DIR &= ~BIT(4) ;
#endif

    //初始化，配置RX中断，波特率115200
    UART_BAUD = (48000000/115200)/4 -1;//115200
    UART_CON = BIT(13) | BIT(12) ; //rx_ie , CLR RX PND

    __data_reset();
    INTALL_HWI(UART_INDEX, uart_irq_handle, 3) ; //注 册中断
    UART_CON |= (BIT(3) | BIT(0)) ;//enable rx isr
}

void h4_uart_dump(void)
{
    printf("h4 uart dump : rd %x / wr %x\n", uart_rx_t.rd_index, uart_rx_t.wr_index);
    printf_buf(uart_rx_t.buf, UART_RX_MAX_SIZE);
}

void h4_uart_clear(void)
{
    CPU_INT_DIS();
    __data_reset();
    CPU_INT_EN();
}
