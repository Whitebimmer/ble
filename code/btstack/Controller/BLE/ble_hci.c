#include "ble/link_layer.h"
#include "thread.h"
#include "stdarg.h"
#include "ble/ble_h4_transport.h"

#define HCI_DEBUG_EN

#ifdef HCI_DEBUG_EN
#define hci_putchar(x)        putchar(x)
#define hci_puts(x)           puts(x)
#define hci_u32hex(x)         put_u32hex(x)
#define hci_u8hex(x)          put_u8hex(x)
#define hci_buf(x,y)          printf_buf(x,y)
#define hci_printf            printf
#else
#define hci_putchar(...)
#define hci_puts(...)
#define hci_u32hex(...)
#define hci_u8hex(...)
#define hci_buf(...)
#define hci_printf(...)
#endif

// OGFs
#define OGF_LINK_CONTROL          0x01
#define OGF_LINK_POLICY           0x02
#define OGF_CONTROLLER_BASEBAND   0x03
#define OGF_INFORMATIONAL_PARAMETERS 0x04
#define OGF_STATUS_PARAMETERS     0x05
#define OGF_LE_CONTROLLER 0x08
#define OGF_BTSTACK 0x3d
#define OGF_VENDOR  0x3f



struct hci_cmd {
	u16 size;
	u8 data[0];
};

struct hci_event{
	u8 event;
	u8 len;
	u8 data[0];
}__attribute__((packed));

struct hci_rx {
	u8 len;
	u8 head[4];
	u8 data[0];
}__attribute__((packed));


struct hci_tx {
    u32 len;
    u8 head[4];
    u8 data[0];
}__attribute__((packed));


struct lbuff_head *hci_cmd_ptr SEC(.btmem_highly_available);
struct lbuff_head *hci_event_buf SEC(.btmem_highly_available);
struct lbuff_head *hci_rx_buf SEC(.btmem_highly_available);

u8 hci_buf[CONTROLLER_MAX_TOTAL_PAYLOAD] __attribute__((aligned(4)));

struct flow_control{
    u8 free_num_hci_command_packets;
    u8 free_num_hci_acl_packets;
};

static struct flow_control ll_flow_control;

//HIC SET/READ LE
static struct le_parameter *le_param_t SEC(.btmem_highly_available);

static struct hci_parameter hci_param SEC(.btmem_highly_available);

#define   ROLE_MASTER   0  /* 0:slave 1:Master */ 


#define STATUS_COMMAND_SUCCEEDED 		0

#define READ_CMD_OGF(buffer) (buffer[1] >> 2)
#define READ_CMD_OCF(buffer) ((buffer[1] & 0x03) << 8 | buffer[0])
#define OPCODE(ogf, ocf) (ocf | (ogf << 10))


static struct thread hci_thread;
/********************************************************************************/
/*
 *                   Event API 
 */
static struct hci_event *__alloc_event(int len, const char *format)
{
	struct hci_event *event;

	len += __vsprintf_len(format);
	event = lbuf_alloc(hci_event_buf, sizeof(*event)+len);
	if (event == NULL){
		ASSERT(event!= NULL, "%d, %s\n", len, format);
		return NULL;
	}
	event->len = len;

	return event;
}


static int le_send_event(int code, const char *format, ...)
{
	struct hci_event *event;

	event = __alloc_event(0, format);
	event->event = code;

	va_list argptr;
	va_start(argptr, format);

	__vsprintf(event->data, format, argptr);
	va_end(argptr);

	lbuf_push(event);

    //hci resume thread
    thread_resume(&hci_thread);

	return 0;
}

static void __hci_emit_event_of_cmd_complete(u16 opcode, const char *format, ...)
{
	struct hci_event *event;

	event = __alloc_event(3, format);
	ASSERT(event != NULL);
	event->event = HCI_EVENT_COMMAND_COMPLETE;
	event->data[0] = ll_flow_control.free_num_hci_command_packets; //Num_HCI_Command_Packets
	event->data[1] = opcode;
	event->data[2] = opcode>>8;

	va_list argptr;
	va_start(argptr, format);
	__vsprintf(event->data+3, format, argptr);
	va_end(argptr);


	lbuf_push(event);

    //hci resume thread
    thread_resume(&hci_thread);

	/* return 0; */
}

static void __hci_emit_event_of_cmd_status(u8 status, u16 opcode)
{
	struct hci_event *event;

	event = lbuf_alloc(hci_event_buf, sizeof(*event)+4);
    ASSERT(event != NULL, "%d\n", opcode);

	event->event = HCI_EVENT_COMMAND_STATUS;
    event->len = 4;
	event->data[0] = status;
    event->data[1] = ll_flow_control.free_num_hci_command_packets;//Num_HCI_Command_Packets
	event->data[2] = opcode;
	event->data[3] = opcode>>8;

	lbuf_push(event);
    
    //hci resume thread
    thread_resume(&hci_thread);
}


/********************************************************************************/


void le_hci_push_control_data(u8 opcode, const u8 *data)
{
    le_ll_push_control_data(opcode, data);
}
/********************************************************************************/
/*
 *                  Link layer handler 
 */
#define ERR_PIN_OR_KEY_MISSING     0x06

static void param_info_debug(struct le_link *link)
{
    printf("\nconnect param aa : %02x %02x %02x %02x\n", link->conn.ll_data.aa[0], link->conn.ll_data.aa[1], link->conn.ll_data.aa[2], link->conn.ll_data.aa[3]);
    printf("connect param crcint : %02x %02x %02x\n", link->conn.ll_data.crcinit[0], link->conn.ll_data.crcinit[1], link->conn.ll_data.crcinit[2]);
    printf("connect param peer_addr_type : %02x\n", link->peer.addr_type);
    puts("connect peer addr : ");printf_buf(link->peer.addr);
    printf("connect param winsize : %02x\n", link->conn.ll_data.winsize);
    printf("connect param winoffset : %04x\n", link->conn.ll_data.winoffset);
    printf("connect param interval : %04x\n", link->conn.ll_data.interval);
    printf("connect param latency : %04x\n", link->conn.ll_data.latency);
    printf("connect param timeout : %04x\n", link->conn.ll_data.timeout);
    printf("connect param windening : %04x\n", link->conn.ll_data.widening);
    printf("connect param channel : %02x %02x %02x %02x %02x\n", link->conn.ll_data.channel[0], link->conn.ll_data.channel[1], link->conn.ll_data.channel[2], link->conn.ll_data.channel[3], link->conn.ll_data.channel[4]);
    printf("connect param hop : %02x\n", link->conn.ll_data.hop);
}

