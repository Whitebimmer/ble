#include "LL_TS_4_2_1_hci_test.h"
#include "hci_cmds.h"


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


typedef struct
{
    char *message;         //提示字符串
    u8  *cmd_str;          //命令表
    //u32 cmd_len;           //命令长度
    //u8  respond_mode;      //命令响应模式
    //u8  respond_type;      //命令响应类型
    //u16 respond_cmd;       //应答的命令，event or acl or sco
    //u16 wait_timeout;       //等待超时时间，0为永不超时
    //u16 cause_event_opcode; //引起event 的 opcode，NULL为空
}hci_test_cmd_t;

typedef struct
{
    char *message;
    int  *table_addr;
    int  *cmd_cnt;
}test_flow_struct_t;


//<global var
static u8 g_cmd_cnt = 0;
static u8 hhy_hci_buf[512];
static int g_steps_cnt = 0;
#define UART_RX_LEN     2
static volatile char uart_rx_buf[UART_RX_LEN];


//<extern func
extern int pc_h4_send_hci_cmd(void *data, u16 size);


///******************************************************************************<table

///--------------------------------
///interval((min:0x20)),non conn,public addr,use channel 39,pol-0
const u8 Send_HCI_LE_Set_Adv_Param_Nconn_publicAddr_c39_pol0[] =
{"01 06 20 0f 20 00 40 00 03 00 00 00 00 00 00 00 00 04 00"};
const hci_test_cmd_t HT_Send_HCI_LE_Set_Adv_Param_Nconn_publicAddr_c39_pol3 =
{
    .message = "Send_HCI_LE_Set_Adv_Param_Nconn_publicAddr_c39_pol0",
    .cmd_str = Send_HCI_LE_Set_Adv_Param_Nconn_publicAddr_c39_pol0,
    //.respond_mode = RESP_OSSEM,
    //.respond_type = HCI_EVENT_PACKET,
    //.respond_cmd = HCI_COMMAND_COMPLETE_EVENT_CODE,
    //.wait_timeout = 300,
    //.cause_event_opcode = HCI_LE_SET_ADV_PARAM,
};

///interval((min:0x20)),non conn,random addr,use channel 39,pol-0
const u8 Send_HCI_LE_Set_Adv_Param_Nconn_randomAddr_c39_pol0[] =
{"01 06 20 0f 20 00 40 00 03 01 00 00 00 00 00 00 00 04 00"};
const hci_test_cmd_t HT_Send_HCI_LE_Set_Adv_Param_Nconn_randomAddr_c39_pol3 =
{
    .message = "Send_HCI_LE_Set_Adv_Param_Nconn_randomAddr_c39_pol0",
    .cmd_str = Send_HCI_LE_Set_Adv_Param_Nconn_randomAddr_c39_pol0,
    //.respond_mode = RESP_OSSEM,
    //.respond_type = HCI_EVENT_PACKET,
    //.respond_cmd = HCI_COMMAND_COMPLETE_EVENT_CODE,
    //.wait_timeout = 300,
    //.cause_event_opcode = HCI_LE_SET_ADV_PARAM,
};

///interval((min:0x20)),non,conn,public addr,pol-3
const u8 Send_HCI_LE_Set_Adv_Param_Nconn_publicAddr_pol3[] =
{"01 06 20 0f 20 00 40 00 03 00 00 00 00 00 00 00 00 07 03"};
const hci_test_cmd_t HT_Send_HCI_LE_Set_Adv_Param_Nconn_publicAddr_pol3 =
{
    .message = "Send_HCI_LE_Set_Adv_Param_Nconn_publicAddr_pol3",
    .cmd_str = Send_HCI_LE_Set_Adv_Param_Nconn_publicAddr_pol3,
    //.respond_mode = RESP_OSSEM,
    //.respond_type = HCI_EVENT_PACKET,
    //.respond_cmd = HCI_COMMAND_COMPLETE_EVENT_CODE,
    //.wait_timeout = 300,
    //.cause_event_opcode = HCI_LE_SET_ADV_PARAM,
};

///interval(min:0x20),conn undir,public addr,pol-3
const u8 Send_HCI_LE_Set_Adv_Param_conn_undir_publicAddr_pol3[] =
{"01 06 20 0f 20 00 40 00 00 00 00 00 00 00 00 00 00 07 03"};
const hci_test_cmd_t HT_Send_HCI_LE_Set_Adv_Param_conn_undir_publicAddr_pol3 =
{
    .message = "Send_HCI_LE_Set_Adv_Param_conn_undir_publicAddr_pol3",
    .cmd_str = Send_HCI_LE_Set_Adv_Param_conn_undir_publicAddr_pol3,
    //.respond_mode = RESP_OSSEM,
    //.respond_type = HCI_EVENT_PACKET,
    //.respond_cmd = HCI_COMMAND_COMPLETE_EVENT_CODE,
    //.wait_timeout = 300,
    //.cause_event_opcode = HCI_LE_SET_ADV_PARAM,
};

///interval(min:0x20),conn undir,public addr,pol-0
const u8 Send_HCI_LE_Set_Adv_Param_conn_undir_publicAddr_pol0[] =
{"01 06 20 0f 20 00 40 00 00 00 00 00 00 00 00 00 00 07 00"};
const hci_test_cmd_t HT_Send_HCI_LE_Set_Adv_Param_conn_undir_publicAddr_pol0 =
{
    .message = "Send_HCI_LE_Set_Adv_Param_conn_undir_publicAddr_pol0",
    .cmd_str = Send_HCI_LE_Set_Adv_Param_conn_undir_publicAddr_pol0,
    //.respond_mode = RESP_OSSEM,
    //.respond_type = HCI_EVENT_PACKET,
    //.respond_cmd = HCI_COMMAND_COMPLETE_EVENT_CODE,
    //.wait_timeout = 300,
    //.cause_event_opcode = HCI_LE_SET_ADV_PARAM,
};

///interval(min:0x20),conn undir,public addr,pol-1
const u8 Send_HCI_LE_Set_Adv_Param_conn_undir_publicAddr_pol1[] =
{"01 06 20 0f 20 00 40 00 00 00 00 00 00 00 00 00 00 07 01"};
const hci_test_cmd_t HT_Send_HCI_LE_Set_Adv_Param_conn_undir_publicAddr_pol1 =
{
    .message = "Send_HCI_LE_Set_Adv_Param_conn_undir_publicAddr_pol1",
    .cmd_str = Send_HCI_LE_Set_Adv_Param_conn_undir_publicAddr_pol1,
    //.respond_mode = RESP_OSSEM,
    //.respond_type = HCI_EVENT_PACKET,
    //.respond_cmd = HCI_COMMAND_COMPLETE_EVENT_CODE,
    //.wait_timeout = 300,
    //.cause_event_opcode = HCI_LE_SET_ADV_PARAM,
};

