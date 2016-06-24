/*
 *********************************************************************************************************
 *                                                AC46
 *                                            hcitest
 *                                             CODE
 *
 *                          (c) Copyright 2015-2016, ZHUHAI JIELI
 *                                           All Rights Reserved
 *
 * File : *
 * By   : jamin.li
 * DATE : 8/20/2015 build this file
 *********************************************************************************************************
 */
#include "hcitest.h"
#include "ble_interface.h"


//<DEFINE
#define HHY_DEBUG
#ifdef 	HHY_DEBUG
	#define hhy_puts     		puts
	#define hhy_printf      	printf
	#define hhy_printf_buf      printf_buf
#else
	#define hhy_puts(...)
	#define hhy_printf(...)
	#define hhy_printf_buf(...)
#endif

#define HCITEST_NOT_MALLOC
#define TI_CC2564_EN		0

//<GOLBAL VAR
#ifdef HCITEST_NOT_MALLOC
u8 hcitest_all_buf[(HCI_UART_REC_BUFSIZE + HCI_UART_SEND_BUFSIZE + HCI_CMD_BUFSIZE)];
#endif
u8 *hci_cmd_bufaddr;
u8 *hci_sbuf;
u8 *hci_rbuf;
u16 hci_rbuf_rpt;
u16 hci_rbuf_wpt;
u16 hci_rbuf_cnt;
u16 rec_timeout_cnt;
hci_parse_ctl hci_r_ctl_blk;
volatile u32 g_10ms_unit_cnt; //n * 10ms
int r_cmd_len;

//<EXTERN VAR
extern void hci_uart_timer(void);
extern void check_uart_recieve_packet(void);
extern void hci_uart_send(u8 *buf,u16 len);
extern void hci_uart_rec_reset(void);

//<EXTERN FUNC
extern void hci_uart_isr(void);
extern void api_check_uart_recieve_packet(void);

//<CONST
const u8 hci_reset_cmd_string[] = {"01 03 0C 00"};
const u8 hci_le_enable_cmd_string[] = {"01 5B FD 02 01 01"};
const u8 hci_set_event_mask_string[] = {"01 01 0C 08 FF 9F FB FF 07 F8 BF 3D"};
const u8 hci_le_set_event_mask_string[] = {"01 01 20 08 0F 00 00 00 00 00 00 00"};


//<FUNC

int func_char_to_hex(u8 *hex_buf, u8 *char_buf)
{
    int cnt = 0;
    int total = 0;
    u8 hex_val = 0;
    u8 data_flag = 0;
    u8 val;

    while(char_buf[cnt] != '\0')
    {
        val = char_buf[cnt];
        cnt++;
        if((val >= '0') && (val <= '9'))
        {
            hex_val = (hex_val << 4) + (val - '0');
            data_flag++;
        }
        else if((val >= 'a') && (val <= 'f'))
        {
            hex_val = (hex_val << 4) + (val - 'a' + 0x0a);
            data_flag++;
        }
        else if((val >= 'A') && (val <= 'F'))
        {
            hex_val = (hex_val << 4) + (val - 'A' + 0x0a);
            data_flag++;
        }
        else ///if(val == ' ')
        {
            if(data_flag > 0)
            {
                if(data_flag > 2)
                {
                    hhy_printf("**char is err,%s\n",char_buf);
                }
                hex_buf[total] = hex_val;
                total++;
                hex_val = 0;
                data_flag = 0;
            }

        }
    }

    if(data_flag > 0)
    {
        hex_buf[total] = hex_val;
        total++;
        hex_val = 0;
    }
    return total;
}

//--------------------------TIME SET----------------------------//
//--------------------------------------------------------------//
void API_h4_receive_check(void)
{
    static u32 count_10ms = 0;

    if(g_10ms_unit_cnt)
    {
        g_10ms_unit_cnt--;
    }

    hci_uart_timer();

    count_10ms++;
    if(count_10ms == 5) //50ms
    {
        count_10ms = 0 ;
        api_check_uart_recieve_packet();
    }
}