#define LE_FEATURES_IS_SUPPORT(x)      (le_param_t->priv->features[0] & x)

static void rx_adv_pdu_handler(struct le_link *link, struct ble_rx *rx)
{
}


static void rx_scan_pdu_handler(struct le_link *link,struct ble_rx *rx)
{
    //
}

static void rx_init_pdu_handler(struct le_link *link,struct ble_rx *rx)
{

}

static void rx_pdu_handler(struct le_link *link, struct ble_rx *rx)
{
    switch (link->state)
    {
        case LL_ADVERTISING:
            rx_adv_pdu_handler(link, rx);
            break;
        case LL_SCANNING:
            rx_scan_pdu_handler(link, rx);
            break;
        case LL_INITIATING:
            rx_init_pdu_handler(link, rx);
            break;
        case LL_CONNECTION_CREATE:
            break;
    }
}


static void rx_data_pdu_handler(struct le_link *link, struct ble_rx *rx)
{
	if (rx->len == 0)
    {
        return;
    }

    struct hci_rx *packet;

    packet = lbuf_alloc(hci_rx_buf, sizeof(*packet)+rx->len);
    ASSERT(packet != NULL, "%s\n", "hci rx alloc err\n");

    packet->len = rx->len;

    packet->head[0] = link->handle;

    //Packet_Boundary_Flag
    if (rx->llid == LL_DATA_PDU_CONTINUE){
        packet->head[1] = (link->handle>>8) | 0x10;
    } else if(rx->llid == LL_DATA_PDU_START){
        packet->head[1] = (link->handle>>8) | 0x20;
    }

    packet->head[2] = rx->len;
    packet->head[3] = rx->len>>8;

    memcpy(packet->data, rx->data, rx->len);
    lbuf_push(packet);

    //hci resume thread
    thread_resume(&hci_thread);
}

enum
{
    ENCRYPTION_OFF = 0,
    ENCRYPTION_ON,
    ENCRYPTION_BREDR_E0_LE_AES_CCM,
    ENCRYPTION_ALL_AES_CCM,
};

static void __hci_event_encryption_change(struct le_link *link, const u8 *data)
{
    if (!hci_param.secure_conn_host_support)
    {
        if (link->encrypt_enable)
        {
            //TO*DO error code
            hci_puts("HCI_EVENT_ENCRYPTION_CHANGE ENCRYPTION_ON\n");
            le_send_event(HCI_EVENT_ENCRYPTION_CHANGE, 
                    "1H1", 0, link->handle, ENCRYPTION_ON);
        }
        else{
            //TO*DO error code
            hci_puts("HCI_EVENT_ENCRYPTION_CHANGE ENCRYPTION_OFF\n");
            le_send_event(HCI_EVENT_ENCRYPTION_CHANGE, 
                    "1H1", 0, link->handle, ENCRYPTION_OFF);
        }
    }
}

static void __hci_event_encryption_refresh_complete(struct le_link *link)
{
    hci_puts("HCI_EVENT_ENCRYPTION_KEY_REFRESH_COMPLETE\n");
    le_send_event(HCI_EVENT_ENCRYPTION_KEY_REFRESH_COMPLETE, "1H",
                0, link->handle);
}

static void __hci_event_disconnection_complete(struct le_link *link, u8 reason, u8 status)
{
    hci_puts("HCI_EVENT_DISCONNECTION_COMPLETE\n");
    le_send_event(HCI_EVENT_DISCONNECTION_COMPLETE, "1H1",
            status, link->handle, reason);
}


#define HCI_EVENT_READ_REMOTE_VERSION_INFORMATION_COMPLETE  0x0C

static void __hci_event_read_remote_version_information_complete(struct le_link *link, u8 *data, u8 status)
{
    hci_puts("HCI_EVENT_READ_REMOTE_VERSION_INFORMATION_COMPLETE\n");
    le_send_event(HCI_EVENT_READ_REMOTE_VERSION_INFORMATION_COMPLETE,
            "1H122",
            status, link->handle, 
            data[0], READ_BT_16(data, 1), READ_BT_16(data, 3));
}

static void master_rx_ctrl_pdu_handler(struct le_link *link, struct ble_rx *rx)
{
    //basic pdu
	u8 opcode = rx->data[0];
	u8 *data = &rx->data[1];

	switch(opcode)
	{
		case LL_CONNECTION_UPDATE_REQ:
            ASSERT(0, "%s\n", "M LL_CONNECTION_UPDATE_REQ");
			break;
        case LL_CHANNEL_MAP_REQ:
            ASSERT(0, "%s\n", "M LL_CHANNEL_MAP_REQ");
            break;
		case LL_TERMINATE_IND:
			break;
        
        case LL_ENC_REQ:
            ASSERT(0, "%s\n", "M LL_ENC_REQ");
            break;
		case LL_ENC_RSP:
			break;
		case LL_START_ENC_REQ:
			break;
        case LL_START_ENC_RSP:
            break;

        case LL_UNKNOWN_RSP:
            break;

		case LL_FEATURE_REQ:
            ASSERT(0, "%s\n", "M LL_FEATURE_REQ");
			break;
		case LL_FEATURE_RSP:
			break;

        case LL_PAUSE_ENC_REQ:
            ASSERT(0, "%s\n", "M LL_PAUSE_ENC_REQ");
            break;
		case LL_PAUSE_ENC_RSP:
			break;

		case LL_VERSION_IND:
			break;
        case LL_REJECT_IND:
            break;

        case LL_SLAVE_FEATURE_REQ:
            break;

		case LL_CONNECTION_PARAM_REQ: //4.2 new feature
			break;
		case LL_CONNECTION_PARAM_RSP: //4.2 new feature
			break;
        case LL_REJECT_IND_EXT:
            break;

        case LL_PING_REQ:
            break;
        case LL_PING_RSP:
            break;
        
        case LL_LENGTH_REQ:
            break;
        case LL_LENGTH_RSP:
            break;

		default:
			printf("unkonw-opcode: %x\n", opcode);
			printf_buf(rx->data, rx->len);
			break;
	}
}