///--------------------------------
const u8 Send_HCI_LE_Set_Advertising_Data_len00[] =
{"01 08 20 20 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"};
const hci_test_cmd_t HT_Send_HCI_LE_Set_Advertising_Data_len00 =
{
    .message = "Send_HCI_LE_Set_Advertising_Data_len00",
    .cmd_str = Send_HCI_LE_Set_Advertising_Data_len00,
    //.respond_mode = RESP_OSSEM,
    //.respond_type = HCI_EVENT_PACKET,
    //.respond_cmd = HCI_COMMAND_COMPLETE_EVENT_CODE,
    //.wait_timeout = 300,
    //.cause_event_opcode = HCI_LE_SET_ADV_DATA,
};

const u8 Send_HCI_LE_Set_Advertising_Data_len31[] =
{"01 08 20 20 1f 1e 19 30 31 32 33 34 35 36 37 38 39 30 31 32 33 34 35 36 37 38 39 30 31 32 33 34 35 36 37 38"};
const hci_test_cmd_t HT_Send_HCI_LE_Set_Advertising_Data_len31 =
{
    .message = "Send_HCI_LE_Set_Advertising_Data_len31",
    .cmd_str = Send_HCI_LE_Set_Advertising_Data_len31,
    //.respond_mode = RESP_OSSEM,
    //.respond_type = HCI_EVENT_PACKET,
    //.respond_cmd = HCI_COMMAND_COMPLETE_EVENT_CODE,
    //.wait_timeout = 300,
    //.cause_event_opcode = HCI_LE_SET_ADV_DATA,
};

///--------------------------------
const u8 Send_HCI_LE_Set_Advertise_Enable[] =
{"01 0a 20 01 01"};
const hci_test_cmd_t HT_Send_HCI_LE_Set_Advertise_Enable =
{
    .message = "Send_HCI_LE_Set_Advertise_Enable",
    .cmd_str = Send_HCI_LE_Set_Advertise_Enable,
    //.respond_mode = RESP_OSSEM,
    //.respond_type = HCI_EVENT_PACKET,
    //.respond_cmd = HCI_COMMAND_COMPLETE_EVENT_CODE,
    //.wait_timeout = 300,
    //.cause_event_opcode = HCI_LE_SET_ADV_ENABLE,
};

const u8 Send_HCI_LE_Set_Advertise_Disable[] =
{"01 0a 20 01 00"};

const hci_test_cmd_t HT_Send_HCI_LE_Set_Advertise_Disable =
{
    .message = "Send_HCI_LE_Set_Advertise_Disable",
    .cmd_str = Send_HCI_LE_Set_Advertise_Disable,
    //.respond_mode = RESP_OSSEM,
    //.respond_type = HCI_EVENT_PACKET,
    //.respond_cmd = HCI_COMMAND_COMPLETE_EVENT_CODE,
    //.wait_timeout = 300,
    //.cause_event_opcode = HCI_LE_SET_ADV_ENABLE,
};

///--------------------------------
///rsp data length:0, rsp data:"IUT"
const u8 Send_HCI_LE_Set_Scan_Response_Length_0_Data_IUT[] =
{"01 09 20 20 00 04 09 49 55 54 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"};
const hci_test_cmd_t HT_Send_HCI_LE_Set_Scan_Response_Length_0_Data_IUT =
{
    .message = "HT_Send_HCI_LE_Set_Scan_Response_Length_0_Data_IUT",
    .cmd_str = Send_HCI_LE_Set_Scan_Response_Length_0_Data_IUT,
    //.respond_mode = RESP_OSSEM,
    //.respond_type = HCI_EVENT_PACKET,
    //.respond_cmd = HCI_COMMAND_COMPLETE_EVENT_CODE,
    //.wait_timeout = 300,
    //.cause_event_opcode = HCI_LE_SET_SCAN_RSP_DATA,
};

///rsp data length:0x1f, rsp data:"IUT"
const u8 Send_HCI_LE_Set_Scan_Response_Length_31_Data_IUT[] =
{"01 09 20 20 1f 04 09 49 55 54 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"};
const hci_test_cmd_t HT_Send_HCI_LE_Set_Scan_Response_Length_31_Data_IUT =
{
    .message = "HT_Send_HCI_LE_Set_Scan_Response_Length_31_Data_IUT",
    .cmd_str = Send_HCI_LE_Set_Scan_Response_Length_31_Data_IUT,
    //.respond_mode = RESP_OSSEM,
    //.respond_type = HCI_EVENT_PACKET,
    //.respond_cmd = HCI_COMMAND_COMPLETE_EVENT_CODE,
    //.wait_timeout = 300,
    //.cause_event_opcode = HCI_LE_SET_SCAN_RSP_DATA,
};

///--------------------------------
const u8 Send_HCI_LE_Set_white_list[] =
{"01 11 20 07 00 f6 c9 f5 88 7a 84"};
const hci_test_cmd_t HT_Send_HCI_LE_Set_white_list =
{
    .message = "Send_HCI_LE_Set_white_list",
    .cmd_str = Send_HCI_LE_Set_white_list,
    //.respond_mode = RESP_OSSEM,
    //.respond_type = HCI_EVENT_PACKET,
    //.respond_cmd = HCI_COMMAND_COMPLETE_EVENT_CODE,
    //.wait_timeout = 300,
    //.cause_event_opcode = HCI_LE_ADD_WHITE_LIST,
};

///--------------------------------
const u8 Send_HCI_LE_Set_random_address[] =
{"01 05 20 06 aa c9 f5 88 7a aa"};
const hci_test_cmd_t HT_Send_HCI_LE_Set_random_address =
{
    .message = "Send_HCI_LE_Set_random_address",
    .cmd_str = Send_HCI_LE_Set_random_address,
    //.respond_mode = RESP_OSSEM,
    //.respond_type = HCI_EVENT_PACKET,
    //.respond_cmd = HCI_COMMAND_COMPLETE_EVENT_CODE,
    //.wait_timeout = 300,
    //.cause_event_opcode = HCI_LE_ADD_WHITE_LIST,
};


///--------------------------------
const int tp_ddi_adv_bv01c[] =
{
    (int)&HT_Send_HCI_LE_Set_Adv_Param_Nconn_publicAddr_pol3,//
    (int)&HT_Send_HCI_LE_Set_Advertising_Data_len00,
    (int)&HT_Send_HCI_LE_Set_Advertise_Enable,
    (int)5000,
    (int)&HT_Send_HCI_LE_Set_Advertise_Disable,
    (int)200,
};

const int tp_ddi_adv_bv02c[] =
{
    (int)&HT_Send_HCI_LE_Set_Adv_Param_conn_undir_publicAddr_pol3,//
    (int)&HT_Send_HCI_LE_Set_Advertising_Data_len00,
    (int)&HT_Send_HCI_LE_Set_Advertise_Enable,
    (int)2000,
    (int)&HT_Send_HCI_LE_Set_Advertise_Disable,
    (int)200,
};