static time_dely_us(u32 count)
{
    while(count > 0)
    {
        count--;
    }
}

static void hcitest_time_dly_n10ms(u32 count)
{
    g_10ms_unit_cnt = count;
    while(g_10ms_unit_cnt);
}

///10ms 定时器
void hci_uart_timer(void)
{
    if(rec_timeout_cnt > 0)
    {
        rec_timeout_cnt--;
        if(rec_timeout_cnt == 0)
        {
            hci_uart_rec_reset();
        }
    }
}


//--------------------------H4 SEND-----------------------------//
//--------------------------------------------------------------//

void hci_uart_putbyte(u8 data)
{
    UT2_BUF = data;
    __asm__ volatile ("csync"); //PI32特有

    while((UT2_CON & BIT(15)) == 0);
}

void hci_uart_send(u8 *buf, u16 len)
{
    hhy_printf("\nHCI_TX(%d):", len);
    hhy_printf_buf(buf,len);

    while(len)
    {

#ifdef H4_FLOW_CONTROL
        if(HCI_UART_RTS_READ())
        {
            hhy_puts("hci:device busy\n");
            while(HCI_UART_RTS_READ())
            {
                hcitest_time_dly_n10ms(1);
            }
            hhy_puts("hci:device idle\n");
        }
#endif
        hci_uart_putbyte(*buf);
        len--;
        buf++;
    }
}

void api_hci_h4_send_phy(u8 packet_type, u8 *packet, u16 size)
{
	hhy_printf("--func=%s\n", __FUNCTION__);

	u8 head_data = 4;

    switch (packet_type)
	{
        case HCI_EVENT_PACKET:
			hhy_puts("--HCI EVENT UP\n");
			hci_uart_send(&head_data, 1); //补发 header
            hci_uart_send(packet, size);
            break;
        case HCI_ACL_DATA_PACKET:
            //acl_handler(packet, size);
            break;
        default:
            break;
    }
	hhy_puts("--HCI H4 end\n");
}

#if 0
void hcitest_h4_send_cmmand(int packet_type, u8 *packet, int len)
{
    int tmp;
    hci_sbuf[0] = (u8)packet_type;
    memcpy(hci_sbuf+1,packet,len);
    hci_uart_send(hci_sbuf,len+1);

#if 1
    if((packet_type == HCI_COMMAND_DATA_PACKET) && (packet[0] == 0x6d))
    {
        tmp = func_char_to_hex(hci_sbuf, hci_set_event_mask_string);
        hci_uart_send(hci_sbuf,tmp);
        hcitest_time_dly_n10ms(2);
        tmp = func_char_to_hex(hci_sbuf, hci_le_set_event_mask_string);
        hci_uart_send(hci_sbuf,tmp);
    }
#endif
//    hci_uart_send(&packet_type,1);
//    hci_uart_send(&packet,len);
}
#endif

//--------------------------H4 RECEIVE--------------------------//
//--------------------------------------------------------------//
static void uart2_isr(void)
{
    u8 value;

    if((UT2_CON & BIT(14)) != 0) //check rx
    {
        UT2_CON |= BIT(12); //clr rx pd

        value = UT2_BUF;
		//__asm__ volatile ("csync");

#if 1
		//put_u8hex(value);
        rec_timeout_cnt = 300;
        if(hci_rbuf_cnt < HCI_UART_REC_BUFSIZE)
        {
            hci_rbuf[hci_rbuf_wpt] = value;
            hci_rbuf_cnt++;
            hci_rbuf_wpt++;
            if(hci_rbuf_wpt >= HCI_UART_REC_BUFSIZE)
            {
                hci_rbuf_wpt = 0;
//                putchar('&');
            }
        }
        else
        {}
#else
	static cnt = 0;

	if(value != 0)
		hcitest_all_buf[cnt++] = value;
	if(cnt == 4)
	{
		cnt = 0;
		printf_buf(hcitest_all_buf, 4);
	}
#endif

    }

    ILAT0_CLR = BIT(UART2_INIT);
}
REG_INIT_HANDLE(uart2_isr);