static void slave_rx_ctrl_pdu_handler(struct le_link *link, struct ble_rx *rx)
{
	u8 opcode = rx->data[0];
	u8 *data = &rx->data[1];

	switch(opcode)
	{
		case LL_CONNECTION_UPDATE_REQ:
			break;
		case LL_CHANNEL_MAP_REQ:
			break;
		case LL_TERMINATE_IND:
			break;

		case LL_ENC_REQ:
			break;
		case LL_ENC_RSP:
            ASSERT(0, "%s\n", "S LL_ENC_RSP");
			break;
		case LL_START_ENC_REQ:
            ASSERT(0, "%s\n", "S LL_START_ENC_REQ");
            break;
		case LL_START_ENC_RSP:
			break;

		case LL_UNKNOWN_RSP:
			break;

		case LL_FEATURE_REQ:
			break;
		case LL_FEATURE_RSP:
			break;

		case LL_PAUSE_ENC_REQ:
			break;
		case LL_PAUSE_ENC_RSP:
			break;


		case LL_VERSION_IND:
			break;
		case LL_REJECT_IND:
            ASSERT(0, "%s\n", "S LL_REJECT_IND");
			break;

        case LL_SLAVE_FEATURE_REQ:
            ASSERT(0, "%s\n", "S LL_SLAVE_FEATURE_REQ");
            break;

		case LL_CONNECTION_PARAM_REQ: //4.2 new feature
			break;
		case LL_CONNECTION_PARAM_RSP: //4.2 new feature
            ASSERT(0, "%s\n", "S LL_SLAVE_FEATURE_REQ");
			break;
        case LL_REJECT_IND_EXT:
            break;

        case LL_PING_REQ:
            break;
        case LL_PING_RSP:
            break;
        
        case LL_LENGTH_REQ:
            break;
        case LL_LENGTH_RSP:
            break;

		default:
			printf("unkonw-opcode: %x\n", opcode);
			printf_buf(rx->data, rx->len);
			break;
	}
}
#define ROLE_IS_SLAVE(link)         (link->role)

static void rx_ctrl_pdu_handler(struct le_link *link, struct ble_rx *rx)
{
    if (ROLE_IS_SLAVE(link))
    {
        slave_rx_ctrl_pdu_handler(link, rx);
    }
    else{
        master_rx_ctrl_pdu_handler(link, rx);
    }
}

//call back func
static void le_hci_rx_handler(struct le_link *link, struct ble_rx *rx)
{
    /* puts("--LL layer PH\n"); */
    switch(rx->llid)
    {
        case LL_RESERVED:
            /* rx_pdu_handler(link, rx); */
            break;
        case LL_DATA_PDU_START:
        case LL_DATA_PDU_CONTINUE:
            rx_data_pdu_handler(link, rx);
            break;
        case LL_CONTROL_PDU:
            /* rx_ctrl_pdu_handler(link, rx); */
            break;
        default:
            break;
    }
}
/* REGISTER_LLP_ACL_RXCHANNEL(le_hci_rx_handler); */

#define HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS              0x13

/* static void le_hci_probe_tx_handler(u8 opcode, struct le_link *link) */
/* { */
    /* switch (opcode) */
/* } */


static void tx_adv_pdu_handler(struct le_link *link,struct ble_tx *tx)
{

}

static void tx_scan_pdu_handler(struct le_link *link,struct ble_tx *tx)
{

}

static void tx_init_pdu_handler(struct le_link *link,struct ble_tx *tx)
{

}

static void tx_pdu_handler(struct le_link *link,struct ble_tx *tx)
{
    switch (link->state)
    {
        case LL_ADVERTISING:
            tx_adv_pdu_handler(link, tx);
            break;
        case LL_SCANNING:
            tx_scan_pdu_handler(link, tx);
            break;
        case LL_INITIATING:
            tx_init_pdu_handler(link, tx);
            break;
    }
}


static void slave_tx_ctrl_pdu_handler(struct le_link *link,struct ble_tx *tx)
{

}

static void master_tx_ctrl_pdu_handler(struct le_link *link,struct ble_tx *tx)
{
    
}

static void tx_ctrl_pdu_handler(struct le_link *link,struct ble_tx *tx)
{
    if (ROLE_IS_SLAVE(link))
    {
        slave_tx_ctrl_pdu_handler(link, tx);
    }
    else
    {
        master_tx_ctrl_pdu_handler(link, tx);
    }
}



static void le_hci_tx_handler(struct le_link *link, struct ble_tx *tx)
{
    switch(tx->llid)
    {
        case LL_RESERVED:
            tx_pdu_handler(link, tx);
            break;
        case LL_DATA_PDU_START:
        case LL_DATA_PDU_CONTINUE:
            //LL Ack
            if (ll_packet_is_send())   
            {
                ll_flow_control.free_num_hci_acl_packets++;
                le_send_event(HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS, "1H2", 1,
                        link->handle, 1);
            }
            break;
        case LL_CONTROL_PDU:
            /* tx_ctrl_pdu_handler(link, tx); */
            break;
    }
}

static void le_hci_event_handler(u8 procedure, struct le_link *link, struct ble_rx *rx, u8 status)
{
    //basic pdu
	u8 opcode = rx->data[0];
	u8 *data = &rx->data[1];

    switch(procedure)
    {
        case SLAVE_CONNECTION_PARAMETER_REQUEST_STEPS:
            break;
        case MASTER_CONNECTION_PARAMETER_REQUEST_STEPS:
            break;
        case MASTER_CONNECTION_UPDATE_STEPS:
            break;
        case SLAVE_CONNECTION_PARAMETER_RESPOND_STEPS:
            break;
        case SLAVE_REJECT_EXT_STEPS:
            break;
        case MASTER_CHANNEL_MAP_REQUEST_STEPS:
            break;
        case MASTER_FEATURES_EXCHANGE_STEPS:
            break;
        case SLAVE_FEATURES_EXCHANGE_STEPS:
            break;
        case VERSION_IND_STEPS:
            __hci_event_read_remote_version_information_complete(link, data, status);
            break;
        case START_ENCRYPTION_STEPS:
            __hci_event_encryption_change(link, data);
            break;
        case RESTART_ENCRYPTION_STEPS:
            __hci_event_encryption_refresh_complete(link);
            break;
        case START_ENCRYPTION_REQ_STEPS:
            __hci_event_encryption_change(link, data);
            break;
        case RESTART_ENCRYPTION_REQ_STEPS:
            __hci_event_encryption_refresh_complete(link);
            break;
        case SLAVE_REJECT_STEPS:
            break;
        case DISCONNECT_STEPS:
            __hci_event_disconnection_complete(link, data[0], status);
            break;
        case DATA_LENGTH_UPDATE_STEPS:
            break;
    }
}