const int tp_ddi_adv_bv03c[] =
{
    (int)&HT_Send_HCI_LE_Set_Adv_Param_Nconn_publicAddr_pol3,//
    (int)&HT_Send_HCI_LE_Set_Advertising_Data_len31,
    (int)&HT_Send_HCI_LE_Set_Advertise_Enable,
    (int)2000,
    (int)&HT_Send_HCI_LE_Set_Advertise_Disable,
    (int)&HT_Send_HCI_LE_Set_Advertising_Data_len00,
    (int)&HT_Send_HCI_LE_Set_Advertise_Enable,
    (int)2000,
    (int)&HT_Send_HCI_LE_Set_Advertise_Disable,
    (int)200,
};

const int tp_ddi_adv_bv04c[] =
{
    (int)&HT_Send_HCI_LE_Set_Adv_Param_conn_undir_publicAddr_pol3,//note while list
    (int)&HT_Send_HCI_LE_Set_Advertising_Data_len31,
    (int)&HT_Send_HCI_LE_Set_Advertise_Enable,
    (int)2000,
    (int)&HT_Send_HCI_LE_Set_Advertise_Disable,
    (int)&HT_Send_HCI_LE_Set_Advertising_Data_len00,
    (int)&HT_Send_HCI_LE_Set_Advertise_Enable,
    (int)2000,
    (int)&HT_Send_HCI_LE_Set_Advertise_Disable,
    (int)200,
};

const int tp_ddi_adv_bv05c_support_filter[] =
{
	//step 1~6
    (int)&HT_Send_HCI_LE_Set_Adv_Param_conn_undir_publicAddr_pol0,//
    (int)&HT_Send_HCI_LE_Set_Scan_Response_Length_31_Data_IUT,
    (int)&HT_Send_HCI_LE_Set_Advertise_Enable,
    (int)2000,
    (int)&HT_Send_HCI_LE_Set_Advertise_Disable,
    (int)200,
	//step 7~8
	//(int)&HT_Send_HCI_LE_Set_white_list,//
	(int)&HT_Send_HCI_LE_Set_Adv_Param_conn_undir_publicAddr_pol1,//
    (int)&HT_Send_HCI_LE_Set_Scan_Response_Length_0_Data_IUT,
    (int)&HT_Send_HCI_LE_Set_Advertise_Enable,
    (int)2000,
    (int)&HT_Send_HCI_LE_Set_Advertise_Disable,
    (int)200,
#if 0
    //step 9~10
    //(int)&HT_Send_HCI_LE_Set_white_list,//
    (int)&HT_Send_HCI_LE_Set_Adv_Param_conn_undir_publicAddr_pol1,//
    (int)&HT_Send_HCI_LE_Set_Scan_Response_Length_31_Data_IUT,
    (int)&HT_Send_HCI_LE_Set_Advertise_Enable,
    (int)2000,
    (int)&HT_Send_HCI_LE_Set_Advertise_Disable,
    (int)200,
    //step 11~14
    (int)&HT_Send_HCI_LE_Set_Adv_Param_conn_undir_publicAddr_pol0,//
    (int)&HT_Send_HCI_LE_Set_Scan_Response_Length_0_Data_IUT,
    (int)&HT_Send_HCI_LE_Set_Advertise_Enable,
    (int)5000,
    (int)&HT_Send_HCI_LE_Set_Advertise_Disable,
    (int)200,
#endif
};

const int tp_ddi_adv_bv06c_support_filter[] =
{
	//step 1~6
    (int)&HT_Send_HCI_LE_Set_Adv_Param_conn_undir_publicAddr_pol0,//
    (int)&HT_Send_HCI_LE_Set_Advertise_Enable,
    (int)2000,
    (int)&HT_Send_HCI_LE_Set_Advertise_Disable,
    (int)200,
	//step 7~8
	//(int)&HT_Send_HCI_LE_Set_white_list,//
	(int)&HT_Send_HCI_LE_Set_Adv_Param_conn_undir_publicAddr_pol1,//
    (int)&HT_Send_HCI_LE_Set_Scan_Response_Length_0_Data_IUT,
    (int)&HT_Send_HCI_LE_Set_Advertise_Enable,
    (int)2000,
    (int)&HT_Send_HCI_LE_Set_Advertise_Disable,
    (int)200,
};

const int tp_ddi_adv_tester_for_scan_bv02c_pbulicAddr[] =
{
    (int)&HT_Send_HCI_LE_Set_Adv_Param_Nconn_publicAddr_c39_pol3,
    (int)&HT_Send_HCI_LE_Set_Advertise_Enable,
    (int)8000,
    (int)&HT_Send_HCI_LE_Set_Advertise_Disable,
    (int)200,
};

const int tp_ddi_adv_tester_for_scan_bv02c_randomAddr[] =
{
    (int)&HT_Send_HCI_LE_Set_random_address,
	(int)&HT_Send_HCI_LE_Set_Adv_Param_Nconn_randomAddr_c39_pol3,
    (int)&HT_Send_HCI_LE_Set_Advertise_Enable,
    (int)8000,
    (int)&HT_Send_HCI_LE_Set_Advertise_Disable,
    (int)200,
};

///--------------------------------
const test_flow_struct_t test_flow_linklayer_adv[] =
{
    {///
        .message = "tp_ddi_adv_bv01c",
        .table_addr = tp_ddi_adv_bv01c,
        .cmd_cnt = sizeof(tp_ddi_adv_bv01c)/sizeof(int),
    },
    {///
        .message = "tp_ddi_adv_bv02c",
        .table_addr = tp_ddi_adv_bv02c,
        .cmd_cnt = sizeof(tp_ddi_adv_bv02c)/sizeof(int),
    },
	{///
	    .message = "tp_ddi_adv_bv03c",
	    .table_addr = tp_ddi_adv_bv03c,
	    .cmd_cnt = sizeof(tp_ddi_adv_bv03c)/sizeof(int),
    },
	{///
	    .message = "tp_ddi_adv_bv04c",
	    .table_addr = tp_ddi_adv_bv04c,
	    .cmd_cnt = sizeof(tp_ddi_adv_bv04c)/sizeof(int),
    },
    {///
	    .message = "tp_ddi_adv_bv05c_support_filter",
	    .table_addr = tp_ddi_adv_bv05c_support_filter,
	    .cmd_cnt = sizeof(tp_ddi_adv_bv05c_support_filter)/sizeof(int),
    },

	{///
	    .message = "tp_ddi_adv_tester_for_scan_bv02c_pbulicAddr",
	    .table_addr = tp_ddi_adv_tester_for_scan_bv02c_pbulicAddr,
	    .cmd_cnt = sizeof(tp_ddi_adv_tester_for_scan_bv02c_pbulicAddr)/sizeof(int),
    },

	{///
	    .message = "tp_ddi_adv_tester_for_scan_bv02c_randomAddr",
	    .table_addr = tp_ddi_adv_tester_for_scan_bv02c_randomAddr,
	    .cmd_cnt = sizeof(tp_ddi_adv_tester_for_scan_bv02c_randomAddr)/sizeof(int),
    },
};