static void read_hci_rbuf_data(u8* move_addr, u16 move_len)
{
    u16 read_len, remain_len;
    CPU_SR_ALLOC();

    if(hci_rbuf_cnt < move_len)
    {
        hhy_puts("*over\n");
        return;
    }

    if ((hci_rbuf_rpt + move_len) >= HCI_UART_REC_BUFSIZE)
    {
        read_len = HCI_UART_REC_BUFSIZE - hci_rbuf_rpt;
        remain_len = move_len - read_len;
    }
    else
    {
        read_len = move_len;
        remain_len = 0;
    }

    memcpy(move_addr, hci_rbuf + hci_rbuf_rpt, read_len);

    hci_rbuf_rpt += read_len;

    if (remain_len > 0)
    {
//        putchar('#');
        memcpy(move_addr + read_len, hci_rbuf, remain_len);
        hci_rbuf_rpt = remain_len;
    }

    OS_ENTER_CRITICAL();
    hci_rbuf_cnt -= move_len;
    OS_EXIT_CRITICAL();

    if (hci_rbuf_rpt >= HCI_UART_REC_BUFSIZE)
    {
        hci_rbuf_rpt = 0;
    }

}

static bool read_hci_packet_type(hci_parse_ctl *p_ctrl)
{
    bool ret = TRUE;

    read_hci_rbuf_data(p_ctrl->header_buf, HCI_PACKET_TYPE_HEADER_LEN);
    p_ctrl->pkg_type = p_ctrl->header_buf[0];

    putchar(p_ctrl->pkg_type);

    switch (p_ctrl->pkg_type)
    {
#if (TI_CC2564_EN == 0) //BR16 LL模组
	case HCI_COMMAND_DATA_PACKET:
		p_ctrl->pkg_header_len = HCI_COMMAND_HEADER_LEN;
		break;
#else 					//TI LL模组	
    case HCI_ACL_DATA_PACKET:
        p_ctrl->pkg_header_len = HCI_ACL_HEADER_LEN;
        break;

    case HCI_SCO_DATA_PACKET:
        p_ctrl->pkg_header_len = HCI_SCO_HEADER_LEN;
        break;

    case HCI_EVENT_PACKET:
        p_ctrl->pkg_header_len = HCI_EVENT_HEADER_LEN;
        break;
#endif/*TI_CC2564_EN*/

    default:
        ret = FALSE;
        break;
    }

    p_ctrl->req_len = p_ctrl->pkg_header_len;

    return ret;
}

static bool read_hci_packet_header(hci_parse_ctl *p_ctrl)
{
    bool ret = TRUE;

    read_hci_rbuf_data((void*)(&p_ctrl->header_buf[1]), p_ctrl->req_len);

    switch (p_ctrl->pkg_type)
    {
#if (TI_CC2564_EN == 0) //BR16 LL模组
	case HCI_COMMAND_DATA_PACKET:
		p_ctrl->req_len = p_ctrl->header_buf[3];
		//hhy_printf("\np_ctrl->req_len=0x%x\n", p_ctrl->req_len);
		break;
	
#else					//TI LL模组	
    case HCI_ACL_DATA_PACKET:
        p_ctrl->req_len = ((u16)(p_ctrl->header_buf[4]) << 8) | (p_ctrl->header_buf[3]);
        break;

    case HCI_SCO_DATA_PACKET:
        p_ctrl->req_len = p_ctrl->header_buf[3];
        break;

    case HCI_EVENT_PACKET:
        p_ctrl->req_len = p_ctrl->header_buf[2];
        break;
#endif/*TI_CC2564_EN*/
    default:
        ret = FALSE;
        break;
    }

    return ret;
}