//Bottom upload from Linklayer API
static const struct lc_handler le_hci_handler={
    .rx_handler     = le_hci_rx_handler,
    .tx_handler     = le_hci_tx_handler,
    .event_handler  = le_hci_event_handler,
};

/********************************************************************************/
/*
 *                  Link Control Command
 */

void link_control_cmd_handler(u16 opcode, u8 *data, int len)
{
	int ocf = opcode&0x3ff;

	switch(ocf)
	{
        case HCI_DISCONNECT:       
			hci_puts("HCI_DISCONNECT\n");
            le_hci_push_control_data(opcode, data);
            __hci_emit_event_of_cmd_status(0, opcode);
            break;
        default:
            puts("link_control_cmd_handler cmd not finish\n");
            break;
    }
}


/********************************************************************************/
/*
 *                  Baseband Command
 */

#define HCI_RESET                                   0x03
#define HCI_SET_EVENT_MASK                          0x01
#define WRITE_LE_HOST_SUPPORT                       0x6d
#define HCI_WRITE_SECURE_CONNECTIONS_HOST_SUPPORT   0x7A

void ctrl_baseband_cmd_handler(u16 opcode, u8 *data, int len)
{
	int ocf = opcode&0x3ff;

	const struct ble_operate *ops;

	switch(ocf)
	{
		case HCI_RESET:
			hci_puts("HCI_RESET\n");
			le_param_t = (struct le_parameter *)__ll_api->init(&hci_param);
			__ll_api->handler_register(&le_hci_handler);
			__hci_emit_event_of_cmd_complete(opcode, "1", 0);	
		   break;
		case WRITE_LE_HOST_SUPPORT:
			hci_puts("WRITE_LE_HOST_SUPPORT\n");
			__hci_emit_event_of_cmd_complete(opcode, "1", 0);	
		   break;
        case HCI_SET_EVENT_MASK:
			hci_puts("HCI_SET_EVENT_MASK\n");
            memcpy(hci_param.event_mask, data, 8);
            __hci_emit_event_of_cmd_complete(opcode, "1", 0);	
            break;
        case HCI_WRITE_SECURE_CONNECTIONS_HOST_SUPPORT:
			hci_puts("HCI_WRITE_SECURE_CONNECTIONS_HOST_SUPPORT\n");
            hci_param.secure_conn_host_support = data[0];
            break;
        default:
            puts("ctrl_baseband_cmd_handler cmd not finish\n");
            break;
	}
}

/********************************************************************************/

/********************************************************************************/
/*
 *                  HIC Informational Command
 */

#define SLOT3_PACKETS               BIT(0)
#define SLOT5_PACKETS               BIT(1)
#define ENCRYPTION                  BIT(2)
#define SLOT_OFFSET                 BIT(3)
#define TIMING_ACCURACY             BIT(4)
#define ROLE_SWITCH                 BIT(5)
#define HOLD_MODE                   BIT(6)
#define SNIFF_MODE                  BIT(7)

#define PARK_STATE                  BIT(8 + 0)
#define POWER_CONTROL_REQUESTS      BIT(8 + 1)
#define CQDDR                       BIT(8 + 2)
#define SCO_LINK                    BIT(8 + 3)
#define HV2_PACKETS                 BIT(8 + 4)
#define HV3_PACKETS                 BIT(8 + 5)
#define u_LAW_LOG_SYNC_DATA         BIT(8 + 6)
#define A_LAW_LOG_SYNV_DATA         BIT(8 + 7)



static const struct hci_read_parameter hci_read_param = {
    .features = {HOLD_MODE|SNIFF_MODE|POWER_CONTROL_REQUESTS},
#ifdef BR17
    .public_addr = {0x2e, 0x3a, 0xba, 0x98, 0x36, 0x54},
#endif
#ifdef BR16
    /* .public_addr = {0x3e, 0x3a, 0xba, 0x98, 0x36, 0x54}, */
    #ifdef ROLE_MASTER
    .public_addr = {0x8e, 0x3a, 0xba, 0x98, 0x22, 0x71},
    #endif
    #ifdef ROLE_SLAVE
    .public_addr = {0x9e, 0x3a, 0xba, 0x98, 0x22, 0x71},
    #endif
#endif
    .hci_version = 0x8,     //4.2
    .hci_revision = 0x0000,
    .lmp_pal_version = 0x00,
    .manufacturer_name = 0x0000,
    .lmp_pal_subversion = 0x0000,
};


static void hci_informational_handler(u16 opcode, u8 *data, int len)
{
	int ocf = opcode&0x3ff;

	switch(ocf)
	{
		case HCI_READ_BD_ADDR:
			hci_puts("HCI_READ_BD_ADDR\n");
            __hci_emit_event_of_cmd_complete(opcode, "1A", 0, hci_read_param.public_addr);	
		   break;
        case HCI_READ_LOCAL_VERSION_INFORMATION:
           puts("HCI_READ_LOCAL_VERSION_INFORMATION\n");
            __hci_emit_event_of_cmd_complete(opcode, "112122", 0, hci_read_param.hci_version, hci_read_param.hci_revision, hci_read_param.lmp_pal_version, hci_read_param.manufacturer_name, hci_read_param.lmp_pal_subversion);
           break;
        /* case HCI_READ_LOCAL_SUPPORTED_COMMAND: */
            /* puts("HCI_READ_LOCAL_SUPPORTED_COMMAND\n"); */
            /* break; */
        case HCI_READ_LOCAL_SUPPORTED_FEATURES:
            puts("HCI_READ_LOCAL_SUPPORT_FEATURES\n");
            __hci_emit_event_of_cmd_complete(opcode, "1c08", 0, hci_read_param.features);	
           break; 
        case HCI_READ_REMOTE_VERSION_INFORMATION:
           break;

        default:
            puts("hci_informational_handler cmd not finish\n");
            break;
	}

}
/********************************************************************************/

/********************************************************************************/
/*
 *                  LE Controller Command
 */