///--------------------------------
///--------------------------------
///-----------"for privacy"--------
///--------------------------------
///--------------------------------
///interval(min:0x20),conn undir,public addr
const u8 Send_HCI_LE_Set_Adv_Param[] =
{"01 06 20 0f 20 00 20 00 00 00 00 00 00 00 00 00 00 01 00"};
const hci_test_cmd_t HCI_LE_Set_Adv_Param=
{
    .message = "Send_HCI_LE_Set_Adv_Param",
    .cmd_str = Send_HCI_LE_Set_Adv_Param,
};

///dev_name:"br17-4.2"
const u8 Send_HCI_LE_Set_Adv_data[] =
{"01 08 20 17 16 02 01 06 09 09 62 72 31 37 2D 34 2E 32 04 0D 00 05 10 03 03 0D 18"};
const hci_test_cmd_t HCI_LE_Set_Adv_data=
{
    .message = "Send_HCI_LE_Set_Adv_data",
    .cmd_str = Send_HCI_LE_Set_Adv_data,
};

///dev_name:"br17-4.2"
const u8 Send_HCI_LE_Set_Adv_rsp_data[] =
{"01 09 20 20 1f 1E FF 57 01 00 7C 5B 5D 17 0E 02 BD D7 0C BB 8A CC C3 3C F2 86 0F 00 00 00 88 0F 10 53 49 56"};
const hci_test_cmd_t HCI_LE_Set_Adv_rsp_data=
{
    .message = "Send_HCI_LE_Set_Adv_rsp_data",
    .cmd_str = Send_HCI_LE_Set_Adv_rsp_data,
};

const int tp_normal_adv[] =
{
    (int)&HCI_LE_Set_Adv_Param,
    (int)&HCI_LE_Set_Adv_data,
    (int)&HCI_LE_Set_Adv_rsp_data,
    (int)&HT_Send_HCI_LE_Set_Advertise_Enable,
    (int)9000,
    (int)&HT_Send_HCI_LE_Set_Advertise_Disable,
    (int)200,
};

/* .peer_identity_address_type = 0, */
/* .peer_identity_address = {0x46, 0x01, 0x70, 0xAC, 0xF5, 0xBC},  //LSB->MSB */
/* .peer_irk = {0x74, 0x01, 0x90, 0x4A, 0xF7, 0x35, 0xA4, 0x38, 0xD3, 0x20, 0x9A, 0xCD, 0x48, 0xC0, 0x54, 0x40}, */
/* peer_addr = f6 c9 f5 88 7a 84 */
const u8 HT_Send_HCI_LE_Add_Device_To_Resolving_List_local_irk_allzero[] =
{"01 27 20 27 00 F6 C9 F5 88 7A 84 74 01 90 4A F7 35 A4 38 D3 20 9A CD 48 C0 54 40 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"};
/* {"01 27 20 27 00 F6 C9 F5 88 7A 84 74 01 90 4A F7 35 A4 38 D3 20 9A CD 48 C0 54 40 E6 EA EE 60 31 7B FC A2 3F A5 79 59 E7 41 CF C7"}; */
const hci_test_cmd_t HCI_LE_Add_Device_To_Resolving_List_local_irk_allzero=
{
    .message = "HCI_LE_Add_Device_To_Resolving_List_local_irk_allzero",
    .cmd_str = HT_Send_HCI_LE_Add_Device_To_Resolving_List_local_irk_allzero,
};

const u8 HT_Send_HCI_LE_Set_Address_Resolution_Enable_en[] = 
{"01 2D 20 01 01"};
const hci_test_cmd_t HCI_LE_Set_Address_Resolution_Enable_en=
{
    .message = "HCI_LE_Set_Address_Resolution_Enable_en",
    .cmd_str = HT_Send_HCI_LE_Set_Address_Resolution_Enable_en,
};

//ramdom addr= {0x3e, 0x3a, 0xba, 0x98, 0x33, 0x11},
const u8 HT_Send_HCI_LE_Set_Random_Address_nonresolvable_private[] =
{"01 05 20 06 3e 3a ba 98 33 11"}; 
const hci_test_cmd_t HCI_LE_Set_Random_Address_nonresolvable_private=
{
    .message = "HCI_LE_Set_Random_Address_nonresolvable_private",
    .cmd_str = HT_Send_HCI_LE_Set_Random_Address_nonresolvable_private,
};

///interval((min:0x20)),non conn,own address 3,use channel 37,pol-0
const u8 Send_HCI_LE_Set_Advertising_Parameters_nonconn_ownaddr3[] =
{"01 06 20 0f 20 00 40 00 03 03 00 F6 C9 F5 88 7A 84 01 00"};
const hci_test_cmd_t HCI_LE_Set_Advertising_Parameters_nonconn_ownaddr3=
{
    .message = "HCI_LE_Set_Advertising_Parameters_nonconn_ownaddr3",
    .cmd_str = Send_HCI_LE_Set_Advertising_Parameters_nonconn_ownaddr3,
};

///dev_name:"br17-4.2" data_length:0
const u8 Send_HCI_LE_Set_Advertising_Data_datalen0[] =
{"01 08 20 17 00 02 01 06 09 09 62 72 31 37 2D 34 2E 32 04 0D 00 05 10 03 03 0D 18"};
const hci_test_cmd_t HCI_LE_Set_Advertising_Data_datalen0=
{
    .message = "HCI_LE_Set_Advertising_Data_datalen0",
    .cmd_str = Send_HCI_LE_Set_Advertising_Data_datalen0,
};

const int tp_sec_adv_bv02c[] =
{
    (int)&HCI_LE_Add_Device_To_Resolving_List_local_irk_allzero,
    (int)&HCI_LE_Set_Address_Resolution_Enable_en,
    (int)&HCI_LE_Set_Random_Address_nonresolvable_private,
    (int)&HCI_LE_Set_Advertising_Parameters_nonconn_ownaddr3,
    (int)&HCI_LE_Set_Advertising_Data_datalen0,
    (int)&HT_Send_HCI_LE_Set_Advertise_Enable,
    (int)9000,
    (int)&HT_Send_HCI_LE_Set_Advertise_Disable,
    (int)200,
};

/* peer_addr = 3E 3A BA 98 11 11 */
const u8 HT_Send_HCI_LE_Add_Device_To_White_List[] =
{"01 11 20 07 00 3E 3A BA 98 11 71"};
const hci_test_cmd_t HCI_LE_Add_Device_To_White_List=
{
    .message = "HCI_LE_Add_Device_To_White_List",
    .cmd_str = HT_Send_HCI_LE_Add_Device_To_White_List,
};