static bool read_hci_packet_data(hci_parse_ctl *p_ctrl)
{
    u32 all_len = p_ctrl->pkg_header_len + 1 + p_ctrl->req_len; //head(3 bytes) + length(1 byte) + packet len 

    if(r_cmd_len > 0)
    {
		putchar('#');
        return FALSE;
    }

	memcpy(hci_cmd_bufaddr, p_ctrl->header_buf, p_ctrl->pkg_header_len + 1); 			//get header
    read_hci_rbuf_data(hci_cmd_bufaddr + p_ctrl->pkg_header_len + 1, p_ctrl->req_len);	//get payload
    r_cmd_len = all_len; //h4 receive bytes length

	//hhy_puts("\n--whole cmd packet\n");
	//hhy_printf_buf(hci_cmd_bufaddr, r_cmd_len);

    return TRUE;
}


void api_check_uart_recieve_packet(void)
{
    bool parse_continue = TRUE;
    hci_parse_ctl *p_hci_ctrl = &hci_r_ctl_blk;

#ifdef H4_FLOW_CONTROL
    if(hci_rbuf_cnt >  HCI_UART_REC_BUFSIZE - 16)
    {
        HCI_UART_CTS_HIGH();
    }
    else
    {
        HCI_UART_CTS_LOW();
    }
#endif

__check_start:
    while(hci_rbuf_cnt < p_hci_ctrl->req_len)
    {
		//putchar('$');
        return;
    }

    switch (p_hci_ctrl->parse_status)
    {
    case UART_HCI_ST_RX_TYPE:
		//putchar('A');
        parse_continue = read_hci_packet_type(p_hci_ctrl);
        if(parse_continue == TRUE)
        {
            p_hci_ctrl->parse_status = UART_HCI_ST_RX_HEADER;
        }
        break;

    case UART_HCI_ST_RX_HEADER:
		//putchar('B');
        parse_continue = read_hci_packet_header(p_hci_ctrl);
        if (parse_continue == TRUE)
        {
            p_hci_ctrl->parse_status = UART_HCI_ST_RX_DATA;
        }
        break;

    case UART_HCI_ST_RX_DATA:
		//putchar('C');
        parse_continue = read_hci_packet_data(p_hci_ctrl);
        rec_timeout_cnt = 0;
        if(parse_continue == FALSE)
        {
            ///cmd buffer is busy
            return;
        }
        p_hci_ctrl->parse_status = UART_HCI_ST_RX_TYPE;
        p_hci_ctrl->req_len = HCI_PACKET_TYPE_HEADER_LEN;
        break;

    default:
		//putchar('D');
        parse_continue = FALSE;
        break;
    }

    if (parse_continue == FALSE)
    {
        hci_uart_rec_reset();
    }

    goto __check_start;

}

static int api_hci_h4_receive_phy(u8 **cmd_buf)
{
	int len = 0;

	*cmd_buf = hci_cmd_bufaddr + HCI_PACKET_TYPE_HEADER_LEN;
	if(r_cmd_len > 0)
    {
		len = r_cmd_len - HCI_PACKET_TYPE_HEADER_LEN; //减去 head(1 byte)
		r_cmd_len = 0;
		
        //hhy_printf("\nH4_PHY_RX(%d): ", len);
        //hhy_printf_buf(*cmd_buf, len);
    }

	return len;
}

void API_hci_send_cmd(void)
{
	int hci_cmd_len = 0;
	u8 *hci_cmd_buf = NULL;

	hci_cmd_len = api_hci_h4_receive_phy(&hci_cmd_buf);
	if(hci_cmd_len == 0)
	{
		return;
	}

	pc_h4_send_hci_cmd(hci_cmd_buf, hci_cmd_len);
}


//-----------------------H4 UART INIT---------------------------//
//--------------------------------------------------------------//
#if TI_CC2564_EN
void ti_cc2564_powerup(void)
{
    int tmp;

    hhy_printf("\n %s\n", __FUNCTION__);

    HCI_UART_SHUTD_LOW();
    hcitest_time_dly_n10ms(20);
    HCI_UART_SHUTD_HIGH();
    hcitest_time_dly_n10ms(300);

    if(HCI_UART_RTS_READ() != 0)
    {
        hhy_puts("hci:cc2564 power up fail!!!\n");
    }
    else
    {
        hhy_puts("hci:cc2564 power up success \n");
    }

    HCI_UART_CTS_LOW();

    tmp = func_char_to_hex(hci_sbuf,hci_reset_cmd_string);
    hci_uart_send(hci_sbuf,tmp);
    hcitest_time_dly(200);
    tmp = func_char_to_hex(hci_sbuf,hci_le_enable_cmd_string);
    hci_uart_send(hci_sbuf,tmp);
    hcitest_time_dly(10);

    hhy_printf_buf(hci_rbuf,hci_rbuf_cnt);

    hci_uart_rec_reset();
    r_cmd_len = 0;
    hcitest_run = 0x55;
}
#endif//TI_CC2564_EN