static void debug_adv_info(void)
{
    printf("Interval min : %x\n",   le_param_t->adv_param.Advertising_Interval_Min );
    printf("Interval min : %x\n",   le_param_t->adv_param.Advertising_Interval_Max );
    printf("type : %x\n",           le_param_t->adv_param.Advertising_Type         );
    printf("own type: %x\n",        le_param_t->adv_param.Own_Address_Type         );
    printf("peer type: %x\n",       le_param_t->adv_param.Peer_Address_Type        );
    printf("peer addr: %s\n",       le_param_t->adv_param.Peer_Address             );
    printf("channel map: %x\n",     le_param_t->adv_param.Advertising_Channel_Map  );
    printf("filter policy: %x\n",   le_param_t->adv_param.Advertising_Filter_Policy);
}
/*************************************************************************************************/
/*
 *                      LE Controller Commands 
 */

#define READ_CONNECTION_HANDLE(buffer)  (READ_BT_16(buffer,0) & 0x0fff)

static void le_hci_command_handler(u16 opcode, u8 *data, int size)
{
	int ocf = opcode & 0x3ff;

	switch(ocf)
	{
		case HCI_LE_SET_EVENT_MASK:
			hci_puts("HCI_LE_SET_EVENT_MASK\n");
            memcpy(le_param_t->event_mask, data, 8);
			__hci_emit_event_of_cmd_complete(opcode, "1", 0);
			break;
		case HCI_LE_READ_BUFFER_SIZE:
			hci_puts("HCI_LE_READ_BUFFER_SIZE\n");
			__hci_emit_event_of_cmd_complete(opcode, "121", 0, le_param_t->priv->buf_param.buffer_size, \
                    le_param_t->priv->buf_param.total_num_le_data_pkt);
			break;
		case HCI_LE_READ_LOCAL_SUPPORT_FEATURES:
			hci_puts("HCI_LE_READ_LOCAL_SUPPORT_FEATURES\n");
			__hci_emit_event_of_cmd_complete(opcode, "1c08", 0, le_param_t->priv->features);
			break;
		case HCI_LE_SET_RANDOM_ADDRESS:
			hci_puts("HCI_LE_SET_RANDOM_ADDRESS\n");
            memcpy(le_param_t->random_address, data, 6);
			__hci_emit_event_of_cmd_complete(opcode, "1", 0);
			break;
		case HCI_LE_SET_ADVERTISING_PARAMETERS:
			hci_puts("HCI_LE_SET_ADVERTISING_PARAMETERS\n");
            memcpy((u8 *)&le_param_t->adv_param, data, sizeof(struct advertising_param));
            /* debug_adv_info(); */
			__hci_emit_event_of_cmd_complete(opcode, "1", 0);
			break;
		case HCI_LE_READ_ADVERTISING_CHANNEL_TX_POWER:
			hci_puts("HCI_LE_READ_ADVERTISING_CHANNEL_TX_POWER\n");
			__hci_emit_event_of_cmd_complete(opcode, "11", 0, le_param_t->priv->transmit_power_level);
			break;
		case HCI_LE_SET_ADVERTISING_DATA:
			hci_puts("HCI_LE_SET_ADVERTISING_DATA\n");
            memset((u8 *)&le_param_t->adv_data, 0x0, sizeof(struct advertising_data));
            le_param_t->adv_data.length = data[0];
            memcpy((u8 *)&le_param_t->adv_data.data, &data[1], data[0]);
			__hci_emit_event_of_cmd_complete(opcode, "1", 0);
			break;
		case HCI_LE_SET_SCAN_RESPONSE_DATA:
			hci_puts("HCI_LE_SET_SCAN_RESPONSE_DATA\n");
            memset((u8 *)&le_param_t->scan_resp_data, 0x0, sizeof(struct scan_response));
            le_param_t->scan_resp_data.length = data[0];
            memcpy((u8 *)&le_param_t->scan_resp_data.data, &data[1], data[0]);
			__hci_emit_event_of_cmd_complete(opcode, "1", 0);
			break;
		case HCI_LE_SET_ADVERTISE_ENABLE:
			if (data[0]){
                hci_puts("ll adversting enable\n");
				__ll_api->open(LL_ADVERTISING);
			} else {
                hci_puts("ll adversting disable\n");
                __ll_api->close(LL_ADVERTISING);
			}
			__hci_emit_event_of_cmd_complete(opcode, "1", 0);
			break;
		case HCI_LE_SET_SCAN_PARAMETERS:
			hci_puts("HCI_LE_SET_SCAN_PARAMETERS\n");
            memcpy((u8 *)&le_param_t->scan_param, data, sizeof(struct scan_parameter));
			__hci_emit_event_of_cmd_complete(opcode, "1", 0);
			break;
		case HCI_LE_SET_SCAN_ENABLE:
            memcpy((u8 *)&le_param_t->scan_enable_param, data, sizeof(struct scan_enable_parameter));
			__hci_emit_event_of_cmd_complete(opcode, "1", 0);
			if (data[0]){
				puts("ll scanning enable\n");
				__ll_api->open(LL_SCANNING);
			} else {
				puts("ll scanning disable\n");
				__ll_api->close(LL_SCANNING);
			}
			break;
		case HCI_LE_CREATE_CONNECTION:
			hci_puts("HCI_LE_CREATE_CONNECTION\n");
            memcpy((u8 *)&le_param_t->conn_param, data, sizeof(struct connection_parameter));
            /* printf_buf((u8 *)&le_param_t->conn_param, sizeof(struct connection_parameter)); */
            __ll_api->open(LL_INITIATING);
            __hci_emit_event_of_cmd_status(0, opcode);
			break;
		case HCI_LE_CREATE_CONNECTION_CANCEL:
			hci_puts("HCI_LE_CREATE_CONNECTION_CANCEL\n");
            __ll_api->close(LL_INITIATING);
			__hci_emit_event_of_cmd_complete(opcode, "1", 0);
			break;
		case HCI_LE_READ_WHIRTE_LIST_SIZE:
			__hci_emit_event_of_cmd_complete(opcode, "11", 0, 
                    le_param_t->priv->white_list_size);
			break;
		case HCI_LE_CLEAR_WHIRTE_LIST:
            __ll_api->white_list_init();
			__hci_emit_event_of_cmd_complete(opcode, "1", 0);
			break;
		case HCI_LE_ADD_DEVICE_TO_WHIRTE_LIST:
            __ll_api->white_list_add(data);
			__hci_emit_event_of_cmd_complete(opcode, "1", 0);
			break;
		case HCI_LE_REMOVE_DEVICE_FROM_WHIRTE_LIST:
            __ll_api->white_list_remove(data);
			__hci_emit_event_of_cmd_complete(opcode, "1", 0);
			break;
		case HCI_LE_CONNECTION_UPDATE:
            le_hci_push_control_data(opcode, data);
            __hci_emit_event_of_cmd_status(0, opcode);
			break;
		case HCI_LE_SET_HOST_CHANNEL_CLASSIFICATION:
            le_hci_push_control_data(opcode, data);
			__hci_emit_event_of_cmd_complete(opcode, "1", 0);
			break;
		case HCI_LE_READ_CHANNEL_MAP:
            {
                u8 channel_map[5];

                ll_read_channel_map(channel_map, data);
				__hci_emit_event_of_cmd_complete(opcode, "1Hc05", 0,
                        READ_CONNECTION_HANDLE(data), channel_map);
            }
			break;
		case HCI_LE_READ_REMOTE_USED_FEATURES:
            le_hci_push_control_data(opcode, data);
            __hci_emit_event_of_cmd_status(0, opcode);
			break;
		case HCI_LE_ENCRYPT:
			hci_puts("HCI_LE_ENCRYPT\n");
			{
				u8 enc[16];
				//u8 xx[16];
				//u8 yy[16];
				//u8 zz[16];
//				swap128(data, xx);
//				swap128(data+16, yy);
				aes128_start(data, data+16, enc);
				__hci_emit_event_of_cmd_complete(opcode, "1c16", 0, enc);
			}
			break;
		case HCI_LE_RAND:
			hci_puts("HCI_LE_RAND\n");
			{
				u8 rand[8];
				__hci_emit_event_of_cmd_complete(opcode, "1c08", 0, rand);
			}
			break;
		case HCI_LE_START_ENCRYPT:
			hci_puts("HCI_LE_START_ENCRYPT\n");
            le_hci_push_control_data(opcode, data);
            __hci_emit_event_of_cmd_status(0, opcode);
			break;
		case HCI_LE_LONG_TERM_KEY_REQUEST_REPLY:
			hci_puts("HCI_LE_LONG_TERM_KEY_REQUEST_REPLY\n");
            /* printf_buf(data, size); */
            printf_buf(data, sizeof (struct long_term_key_request_reply_parameter));
            le_hci_push_control_data(opcode, data);
            __hci_emit_event_of_cmd_complete(opcode, "12", 0,
                    READ_CONNECTION_HANDLE(data));
			break;
		case HCI_LE_LONG_TERM_KEY_REQUEST_NAGATIVE_REPLY:
			hci_puts("HCI_LE_LONG_TERM_KEY_REQUEST_NAGATIVE_REPLY\n");
            le_hci_push_control_data(opcode, data);
            __hci_emit_event_of_cmd_complete(opcode, "12", 0,
                    READ_CONNECTION_HANDLE(data));
			break;
		case HCI_LE_READ_SUPPORTED_STATES:
			break;
		case HCI_LE_RECEIVER_TEST:
			/*le_receive_test(packet);*/
			__hci_emit_event_of_cmd_complete(opcode, "1", 0);
			break;
		case HCI_LE_TRANSMITTER_TEST:
			/*le_transmitter_test(packet);*/
			__hci_emit_event_of_cmd_complete(opcode, "1", 0);
			break;
		case HCI_LE_TEST_END:
			/*__hci_emit_event_of_cmd_complete(opcode, "12", 0, le_test_end());*/
			break;

        case HCI_LE_REMOTE_CONNECTION_PARAMETER_REQUEST_REPLY:
            le_hci_push_control_data(opcode, data);
            __hci_emit_event_of_cmd_complete(opcode, "12", 0,
                    READ_CONNECTION_HANDLE(data));
            break;           
        case HCI_LE_REMOTE_CONNECTION_PARAMETER_REQUEST_NEGATIVE_REPLY:
            le_hci_push_control_data(opcode, data);
            __hci_emit_event_of_cmd_complete(opcode, "12", 0,
                    READ_CONNECTION_HANDLE(data));
            break;           

        case HCI_LE_SET_DATA_LENGTH:
			hci_puts("HCI_LE_SET_DATA_LENGTH\n");
            le_hci_push_control_data(opcode, data);
            __hci_emit_event_of_cmd_complete(opcode, "12", 0,
                    READ_CONNECTION_HANDLE(data));
            break;
        case HCI_LE_READ_SUGGESTED_DEFAULT_DATA_LENGTH:
			hci_puts("HCI_LE_READ_SUGGESTED_DEFAULT_DATA_LENGTH\n");
            {
                struct ll_data_pdu_length *priv;

                priv = ll_read_suggested_default_data_length();
                __hci_emit_event_of_cmd_complete(opcode, "122", 0,
                    priv->connInitialMaxTxOctets,
                    priv->connInitialMaxTxTime);
            }
            
            break;
        case HCI_LE_WRITE_SUGGESTED_DEFAULT_DATA_LENGTH:
			hci_puts("HCI_LE_WRITE_SUGGESTED_DEFAULT_DATA_LENGTH\n");
            {
                ll_write_suggested_default_data_length(data);
                __hci_emit_event_of_cmd_complete(opcode, "1", 0);
            }
            break;
            
        case HCI_LE_ADD_DEVICE_TO_RESOLVING_LIST:
			hci_puts("HCI_LE_ADD_DEVICE_TO_RESOLVING_LIST\n");
            __ll_api->resolving_list_add(data);
			__hci_emit_event_of_cmd_complete(opcode, "1", 0);
            break;
        case HCI_LE_REMOVE_DEVICE_FROM_RESOLVING_LIST:
            __ll_api->resolving_list_remove(data);
			__hci_emit_event_of_cmd_complete(opcode, "1", 0);
            break;
        case HCI_LE_CLEAR_RESOLVING_LIST:
            __ll_api->resolving_list_init();
			__hci_emit_event_of_cmd_complete(opcode, "1", 0);
            break;
        case HCI_LE_READ_RESOLVING_LIST_SIZE:
			hci_puts("HCI_LE_READ_RESOLVING_LIST_SIZE\n");
			__hci_emit_event_of_cmd_complete(opcode, "11", 0,
                    le_param_t->priv->resolving_list_size);
            break;
        case HCI_LE_READ_PEER_RESOLVABLE_ADDRESS:
            {
                u8 peer_RPA[6];

                __ll_api->read_peer_RPA(data[0], data + 1, peer_RPA);

                __hci_emit_event_of_cmd_complete(opcode, "1A", 0,
                        peer_RPA);
            } 
            break;
        case HCI_LE_READ_LOCAL_RESOLVABLE_ADDRESS:
            {
                u8 local_RPA[6];

                __ll_api->read_local_RPA(data[0], data + 1, local_RPA);

                __hci_emit_event_of_cmd_complete(opcode, "1A", 0,
                        local_RPA);
            } 
            break;
        case HCI_LE_SET_ADDRESS_RESOLUTION_ENABLE:
			hci_puts("HCI_LE_SET_ADDRESS_RESOLUTION_ENABLE\n");
            le_param_t->resolution_enable = data[0];
            __ll_api->resolution_enable();
			__hci_emit_event_of_cmd_complete(opcode, "1", 0);
            break;
        case HCI_LE_SET_RESOLVABLE_PRIVATE_ADDRESS_TIMEOUT:
			hci_puts("HCI_LE_SET_RESOLVABLE_PRIVATE_ADDRESS_TIMEOUT\n");
            __ll_api->set_RPA_timeout(data);
            __hci_emit_event_of_cmd_complete(opcode, "1", 0);
            break;
        case HCI_LE_READ_MAXIMUM_DATA_LENGTH:
			hci_puts("HCI_LE_READ_MAXIMUM_DATA_LENGTH\n");
            {
                struct ll_data_pdu_length_read_only *ll_data_pdu_length_ro;
                
                ll_data_pdu_length_ro = ll_read_maximum_data_length();

                __hci_emit_event_of_cmd_complete(opcode, "12222", 0,
                        ll_data_pdu_length_ro->supportedMaxTxOctets,
                        ll_data_pdu_length_ro->supportedMaxTxTime,
                        ll_data_pdu_length_ro->supportedMaxRxOctets,
                        ll_data_pdu_length_ro->supportedMaxRxTime);
            }
            break;
            
        default:
            puts("le_hci_command_handler cmd not finish\n");
            break;
	}
}