/* .peer_identity_address_type = 0, */
/* .peer_identity_address = {0x46, 0x01, 0x70, 0xAC, 0xF5, 0xBC},  //LSB->MSB */
/* .peer_irk = {0x74, 0x01, 0x90, 0x4A, 0xF7, 0x35, 0xA4, 0x38, 0xD3, 0x20, 0x9A, 0xCD, 0x48, 0xC0, 0x54, 0x40}, */
/* peer_addr = 3E 3A BA 98 11 71 */
const u8 HT_Send_HCI_LE_Add_Device_To_Resolving_List_local_irk[] =
{"01 27 20 27 00 3E 3A BA 98 11 71 74 01 90 4A F7 35 A4 38 D3 20 9A CD 48 C0 54 40 E6 EA EE 60 31 7B FC A2 3F A5 79 59 E7 41 CF C7"};
const hci_test_cmd_t HCI_LE_Add_Device_To_Resolving_List_local_irk=
{
    .message = "HCI_LE_Add_Device_To_Resolving_List_local_irk",
    .cmd_str = HT_Send_HCI_LE_Add_Device_To_Resolving_List_local_irk,
};

//timeout = 0x0384
const u8 HT_Send_HCI_LE_Set_Random_Private_Address_Timeout[] =
{"01 2e 20 02 B8 A1"};
const hci_test_cmd_t HCI_LE_Set_Random_Private_Address_Timeout=
{
    .message = "HCI_LE_Set_Random_Private_Address_Timeout",
    .cmd_str = HT_Send_HCI_LE_Set_Random_Private_Address_Timeout,
};

///interval((min:0x20)),non conn,own address 3,use channel 37,pol-3
const u8 Send_HCI_LE_Set_Advertising_Parameters_nonconn_ownaddr3_pol3[] =
{"01 06 20 0f 20 00 40 00 03 03 00 F6 C9 F5 88 7A 84 01 03"};
const hci_test_cmd_t HCI_LE_Set_Advertising_Parameters_nonconn_ownaddr3_pol3=
{
    .message = "HCI_LE_Set_Advertising_Parameters_nonconn_ownaddr3_pol3",
    .cmd_str = Send_HCI_LE_Set_Advertising_Parameters_nonconn_ownaddr3_pol3,
};

const int tp_sec_adv_bv03c[] =
{
    (int)&HCI_LE_Add_Device_To_White_List,
    (int)&HCI_LE_Add_Device_To_Resolving_List_local_irk,
    (int)&HCI_LE_Set_Random_Private_Address_Timeout,
    (int)&HCI_LE_Set_Advertising_Parameters_nonconn_ownaddr3_pol3,
    (int)&HCI_LE_Set_Advertising_Data_datalen0,
    (int)&HCI_LE_Set_Address_Resolution_Enable_en,
    (int)&HT_Send_HCI_LE_Set_Advertise_Enable,
    (int)9000,
    (int)&HT_Send_HCI_LE_Set_Advertise_Disable,
    (int)200,
};

///interval((min:0x20)),scan IND,own address 3,use channel 37,pol-3
const u8 Send_HCI_LE_Set_Advertising_Parameters_scanind_ownaddr3_pol3[] =
{"01 06 20 0f 20 00 40 00 02 03 00 F6 C9 F5 88 7A 84 01 03"};
const hci_test_cmd_t HCI_LE_Set_Advertising_Parameters_scanind_ownaddr3_pol3=
{
    .message = "HCI_LE_Set_Advertising_Parameters_scanind_ownaddr3_pol3",
    .cmd_str = Send_HCI_LE_Set_Advertising_Parameters_scanind_ownaddr3_pol3,
};

///dev_name:"br17-4.2"
const u8 Send_HCI_LE_Set_Adv_rsp_data_datalen0[] =
{"01 09 20 20 00 1E FF 57 01 00 7C 5B 5D 17 0E 02 BD D7 0C BB 8A CC C3 3C F2 86 0F 00 00 00 88 0F 10 53 49 56"};
const hci_test_cmd_t HCI_LE_Set_Adv_rsp_data_datalen0=
{
    .message = "HCI_LE_Set_Adv_rsp_data_datalen0",
    .cmd_str = Send_HCI_LE_Set_Adv_rsp_data_datalen0,
};

const int tp_sec_adv_bv04c[] =
{
    (int)&HCI_LE_Set_Random_Address_nonresolvable_private,
    (int)&HCI_LE_Add_Device_To_Resolving_List_local_irk_allzero,
    (int)&HCI_LE_Add_Device_To_White_List,
    (int)&HCI_LE_Set_Advertising_Parameters_scanind_ownaddr3_pol3,
    (int)&HCI_LE_Set_Advertising_Data_datalen0,
    (int)&HCI_LE_Set_Adv_rsp_data_datalen0,
    (int)&HCI_LE_Set_Address_Resolution_Enable_en,
    (int)&HT_Send_HCI_LE_Set_Advertise_Enable,
    (int)9000,
    (int)&HT_Send_HCI_LE_Set_Advertise_Disable,
    (int)200,
};

///interval((min:0x20)),scan IND,own address 2,use channel 37,pol-3
const u8 Send_HCI_LE_Set_Advertising_Parameters_scanind_ownaddr2_pol3[] =
{"01 06 20 0f 20 00 40 00 02 02 00 F6 C9 F5 88 7A 84 01 03"};
const hci_test_cmd_t HCI_LE_Set_Advertising_Parameters_scanind_ownaddr2_pol3=
{
    .message = "HCI_LE_Set_Advertising_Parameters_scanind_ownaddr2_pol3",
    .cmd_str = Send_HCI_LE_Set_Advertising_Parameters_scanind_ownaddr2_pol3,
};

const int tp_sec_adv_bv05c[] =
{
    (int)&HCI_LE_Add_Device_To_Resolving_List_local_irk,
    (int)&HCI_LE_Set_Random_Private_Address_Timeout,
    (int)&HCI_LE_Add_Device_To_White_List,
    (int)&HCI_LE_Set_Advertising_Parameters_scanind_ownaddr2_pol3,
    (int)&HCI_LE_Set_Advertising_Data_datalen0,
    (int)&HCI_LE_Set_Address_Resolution_Enable_en,
    (int)&HT_Send_HCI_LE_Set_Advertise_Enable,
    (int)9000,
    (int)&HT_Send_HCI_LE_Set_Advertise_Disable,
    (int)200,
};

const u8 Send_HCI_LE_Add_Device_To_Resolving_List_unusedIRK_unusedPeeraddr[] =
{"01 27 20 27 00 00 00 00 00 00 00 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11 22 22 22 22 22 22 22 22 22 22 22 22 22 22 22 22"};
const hci_test_cmd_t HCI_LE_Add_Device_To_Resolving_List_unusedIRK_unusedPeeraddr=
{
    .message = "HCI_LE_Add_Device_To_Resolving_List_unusedIRK_unusedPeeraddr",
    .cmd_str = Send_HCI_LE_Add_Device_To_Resolving_List_unusedIRK_unusedPeeraddr,
};