void hci_uart_exit(void)
{
    hhy_printf("\n %s \n",__FUNCTION__);
    clear_ie(UART2_INIT);///disable irq
    HCI_UART_OUT_PIN_DIS();
    HCI_UART_IN_PIN_DIS();
}

void hci_uart_rec_reset(void)
{
    CPU_SR_ALLOC();
    OS_ENTER_CRITICAL();

    hci_rbuf_rpt = 0;
    hci_rbuf_wpt = 0;
    hci_rbuf_cnt = 0;
    rec_timeout_cnt = 0;
    hci_r_ctl_blk.parse_status = UART_HCI_ST_RX_TYPE;
    hci_r_ctl_blk.req_len = HCI_PACKET_TYPE_HEADER_LEN;
    hci_r_ctl_blk.pkg_type = 0;

    OS_EXIT_CRITICAL();
    hhy_puts("hci:uart reset\n");
}

void hci_uart_init(void)
{
    hhy_printf("\n %s\n",__FUNCTION__);

    ///UART2配置
    hhy_puts("hci_uart: TX--PA10，RX--PA11\n");
    IOMC1 &= ~(BIT(15)|BIT(14));
    IOMC1 |= BIT(14);
    PORTA_OUT |= BIT(10) ;
    PORTA_DIR |= BIT(11) ;
    PORTA_DIR &=~BIT(10) ;

   //初始化，配置RX中断，波特率115200
    UT2_BAUD = (64000000/115200)/4 -1;//115200
    UT2_CON = BIT(13) | BIT(12) ; //rx_ie , CLR RX PND

    INTALL_HWI(UART2_INIT, uart2_isr, 3) ; //注 册中断
    UT2_CON |= (BIT(3) | BIT(0)) ;//enable rx isr

    HCI_UART_OUT_PIN_EN();
    HCI_UART_IN_PIN_DIS();
    HCI_UART_CTS_LOW();

    HCI_UART_PA12_HIGH();

#ifdef HCITEST_NOT_MALLOC
    hci_sbuf = hcitest_all_buf;
#else
    hci_sbuf = malloc(HCI_UART_REC_BUFSIZE + HCI_UART_SEND_BUFSIZE + HCI_CMD_BUFSIZE);
#endif // HCITEST_HAVE_MALLOC

    if(hci_sbuf == NULL)
    {
        hhy_puts("hci:uart mallo fail \n");
        return;
    }
    else
    {
        hhy_printf("hci_sbuf = 0x%x\n",(int)hci_sbuf);
    }

    hci_rbuf = hci_sbuf + HCI_UART_SEND_BUFSIZE;
    hci_cmd_bufaddr = hci_sbuf + HCI_UART_SEND_BUFSIZE + HCI_UART_REC_BUFSIZE;
    hci_rbuf_rpt = 0;
    hci_rbuf_wpt = 0;
    hci_rbuf_cnt = 0;
    rec_timeout_cnt = 0;
    hci_r_ctl_blk.parse_status = UART_HCI_ST_RX_TYPE;
    hci_r_ctl_blk.req_len = HCI_PACKET_TYPE_HEADER_LEN;
    hci_r_ctl_blk.pkg_type = 0;
    r_cmd_len = 0;

#if TI_CC2564_EN
    ti_cc2564_powerup();
#endif//TI_CC2564_EN

#ifdef HHY_DEBUG
	///<huayue add
	le_hci_register_packet_handler(api_hci_h4_send_phy); //register h4 send interface
#endif
}