/*************************************************************************************************/
static int ble_hci_command_process()
{
	struct hci_cmd *cmd;
    int awake = 0;

	cmd = lbuf_pop(hci_cmd_ptr);
	if (!cmd){
		return awake;
	}

    hci_puts("\nRX : CMD ");
    hci_buf(cmd->data, cmd->size);

	u16 opcode = (cmd->data[1]<<8) | cmd->data[0]; 

	int	ogf = opcode>>10;

    //data[0:1] opcode  [2]:size
	switch(ogf){
        case OGF_LINK_CONTROL:
			link_control_cmd_handler(opcode, cmd->data+3, cmd->size-3);
            break;
		case OGF_CONTROLLER_BASEBAND:
			ctrl_baseband_cmd_handler(opcode, cmd->data+3, cmd->size-3);
			break;
		case OGF_INFORMATIONAL_PARAMETERS:
			hci_informational_handler(opcode, cmd->data+3, cmd->size-3);
			break;
		case OGF_LE_CONTROLLER:
			le_hci_command_handler(opcode, cmd->data+3, cmd->size-3);
			break;
	}

	lbuf_free(cmd);
    ll_flow_control.free_num_hci_command_packets++;

    {
        awake = 1;
    }
    return awake;
}




int le_hci_push_command(u8 *packet, int size)
{
	struct hci_cmd *cmd;

	cmd = lbuf_alloc(hci_cmd_ptr, sizeof(*cmd)+size);
	if (!cmd){
		return -1;
	}

	cmd->size = size;
	memcpy(cmd->data, packet, size);

    ll_flow_control.free_num_hci_command_packets--;
	lbuf_push(cmd);

	return 0;
}