///interval((min:0x20)),own address 3,use channel 37,pol-0
const u8 Send_HCI_LE_Set_Advertising_Parameters_ownaddr3_pol0[] =
{"01 06 20 0f 20 00 40 00 00 03 00 00 00 00 00 00 00 01 00"};
const hci_test_cmd_t HCI_LE_Set_Advertising_Parameters_ownaddr3_pol0=
{
    .message = "HCI_LE_Set_Advertising_Parameters_ownaddr3_pol0",
    .cmd_str = Send_HCI_LE_Set_Advertising_Parameters_ownaddr3_pol0,
};

const int tp_sec_adv_bv06c[] =
{
    (int)&HCI_LE_Add_Device_To_Resolving_List_unusedIRK_unusedPeeraddr,
    (int)&HCI_LE_Set_Address_Resolution_Enable_en,
    (int)&HCI_LE_Set_Random_Address_nonresolvable_private,
    (int)&HCI_LE_Set_Advertising_Parameters_ownaddr3_pol0,
    (int)&HT_Send_HCI_LE_Set_Advertise_Enable,
    (int)9000,
    (int)&HT_Send_HCI_LE_Set_Advertise_Disable,
    (int)200,
};

const u8 Send_HCI_LE_Add_Device_To_Resolving_List_PeerIRKzero[] =
{"01 27 20 27 00 F6 C9 F5 88 7A 84 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 E6 EA EE 60 31 7B FC A2 3F A5 79 59 E7 41 CF C7"};
const hci_test_cmd_t HCI_LE_Add_Device_To_Resolving_List_PeerIRKzero=
{
    .message = "HCI_LE_Add_Device_To_Resolving_List_PeerIRKzero",
    .cmd_str = Send_HCI_LE_Add_Device_To_Resolving_List_PeerIRKzero,
};

///interval((min:0x20)),own address 2,use channel 37,pol-0
const u8 Send_HCI_LE_Set_Advertising_Parameters_ownaddr2_pol0[] =
{"01 06 20 0f 20 00 40 00 00 02 00 00 00 00 00 00 00 01 00"};
const hci_test_cmd_t HCI_LE_Set_Advertising_Parameters_ownaddr2_pol0=
{
    .message = "HCI_LE_Set_Advertising_Parameters_ownaddr2_pol0",
    .cmd_str = Send_HCI_LE_Set_Advertising_Parameters_ownaddr2_pol0,
};

const int tp_sec_adv_bv07c[] =
{
    (int)&HCI_LE_Add_Device_To_Resolving_List_PeerIRKzero,
    (int)&HCI_LE_Set_Random_Private_Address_Timeout,
    (int)&HCI_LE_Set_Random_Address_nonresolvable_private,
    (int)&HCI_LE_Set_Advertising_Parameters_ownaddr2_pol0,
    (int)&HCI_LE_Set_Advertising_Data_datalen0,
    (int)&HCI_LE_Add_Device_To_White_List,
    (int)&HCI_LE_Set_Address_Resolution_Enable_en,
    (int)&HT_Send_HCI_LE_Set_Advertise_Enable,
    (int)9000,
    (int)&HT_Send_HCI_LE_Set_Advertise_Disable,
    (int)200,
};

///interval((min:0x20)),own address 2,use channel 37,pol-3
const u8 Send_HCI_LE_Set_Advertising_Parameters_ownaddr2_pol3[] =
{"01 06 20 0f 20 00 40 00 00 02 00 00 00 00 00 00 00 01 03"};
const hci_test_cmd_t HCI_LE_Set_Advertising_Parameters_ownaddr2_pol3=
{
    .message = "HCI_LE_Set_Advertising_Parameters_ownaddr2_pol3",
    .cmd_str = Send_HCI_LE_Set_Advertising_Parameters_ownaddr2_pol3,
};

const int tp_sec_adv_bv08c[] =
{
    (int)&HCI_LE_Add_Device_To_Resolving_List_local_irk,
    (int)&HCI_LE_Set_Random_Private_Address_Timeout,
    (int)&HCI_LE_Set_Advertising_Parameters_ownaddr2_pol3,
    (int)&HCI_LE_Add_Device_To_White_List,
    (int)&HCI_LE_Set_Advertising_Data_datalen0,
    (int)&HCI_LE_Set_Address_Resolution_Enable_en,
    (int)&HT_Send_HCI_LE_Set_Advertise_Enable,
    (int)9000,
    (int)&HT_Send_HCI_LE_Set_Advertise_Disable,
    (int)200,
};

///interval((min:0x20)),direct IND,own address 2,use channel 37,pol-3
const u8 Send_HCI_LE_Set_Advertising_Parameters_directADV_ownaddr2_pol3[] =
{"01 06 20 0f 20 00 40 00 04 02 00 3E 3A BA 98 11 71 01 03"};
const hci_test_cmd_t HCI_LE_Set_Advertising_Parameters_directADV_ownaddr2_pol3=
{
    .message = "HCI_LE_Set_Advertising_Parameters_directADV_ownaddr2_pol3",
    .cmd_str = Send_HCI_LE_Set_Advertising_Parameters_directADV_ownaddr2_pol3,
};

const int tp_sec_adv_bv11c[] =
{
    (int)&HCI_LE_Add_Device_To_Resolving_List_local_irk,
    (int)&HCI_LE_Set_Random_Private_Address_Timeout,
    (int)&HCI_LE_Set_Advertising_Parameters_directADV_ownaddr2_pol3,
    (int)&HCI_LE_Add_Device_To_White_List,
    (int)&HCI_LE_Set_Advertising_Data_datalen0,
    (int)&HCI_LE_Set_Address_Resolution_Enable_en,
    (int)&HT_Send_HCI_LE_Set_Advertise_Enable,
    (int)9000,
    /* (int)&HT_Send_HCI_LE_Set_Advertise_Disable, */
    (int)200,
};

const u8 Send_HCI_LE_Set_Scan_Parameters_passive_pol0[] =
{"01 0b 20 07 00 20 00 20 00 00 00"};
const hci_test_cmd_t HCI_LE_Set_Scan_Parameters_passive_pol0=
{
    .message = "HCI_LE_Set_Scan_Parameters_passive_pol0",
    .cmd_str = Send_HCI_LE_Set_Scan_Parameters_passive_pol0,
};

const u8 Send_HCI_LE_Set_Scan_Enable_en[] =
{"01 0c 20 02 01 00"};
const hci_test_cmd_t HCI_LE_Set_Scan_Enable_en=
{
    .message = "HCI_LE_Set_Scan_Enable_en",
    .cmd_str = Send_HCI_LE_Set_Scan_Enable_en,
};

