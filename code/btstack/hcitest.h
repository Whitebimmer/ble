/*
*********************************************************************************************************
*                                                AC46
*                                      HCI TEST
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

#ifndef __HCI_TEST_H__
#define __HCI_TEST_H__

#include "typedef.h"
#include "hwi.h"


#define HCI_UART_REC_BUFSIZE    1024
#define HCI_UART_SEND_BUFSIZE   128
#define HCI_CMD_BUFSIZE         256


#define HCI_PACKET_TYPE_HEADER_LEN     	1
#define HCI_COMMAND_HEADER_LEN     		3
#define HCI_ACL_HEADER_LEN             	4
#define HCI_SCO_HEADER_LEN             	3
#define HCI_EVENT_HEADER_LEN           	2


///hcitest_control
///#define HCITEST_CORE42_CMD_SUPPORT

///#define TI_CONTROLLER_TEST     ///TI模组测试


#ifdef TI_CONTROLLER_TEST
#define H4_FLOW_CONTROL  ///流控打开
#endif


typedef enum
{
    REPORT_ADV_IND,
    REPORT_ADV_DIRECT_IND,
    REPORT_ADV_SCAN_IND,
    REPORT_ADV_NONCONN_IND,
    REPORT_SCAN_RSP,
} eport_type_e;


typedef enum
{
    UART_HCI_ST_RX_NULL = 0,
    UART_HCI_ST_RX_TYPE,
    UART_HCI_ST_RX_HEADER,
    UART_HCI_ST_RX_DATA,
} parse_status_e;

typedef struct
{
    u8 header_buf[8];
    u8 parse_status;
    u8 pkg_type;
    u16 pkg_header_len;
    u16 req_len;
    u16 Reserved;
} hci_parse_ctl;


typedef struct
{
    u16 handle:12;
    u16 PB_flag:2;
    u16 BC_flag:2;
    u16 total_len;
    u16 l2cap_len;
    u16 channel_id;
    u8  data[1];
}acl_pkt_t;


typedef struct
{
    u8 type;
    u8 address[6];
    u8 Reserved;
    u16 conn_handle;
}remote_tbl_t;


///PA13,PA14,PA15
#define UART_IN_ALL   BIT(13)
#define UART_OUT_ALL (BIT(12)|BIT(8)|BIT(9))

#define HCI_UART_OUT_PIN_EN()    {PORTA_PU &= ~UART_OUT_ALL;PORTA_PD &= ~UART_OUT_ALL;PORTA_DIR &= ~UART_OUT_ALL;}
#define HCI_UART_OUT_PIN_DIS()   {PORTA_PU &= ~UART_OUT_ALL;PORTA_PD &= ~UART_OUT_ALL;PORTA_DIR |= UART_OUT_ALL;}

#define HCI_UART_IN_PIN_EN()     {PORTA_PU |= UART_IN_ALL;PORTA_DIR |= UART_IN_ALL;}
#define HCI_UART_IN_PIN_DIS()    {PORTA_PU |= UART_IN_ALL;PORTA_DIR |= UART_IN_ALL;}

#define HCI_UART_RTS_READ()      (PORTA_IN &BIT(13))

#define HCI_UART_CTS_HIGH()      PORTA_OUT |= BIT(8)
#define HCI_UART_CTS_LOW()       PORTA_OUT &= ~BIT(8)

#define HCI_UART_SHUTD_HIGH()    PORTA_OUT |= BIT(9)
#define HCI_UART_SHUTD_LOW()     PORTA_OUT &= ~BIT(9)

#define HCI_UART_PA12_HIGH()     PORTA_OUT |= BIT(12)
#define HCI_UART_PA12_LOW()      PORTA_OUT &= ~BIT(12)


extern u8 *hci_cmd_bufaddr;
extern u8 *hci_sbuf;
extern u8 *hci_rbuf;

extern u16 hci_sbuf_rpt;
extern u16 hci_sbuf_wpt;
extern u16 hci_sbuf_cnt;

extern u16 hci_rbuf_rpt;
extern u16 hci_rbuf_wpt;
extern u16 hci_rbuf_cnt;

extern u16 hci_cmd_buf_cnt;
///extern cbuffer_t *hci_cmd_cbuff;

extern int func_char_to_hex(u8 *hex_buf,u8 *char_buf);

#endif /* #ifndef __HCI_TEST_H__ */