#define READ_ACL_CONNECTION_HANDLE(buffer) (READ_BT_16(buffer,0) & 0x0fff)
#define READ_ACL_SIZE(buffer)           READ_BT_16(buffer,2)

int le_hci_push_acl_data(u8 *packet, int len)
{
    int handle;
    u16 size;

    /* hci_puts("\nTX : DATA "); */
    /* hci_buf(tx, sizeof(*tx)+tx->len); */
    handle = READ_ACL_CONNECTION_HANDLE(packet);
    size = READ_ACL_SIZE(packet);
    
    /* printf_buf(packet->data, packet->len - 4); */
    /* printf_buf(packet->data, size); */

    ll_send_acl_packet(handle, packet, len);

    ll_flow_control.free_num_hci_acl_packets--;

	return 0;
}


static void ble_hci_h4_download_data(int packet_type, u8 *packet, int len)
{
	if (packet_type == HCI_COMMAND_DATA_PACKET){
        /* hci_puts("\nTX CMD "); */
        /* hci_buf(packet, len); */
		le_hci_push_command(packet, len);
        thread_resume(&hci_thread);
	} else {
        /* hci_puts("\nTX DATA "); */
        /* hci_buf(packet, len); */
        //direct to RF tx buf
		le_hci_push_acl_data(packet, len);
	}
    //hci resume thread
}
REGISTER_H4_CONTROLLER(ble_hci_h4_download_data);

static int ble_hci_h4_can_send_packet_now(u8 packet_type)
{
    if (packet_type == HCI_ACL_DATA_PACKET){
        
        return 1;
    }else {

        return 1;
    }
}
REGISTER_H4_CONTROLLER_FLOW_CONTROL(ble_hci_h4_can_send_packet_now);




static int ble_hci_h4_upload_data()
{
	struct hci_rx *rx;
	struct hci_event *event;
	struct hci_tx *tx;
    int awake = 0;

	event = lbuf_pop(hci_event_buf);
	if (event){
        puts("\nEMIT : EVENT ");
        printf_buf(event, sizeof(*event)+event->len);
		ble_h4_packet_handler(HCI_EVENT_PACKET, event, sizeof(*event)+event->len);
		lbuf_free(event);
	}

	rx = lbuf_pop(hci_rx_buf);
	if (rx) {
        puts("\nRX : ACL ");
        printf_buf(rx->head, rx->len+sizeof(rx->head));
		ble_h4_packet_handler(HCI_ACL_DATA_PACKET, rx->head, rx->len+sizeof(rx->head));
		lbuf_free(rx);
	}

    return awake;
}