const int tp_ddi_scn_bv13c[] =
{
    (int)&HCI_LE_Add_Device_To_Resolving_List_local_irk_allzero,
    (int)&HCI_LE_Set_Random_Private_Address_Timeout,
    (int)&HCI_LE_Set_Address_Resolution_Enable_en,
    (int)&HCI_LE_Set_Scan_Parameters_passive_pol0,
    (int)&HCI_LE_Set_Scan_Enable_en,
    (int)9000,
};

const u8 Send_HCI_LE_Create_Connection_pol0_ownaddr2_peeraddr2[] =
{"01 0d 20 19 20 00 20 00 00 02 3e 3a ba 98 22 71 02 f0 00 f0 00 00 00 20 03 50 00 50 00"};
const hci_test_cmd_t HCI_LE_Create_Connection_pol0_ownaddr2_peeraddr2=
{
    .message = "HCI_LE_Create_Connection_pol0_ownaddr2_peeraddr2",
    .cmd_str = Send_HCI_LE_Create_Connection_pol0_ownaddr2_peeraddr2,
};

const int tp_con_ini_bv09c[] =
{
    (int)&HCI_LE_Add_Device_To_Resolving_List_local_irk,
    (int)&HCI_LE_Set_Random_Private_Address_Timeout,
    (int)&HCI_LE_Set_Address_Resolution_Enable_en,
    (int)&HCI_LE_Create_Connection_pol0_ownaddr2_peeraddr2,
    (int)9000,
};

/* .peer_identity_address_type = 0, */
/* .peer_identity_address = {0x46, 0x01, 0x70, 0xAC, 0xF5, 0xBC},  //LSB->MSB */
/* .peer_irk = {0x74, 0x01, 0x90, 0x4A, 0xF7, 0x35, 0xA4, 0x38, 0xD3, 0x20, 0x9A, 0xCD, 0x48, 0xC0, 0x54, 0x40}, */
/* peer_addr = 3E 3A BA 98 22 71 */
const u8 Send_HCI_LE_Add_Device_To_Resolving_List_local_irk_master[] =
{"01 27 20 27 00 3E 3A BA 98 22 71 74 01 90 4A F7 35 A4 38 D3 20 9A CD 48 C0 54 40 E6 EA EE 60 31 7B FC A2 3F A5 79 59 E7 41 CF C7"};
const hci_test_cmd_t HCI_LE_Add_Device_To_Resolving_List_local_irk_master=
{
    .message = "HCI_LE_Add_Device_To_Resolving_List_local_irk_master",
    .cmd_str = Send_HCI_LE_Add_Device_To_Resolving_List_local_irk_master,
};

const int tp_con_ini_bv12c[] =
{
    (int)&HCI_LE_Add_Device_To_Resolving_List_local_irk_master,
    (int)&HCI_LE_Set_Random_Private_Address_Timeout,
    (int)&HCI_LE_Set_Address_Resolution_Enable_en,
    (int)&HCI_LE_Create_Connection_pol0_ownaddr2_peeraddr2,
    (int)9000,
};
/* const int tp_con_ini_bv12c[] = */
/* { */
/*     (int)&HCI_LE_Add_Device_To_Resolving_List_local_irk_master, */
/*     (int)&HCI_LE_Set_Random_Private_Address_Timeout, */
/*     (int)&HCI_LE_Set_Address_Resolution_Enable_en, */
/*     (int)&HCI_LE_Create_Connection_pol0_ownaddr2_peeraddr2, */
/*     (int)9000, */
/* }; */

const test_flow_struct_t test_flow_linklayer_privacy[] =
{
    {///
        .message = "tp_normal_adv",
        .table_addr = tp_normal_adv,
        .cmd_cnt = sizeof(tp_normal_adv)/sizeof(int),
    },
    {///
        .message = "tp_sec_adv_bv02c",
        .table_addr = tp_sec_adv_bv02c,
        .cmd_cnt = sizeof(tp_sec_adv_bv02c)/sizeof(int),
    },
    {///
        .message = "tp_sec_adv_bv03c",
        .table_addr = tp_sec_adv_bv03c,
        .cmd_cnt = sizeof(tp_sec_adv_bv03c)/sizeof(int),
    },
    {///
        .message = "tp_sec_adv_bv04c",
        .table_addr = tp_sec_adv_bv04c,
        .cmd_cnt = sizeof(tp_sec_adv_bv04c)/sizeof(int),
    },
    {///
        .message = "tp_sec_adv_bv05c",
        .table_addr = tp_sec_adv_bv05c,
        .cmd_cnt = sizeof(tp_sec_adv_bv05c)/sizeof(int),
    },
    {///
        .message = "tp_sec_adv_bv06c",
        .table_addr = tp_sec_adv_bv06c,
        .cmd_cnt = sizeof(tp_sec_adv_bv06c)/sizeof(int),
    },
    {///
        .message = "tp_sec_adv_bv07c",
        .table_addr = tp_sec_adv_bv07c,
        .cmd_cnt = sizeof(tp_sec_adv_bv07c)/sizeof(int),
    },
    {///
        .message = "tp_sec_adv_bv08c",
        .table_addr = tp_sec_adv_bv08c,
        .cmd_cnt = sizeof(tp_sec_adv_bv08c)/sizeof(int),
    },
    {///
        .message = "tp_sec_adv_bv11c",
        .table_addr = tp_sec_adv_bv11c,
        .cmd_cnt = sizeof(tp_sec_adv_bv11c)/sizeof(int),
    },
    {///
        .message = "tp_ddi_scn_bv13c",
        .table_addr = tp_ddi_scn_bv13c,
        .cmd_cnt = sizeof(tp_ddi_scn_bv13c)/sizeof(int),
    },
    {///
        .message = "tp_con_ini_bv09c",
        .table_addr = tp_con_ini_bv09c,
        .cmd_cnt = sizeof(tp_con_ini_bv09c)/sizeof(int),
    },
    {///
        .message = "tp_con_ini_bv12c",
        .table_addr = tp_con_ini_bv12c,
        .cmd_cnt = sizeof(tp_con_ini_bv12c)/sizeof(int),
    },
};

///--------------------------------
static void hhy_hci_cmd_send(void *buf)
{
    u16 len;

    len = func_char_to_hex(hhy_hci_buf, (u8 *)buf + 3); //+3的原因是为了兼容敏贤测试数据

    pc_h4_send_hci_cmd(hhy_hci_buf, len); //发送hci命令
}

#define TP_DDI_ADV_STEPS			(sizeof(test_flow_linklayer_adv) / sizeof(test_flow_struct_t))
#define TP_PRIVACY_STEPS			(sizeof(test_flow_linklayer_privacy) / sizeof(test_flow_struct_t))
#define CUR_TP_TYPE_STEPS			TP_PRIVACY_STEPS
#define CUR_TP_TYPE                 test_flow_linklayer_privacy
#define TYPE_DDI			('2')
	#define DDI_ADV				'2'
	#define DDI_SCN				'3'
#define TYPE_CON			('3')
	#define CON_ADV				'2'
	#define CON_INI				'3'
	#define CON_SLA				'4'
	#define CON_MAS				'5'
#define TYPE_TIM			('4')

//<流程处理函数
static void flow_deal_func(char *rx_buf, u8 len, test_flow_struct_t *flow_p, u32 run_all_en, int *p_steps_cnt)
{
	static u32 time_cnt = 0;
	//test_flow_linklayer_adv 流程参数
	int *each_flow_addr;     			//每一个流程执行地址
	u32 each_flow_step_size; 			//每一个流程需要执行的步骤数量
	static u32 each_flow_step_cnt = 0;	//每一个流程执行步进
	//tp_ddi_adv_bv 流程参数
	u32 			flow_time_delay; //hci命令间延时
	hci_test_cmd_t *flow_parm_addr;  //hci命令地址


	each_flow_addr      = flow_p[*p_steps_cnt].table_addr; 		//获取每个流程的执行地址
	each_flow_step_size = flow_p[*p_steps_cnt].cmd_cnt; 		//获取每个流程需要执行的步骤数量

	if( each_flow_addr[each_flow_step_cnt]  < 1000000 )
	{
		flow_time_delay = each_flow_addr[each_flow_step_cnt] /100; 						//延时参数

		if(time_cnt == flow_time_delay) //延时走完
		{
			time_cnt = 0;
		}
		else //延时没走完
		{
			time_cnt++;
			return;
		}
	}
	else
	{
		flow_parm_addr = (hci_test_cmd_t *)(each_flow_addr[each_flow_step_cnt]);		//命令参数地址
		hhy_puts("\n\n");
		hhy_puts("******************************\n");
		hhy_puts(flow_parm_addr->message);			//打印命令名字
		hhy_puts("\n");
		if(flow_parm_addr->cmd_str != NULL)
		{
			hhy_hci_cmd_send(flow_parm_addr->cmd_str); 	//发送命令
		}
		else
		{
			hhy_puts("--CMD NULL\n");
		}
	}

	each_flow_step_cnt++; //走下一条命令
	if(each_flow_step_cnt == each_flow_step_size)
	{
		each_flow_step_cnt = 0;
		hhy_puts("\n------------------------------------------\n");
		hhy_puts(flow_p[*p_steps_cnt].message);
		hhy_puts("\n------------------------------------------\n");
		if(run_all_en)
		{
			(*p_steps_cnt)++; //test_flow_linklayer_adv往下一个流程
			if(*p_steps_cnt == CUR_TP_TYPE_STEPS) //一个测试流程走完
			{
				hhy_puts("CUR_TP_TYPE_STEPS\n");
				*p_steps_cnt = 0;
				goto __clear_rx_buf;
			}
			else
				return;
		}
		else
			goto __clear_rx_buf;
	}
	else
	{
		return;
	}

__clear_rx_buf:
	hhy_puts("\n-------FLOW END-------\n");
	memset(rx_buf, 0, len);
}


/* void Api_tp_ddi_adv(char *rx_buf, u8 len) */
/* { */
	/* flow_deal_func(rx_buf, len, test_flow_linklayer_adv, TRUE, &g_steps_cnt); */
/* } */
/*  */
/* void Api_tp_ddi_scn(char *rx_buf, u8 len) */
/* { */
	/* //pc_h4_send_hci_cmd(); */
/* } */
/*  */
/* void Api_tp_ddi(char *rx_buf, u8 len) */
/* { */
	/* switch(rx_buf[1]) */
	/* { */
	/* case DDI_ADV: */
		/* Api_tp_ddi_adv(rx_buf, len); */
		/* break; */
	/* case DDI_SCN: */
		/* Api_tp_ddi_scn(rx_buf, len); */
		/* break; */
	/* default: */
		/* break; */
	/* } */
/* } */
/*  */
/* void Api_tp_con(char *rx_buf, u8 len) */
/* { */
/*  */
/* } */
/* void Api_tp_tim(char *rx_buf, u8 len) */
/* { */
/*  */
/* } */
/*  */
/*  */
/* void API_ll_ts_4_2_1_hci_test_ALL(char *rx_buf, u8 len) */
/* { */
	/* switch(rx_buf[0]) */
	/* { */
	/* case TYPE_DDI: */
		/* //hhy_puts("TYPE_DDI\n"); */
		/* Api_tp_ddi(rx_buf, len); */
		/* break; */
	/* case TYPE_CON: */
		/* Api_tp_con(rx_buf, len); */
		/* break; */
	/* case TYPE_TIM: */
		/* Api_tp_tim(rx_buf, len); */
		/* break; */
	/* default: */
		/* break; */
	/* } */
/* } */


static void ll_rx_buf_change(char *rx_buf, u8 len, char c)
{
	int cnt_temp = CUR_TP_TYPE_STEPS;

	switch(c)
	{
	case '+':
		g_steps_cnt += 2;
	case '-':
		g_steps_cnt--;
		if(g_steps_cnt == cnt_temp)
		{
			g_steps_cnt = 0;
		}
		else if(g_steps_cnt < 0)
		{
			g_steps_cnt = CUR_TP_TYPE_STEPS - 1;
		}
		hhy_puts("\n");
		hhy_puts("*** cur TP: ");
		hhy_puts(CUR_TP_TYPE[g_steps_cnt].message);
		hhy_puts("\n");
		break;
	case '=':
		memset(rx_buf, '=', len);
		break;
	default:
		break;
	}
}

static void ll_ts_4_2_1_hci_test_SINGLE(char *rx_buf, u8 len)
{
	flow_deal_func(rx_buf, len, CUR_TP_TYPE, FALSE, &g_steps_cnt);
}


void API_uart_debug(void)
{
    static u16 cnt = 0;
    static u8 count = 0;
    char c;

    cnt++;

	printf("\n--func=%s\n", __FUNCTION__);
    //1.change test option
    c = getchar();
    if((c >= '0') && (c <= '9'))
    {
        putchar(c);
        uart_rx_buf[count++] = c;
        if(count == UART_RX_LEN)
        {
            count = 0;
        }
    }
    if((c == '+') || (c == '-') || (c == '='))
    {
        ll_rx_buf_change(uart_rx_buf, UART_RX_LEN, c);
    } 

    //2.single ll ts test
    if(cnt == 250)
    {
        cnt = 0;
        
        int i;
        for(i = 0; i < UART_RX_LEN; i++)
        {
            if(uart_rx_buf[i] == 0)
            {
                return;
            }
        }
        ll_ts_4_2_1_hci_test_SINGLE(uart_rx_buf, UART_RX_LEN);
    }
}