static void hci_thread_process(struct thread *th)
{
    int awake = 0;

	ble_hci_command_process();

	ble_hci_h4_upload_data();

    //wait all channel idle
    if ((lbuf_empty(hci_event_buf))
        && (lbuf_empty(hci_rx_buf))
        && (lbuf_empty(hci_cmd_ptr)))
    {
        //suspend hci thread now
        thread_suspend(th, 0);
    }
}

/* void aes128_test(): */
void aes128_test()
{
	u8 key[]= {0x4C, 0x68, 0x38, 0x41, 0x39, 0xF5, 0x74, 0xD8, 
		0x36, 0xBC, 0xF3, 0x4E, 0x9D, 0xFB, 0x01, 0xBF};
	u8 plaintext[] = {0x02, 0x13, 0x24, 0x35, 0x46, 0x57, 0x68,
	   	0x79, 0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13};
	u8 enc[] = {0x99, 0xad, 0x1b, 0x52, 0x26, 0xa3, 0x7e, 0x3e,
	   	0x05, 0x8e, 0x3b, 0x8e, 0x27, 0xc2, 0xc6, 0x66};
	u8 encrypt[16];
	u8 _encrypt[16];
	u8 _key[16];
	u8 _plaintext[16];

	puts("aes128_test\n");

	swap128(key, _key);
	swap128(plaintext, _plaintext);

	aes128_start(_key, _plaintext, encrypt);
    aes128_start_dec(_key, encrypt, _encrypt);


	swap128(encrypt, _key);
    ASSERT(!memcmp(enc, _key, 16), "%s\n", "AES128 encode err!");

	swap128(_encrypt, _key);
    ASSERT(!memcmp(plaintext, _key, 16), "%s\n", "AES128 decode err!");

	printf_buf(encrypt, 16);
	printf_buf(_encrypt, 16);
	/* printf_buf(encrypt, 16); */

    puts("ah test\n");

    //MSB->LSB
	u8 irk[]= {0xec, 0x02, 0x34, 0xa3 , 0x57, 0xc8, 0xad, 0x05 , 0x34, 0x10, 0x10, 0xa6 , 0x0a, 0x39, 0x7d, 0x9b}; 

    u8 prand[] = {0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00 , 0x00, 0x70, 0x81, 0x94};

    u8 ah[] = {0x15, 0x9d, 0x5f, 0xb7 , 0x2e, 0xbe, 0x23, 0x11 , 0xa4, 0x8c, 0x1b, 0xdc , 0xc4, 0x0d, 0xfb, 0xaa};

	swap128(irk, _key);
	swap128(prand, _plaintext);

	/* aes128_start(irk, prand, encrypt); */
	aes128_start(_key, _plaintext, encrypt);

	swap128(encrypt, _key);
    ASSERT(!memcmp(ah, _key, 16), "%s\n", "ah encode err!");
	/* printf_buf(encrypt, 16); */
}





static void hci_param_init(void)
{
    hci_param.priv = &hci_read_param;

    /* device_addr_generate(hci_param.static_addr, STATIC_DEVICE_ADDR); */
    /* device_addr_verify(hci_param.static_addr, 0, 6); */
    /* puts("STATIC_DEVICE_ADDR : "); */
    /* printf_buf(hci_param.static_addr, 6); */

    /* device_addr_generate(hci_param.non_resolvable_private_addr, NON_RESOVABLE_PRIVATE_ADDR); */
    /* device_addr_verify(hci_param.non_resolvable_private_addr, 0, 6); */
    /* puts("NON_RESOVABLE_PRIVATE_ADDR  : "); */
    /* printf_buf(hci_param.non_resolvable_private_addr, 6); */

    /* device_addr_generate(hci_param.resolvable_private_addr, RESOLVABLE_PRIVATE_ADDR); */
    /* device_addr_verify(hci_param.resolvable_private_addr, 0, 6); */
    /* puts("RESOLVABLE_PRIVATE_ADDR "); */
    /* printf_buf(hci_param.resolvable_private_addr, 6); */
}

#define HCI_BUF_CMD_POS         0
#define HCI_BUF_EVENT_POS       (HCI_BUF_CMD_POS + CONTROLLER_MAX_CMD_PAYLOAD)
#define HCI_BUF_RX_POS          (HCI_BUF_EVENT_POS + CONTROLLER_MAX_EVENT_PAYLOAD)

static void lbuf_debug(void);

int hci_firmware_init()
{
    //host to controller
	hci_cmd_ptr = lbuf_init(hci_buf + HCI_BUF_CMD_POS, CONTROLLER_MAX_CMD_PAYLOAD);
    ll_flow_control.free_num_hci_command_packets = 4;

    //controller to host
	hci_event_buf = lbuf_init(hci_buf + HCI_BUF_EVENT_POS, CONTROLLER_MAX_EVENT_PAYLOAD);
	hci_rx_buf = lbuf_init(hci_buf + HCI_BUF_RX_POS, CONTROLLER_MAX_RX_PAYLOAD);

    ll_flow_control.free_num_hci_acl_packets = le_param_t->priv->buf_param.total_num_le_data_pkt;
    hci_param_init();

	thread_create(&hci_thread, hci_thread_process, 3);

    if (bt_power_is_poweroff_post())
    {
        __ll_api->init(&hci_param);
    }
    /* aes128_test(); */
    /* debug_ll_privacy(); */
    /* lbuf_debug(); */
}

static void lbuf_debug(void)
{
    u8 *ptr;
    ptr = lbuf_alloc(hci_rx_buf, 10);
    printf("lbuf is empty : %x\n", lbuf_empty(hci_rx_buf));
    lbuf_push(ptr);
    printf("lbuf is empty : %x\n", lbuf_empty(hci_rx_buf));
    printf("lbuf have next: %x\n", lbuf_have_next(hci_rx_buf));
    printf("lbuf is empty : %x / pop %x\n", lbuf_empty(hci_rx_buf), lbuf_pop(hci_rx_buf));
    printf("lbuf have next: %x\n", lbuf_have_next(hci_rx_buf));
    printf("lbuf is empty : %x / pop %x\n", lbuf_empty(hci_rx_buf), lbuf_pop(hci_rx_buf));
    printf("lbuf have next: %x\n", lbuf_have_next(hci_rx_buf));
    lbuf_free(ptr);
    printf("lbuf is empty : %x\n", lbuf_empty(hci_rx_buf));
}





