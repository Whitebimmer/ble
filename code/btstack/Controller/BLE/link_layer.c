#include "ble/link_layer.h"
#include "stdarg.h"
#include "thread.h"
#include "ble/ble_h4_transport.h"
#include "bt_memory.h"
#include "RF_ble.h"


#define LL_DEBUG_EN

#ifdef LL_DEBUG_EN
#define ll_putchar(x)        putchar(x)
#define ll_puts(x)           puts(x)
#define ll_u32hex(x)         put_u32hex(x)
#define ll_u8hex(x)          put_u8hex(x)
#define ll_pbuf(x,y)          printf_buf(x,y)
#define ll_printf            printf
#else
#define ll_putchar(...)
#define ll_puts(...)
#define ll_u32hex(...)
#define ll_u8hex(...)
#define ll_pbuf(...)
#define ll_printf(...)
#endif


#define BLE_MASTER   0
#define BLE_SLAVE 	 1

#define LINK_NUM 	    8	
#define LINK_MEM_SIZE  512 

#define READ_BT16(p, offset)  (((u16)p[(offset)+1]<<8) | p[offset])

struct oneshot{
	struct list_head entry;
	void (*func)(void *priv);
	int  data[0];
};


struct link_layer{
	u8 handle;
	struct list_head head;
    const struct lc_handler *handler;
	struct thread ll_thread;
};

//============= data length 4.2 feature begin

static const struct ll_data_pdu_length_read_only ll_data_pdu_length_ro = {

    /* .supportedMaxTxOctets = 27, */
    /* .supportedMaxTxTime = 328, */

    /* .supportedMaxRxOctets = 27, */
    /* .supportedMaxRxTime = 328, */

    /* if (LE_FEATURES_IS_SUPPORT(LE_DATA_PACKET_LENGTH_EXTENSION)) */
    .supportedMaxTxOctets = BLE_HW_MAX_RX_OCTETES,
    .supportedMaxTxTime = 2120,

    .supportedMaxRxOctets = BLE_HW_MAX_TX_OCTETES,
    .supportedMaxRxTime = 2120,
};


struct ll_data_pdu_length le_data_length;

//============= data length 4.2 feature end
//
//============= privacy 4.2 feature begin
struct ll_privacy{
    struct sys_timer RPA_timeout;
    u32 RPA_timeout_cnt;

    u8 random48[6];
};

struct ll_privacy le_privacy;
//============= privacy 4.2 feature end

u8 ll_error_code;
/*
 *-------------------HCI PARAMETER
 */
static struct hci_parameter *hci_param_t SEC(.btmem_highly_available);

static struct link_layer ll SEC(.btmem_highly_available);


#define ROLE_IS_SLAVE(link)         (((struct le_link *)link)->role)
#define READ_CONNECTION_HANDLE(buffer)  (READ_BT_16(buffer,0) & 0x0fff)
/*******************************************************************/
/*
 *-------------------LE READ PARAMETER
 */

#define LL_TRANSMIT_POWER_LEVEL         0x1

#define LL_ACL_PDU_LENGTH               64
#define LL_TOTAL_NUM_LE_DATA_PACKET     (BLE_HW_TX_SIZE/LL_ACL_PDU_LENGTH - 5)
#if (LL_TOTAL_NUM_LE_DATA_PACKET > (BLE_HW_TX_SIZE/LL_ACL_PDU_LENGTH))
        #error LL_TOTAL_NUM_LE_DATA_PACKET OVERFLOW
#endif


#define LL_WHITE_LIST_SIZE              1
#define LL_RESOLVING_LIST_SIZE          1


#define VERSNR      0x8         //Core Spec 4.2
#define COMPLD      0x0078      //Nike Inc
#define SUBVERSNR   0x0000

#ifndef	BLE_PRIVACY_EN
#define LE_FEATURES         (LE_ENCRYPTION|LE_SLAVE_INIT_FEATURES_EXCHANGE|LE_DATA_PACKET_LENGTH_EXTENSION)//CONNECTION_PARAMETER_REQUEST
#else
#define LE_FEATURES         (LE_ENCRYPTION|LE_SLAVE_INIT_FEATURES_EXCHANGE)
#endif

static const struct le_read_parameter le_read_param = {
    .features = {LE_FEATURES, 0, 0, 0, 0, 0, 0, 0},
    .version = {0x06, 0x0f, 0x00, 0x09, 0x61},
    .versnr = VERSNR,
    .compld = COMPLD,
    .subversnr = SUBVERSNR,

    //div 2 for lbuf struct cost & hw overlayed
    .buf_param.total_num_le_data_pkt = LL_TOTAL_NUM_LE_DATA_PACKET,
    .buf_param.buffer_size = LL_ACL_PDU_LENGTH,

    .white_list_size        = LL_WHITE_LIST_SIZE,
    .transmit_power_level   = LL_TRANSMIT_POWER_LEVEL,
    .resolving_list_size    = LL_RESOLVING_LIST_SIZE,
};

#define LE_FEATURES_IS_SUPPORT(x)      (le_read_param.features[0] & x)


static struct le_parameter le_param SEC(.btmem_highly_available);
/*******************************************************************/

/*******************************************************************/
/*
 *-------------------LE Link Memory
 */

#if 0
static u8 mem_pool[LINK_MEM_SIZE * LINK_NUM] __attribute__((aligned(4)));

static void * __malloc(int size)
{
	void *p;

	p = lbuf_alloc(mem_pool, size);
	ASSERT(p != NULL, "%s\n", "link mem pool full\n");

	return p;
}

static void __free(void *p)
{
	lbuf_free(p);
}

#else

static void * __malloc(int size)
{
    void *p;

	p = bt_malloc(BTMEM_HIGHLY_AVAILABLE, size);
	ASSERT(p != NULL);

	return p; 
}

static void __free(void *p)
{
    bt_free(p);
}
#endif

/*******************************************************************/
/*
 *-------------------HCI LE Command Paramter Memory
 */
#if 0
static u8 hci_mem_pool[256] __attribute__((aligned(4)));

static void *__hci_param_malloc(int size)
{
	void *p;

	p = lbuf_alloc(hci_mem_pool, size);
	ASSERT(p != NULL, "%s\n", "HCI mem pool full\n");

	return p;
}

static void __hci_param_free(void *p)
{
	lbuf_free(p);
}
#else

static void *__hci_param_malloc(int size)
{
	void *p;

	p = bt_malloc(BTMEM_HIGHLY_AVAILABLE, size);
	ASSERT(p != NULL, "%s\n", "HCI mem pool full\n");

	return p;
}

static void __hci_param_free(void *p)
{
    bt_free(p);
}
#endif
/*******************************************************************/


static void __event_oneshot_add(struct le_link *link, 
		void (*func)(struct le_link *), int instant)
{
	struct oneshot *p = __malloc(sizeof(struct oneshot)+4);

	ASSERT(p != NULL, "%s\n", __func__);

	if (p != NULL){
		p->func = func;
		p->data[0] = instant;
		list_add_tail(&p->entry, &link->event_oneshot_head);
	}
}

static void __event_oneshot_run(struct le_link *link, int event_cnt)
{
	struct oneshot *p, *n;

    /* putchar('#'); */
    /* put_u16hex(event_cnt); */
	list_for_each_entry_safe(p, n, &link->event_oneshot_head, entry){
		if (event_cnt == p->data[0]){
			list_del(&p->entry);
			p->func(link);
			__free(p);
		}
	}
}

static void __event_oneshot_remove(struct le_link *link, 
        void (*func)(struct le_link *), int instant)
{
	struct oneshot *p, *n;

    /* putchar('#'); */
    /* put_u16hex(event_cnt); */
	list_for_each_entry_safe(p, n, &link->event_oneshot_head, entry){
		if ((func == p->func) && (instant == p->data[0])){
			list_del(&p->entry);
			__free(p);
		}
	}
}

static void __rx_oneshot_add(struct le_link *link, void (*func)(struct le_link*))
{
	struct oneshot *p = __malloc(sizeof(struct oneshot));

	ASSERT(p != NULL, "%s\n", __func__);

	if (p != NULL){
		p->func = func;
		list_add_tail(&p->entry, &link->rx_oneshot_head);
	}
}

static void __rx_oneshot_run(struct le_link *link)
{
	struct oneshot *p, *n;

	list_for_each_entry_safe(p, n, &link->rx_oneshot_head, entry){
		list_del(&p->entry);
		p->func(link);
		__free(p);
	}
}


/*******************************************************************/
/*
 *-------------------LE EVENT PARAMETER
 */
static struct le_event_parameter le_event_param;

/********************************************************************************/
/*
 *                  7.8.1 LE_SET_EVENT_MASK
 *
 *                  EVENT MASK(LE META EVENT MASK)
 */

struct le_event{
	u8 event;
	u8 len;
	u8 data[0];
}__attribute__((packed));

struct lbuff_head *le_event_buf SEC(.btmem_highly_available);

static u8 ll_buf[512] __attribute__((aligned(4)));

static struct le_event *__alloc_event(int len, const char *format)
{
	struct le_event *event;

	len += __vsprintf_len(format);
	event = lbuf_alloc(le_event_buf, sizeof(*event)+len);
	if (event == NULL){
		ASSERT(event!= NULL, "%d, %s\n", len, format);
		return NULL;
	}
	event->len = len;

	return event;
}

static bool meta_event_mask(int subevent_code)
{
    if (hci_param_t->event_mask[LE_META_EVENT/8] & BIT(LE_META_EVENT%8))
    {
        /* subevent_code -= 1; */
        if (le_param.event_mask[subevent_code/8] & BIT(subevent_code%8))
        {
            return TRUE;            
        }
    }
    ll_puts("LE event mask - disable");
    return FALSE;
}

#define EVENT_PACKET_BUFFER_SIZE    0x80
static u8 event_packet_buffer[EVENT_PACKET_BUFFER_SIZE];

static bool __hci_emit_le_meta_event_static(u8 subevent_code, const char *format, ...)
{
	struct le_event *event_static;

    if (meta_event_mask(subevent_code) == FALSE)
    {
        return FALSE;
    }

    event_static = event_packet_buffer;
	event_static->event = HCI_EVENT_LE_META;
	event_static->data[0] = subevent_code;
    event_static->len = 1;

	va_list argptr;
	va_start(argptr, format);
	event_static->len += __vsprintf(event_static->data+1, format, argptr);
	va_end(argptr);

	struct le_event *event;

	event = lbuf_alloc(le_event_buf, sizeof(*event)+event_static->len);
	if (event == NULL){
		ASSERT(event!= NULL, "%d, %s\n", event_static->len, format);
		return NULL;
	}
    memcpy(event, event_static, sizeof(*event)+event_static->len);

	lbuf_push(event);

    //resume ll thread
    thread_resume(&ll.ll_thread);

    return TRUE;
}

static bool __hci_emit_le_meta_event(u8 subevent_code, const char *format, ...)
{
	struct le_event *event;

    if (meta_event_mask(subevent_code) == FALSE)
    {
        return FALSE;
    }

	event = __alloc_event(1, format);
	ASSERT(event != NULL);
    printf("event len : %x\n", event->len);

	event->event = HCI_EVENT_LE_META;
	event->data[0] = subevent_code;

	va_list argptr;
	va_start(argptr, format);
	__vsprintf(event->data+1, format, argptr);
	va_end(argptr);

	lbuf_push(event);

    //resume ll thread
    thread_resume(&ll.ll_thread);

    return TRUE;
	/* return 0; */
}

static int le_meta_event_upload()
{
    int awake = 0;

	struct le_event *event;

	event = lbuf_pop(le_event_buf);
	if (event){
        puts("\nEMIT : META EVENT ");
        printf_buf(event, sizeof(*event)+event->len);

		ble_h4_packet_handler(HCI_EVENT_PACKET, event, sizeof(*event)+event->len);
		lbuf_free(event);
    }

    {
       awake = 1; 
    }
    return awake;
}

/*******************************************************************/

/********************************************************************************/
/*
 *                  LL Control Data State Machine
 */
#define WAIT_TX                 (BIT(15))
#define WAIT_RX                 (BIT(14))
#define LL_HALT                 (WAIT_RX|WAIT_TX)

typedef u16 ll_step_extend;
typedef u8  ll_step;


static struct ll_control_data_step ll_ctrl_step;

#define LL_FINISH                   0xff
#define LL_DONE(x)                  (LL_FINISH | (ll_step_extend)x<<8)
#define LL_IS_FINSH()               ((ll_ctrl_step.steps_ptr[ll_ctrl_step.cnt] & 0xff) == LL_FINISH)

#define LL_STEPS()                  (ll_ctrl_step.steps_ptr[ll_ctrl_step.cnt]>>8)

struct ll_control_data_step{
    u8 cnt;
    const ll_step_extend *steps_ptr;
};

//get Local Host indicate LL control procedure
static ll_step ll_control_current_procedure_by_localhost()
{
    u8 cnt;

    for(cnt = ll_ctrl_step.cnt; ; cnt++) 
    {
        if ((ll_ctrl_step.steps_ptr[cnt] & 0xff) == LL_FINISH)
        {
            
            return (ll_ctrl_step.steps_ptr[cnt]>>8);
        }
    }
}

//Master LL control procedure
static u8 master_ll_control_current_procedure;

bool ll_control_procedure_push(u8 procedure)
{
    if (master_ll_control_current_procedure)
    {
        //in procedure collisions occur
        return FALSE;
    }

    master_ll_control_current_procedure = procedure;

    return TRUE;
}

void ll_control_procedure_pop(u8 procedure)
{
    if (master_ll_control_current_procedure == procedure)
    {
        //procedure end
        master_ll_control_current_procedure = 0;
    }
    else{
        //unexcepted case
        ASSERT(0, "%s\n", __func__);
    }
}

#define LL_PROCEDURE_IS_COLLISIONS(x)       (ll_control_procedure_push(x) == FALSE)


#define LL_CTRL_NO_STEPS()    (ll_ctrl_step.steps_ptr == NULL)

            
typedef enum{
    LL_CONTROL_SUCCESS = 0,
    LL_CONTROL_REJECT,
    LL_CONTROL_EXT_REJECT,
    LL_CONTROL_UNKNOW_RSP,
    LL_CONTROL_TIMEOUT,
}LL_CONTROL_CASE;

static LL_CONTROL_CASE ll_control_procedure_case_remap(u16 error_case)
{
    if (error_case == LL_CONTROL_SUCCESS)
    {
        return LL_CONTROL_SUCCESS; 
    }
    //make different case united
    LL_CONTROL_CASE ll_case;

    switch(error_case)
    {
    case (LL_REJECT_IND|WAIT_RX):
        ll_case = LL_CONTROL_REJECT;
        break;
    case (LL_REJECT_IND_EXT|WAIT_RX):
        ll_case = LL_CONTROL_EXT_REJECT;
        break;
    case (LL_UNKNOWN_RSP|WAIT_RX):
        ll_case = LL_CONTROL_UNKNOW_RSP;
        break;
    default:
        break;
    }

    return ll_case;
}

#define LL_CONTROL_CASE_REMAP(x)    ll_control_procedure_case_remap(x)


static void ll_control_procedure_finish_emit_event(struct le_link *link, struct ble_rx *rx, LL_CONTROL_CASE status);



static void ll_control_procedure_finish(struct le_link *link, struct ble_rx *rx, LL_CONTROL_CASE status)
{
    ll_control_procedure_finish_emit_event(link, rx, status);
    ll_ctrl_step.steps_ptr = NULL;
}

static void ll_response_timeout_stop(struct le_link *link);
static void ll_control_step_verify(ll_step_extend step_ex, 
        struct le_link *link, struct ble_rx *rx, struct ble_tx *tx)
{
    if (ll_ctrl_step.steps_ptr == NULL)
    {
        //filte out not Host indicate case
        return;
    }
    
    /* if (rx) */
    /* { */
        /* printf("rx steps : %04x\n", step_ex); */
    /* } */
    /* if (tx) */
    /* { */
        /* printf("tx steps : %04x\n", step_ex); */
    /* } */
    //procedure break by accident case
    if ((step_ex == (LL_REJECT_IND|WAIT_RX)) 
            || (step_ex == (LL_REJECT_IND_EXT|WAIT_RX))
            || (step_ex == (LL_UNKNOWN_RSP|WAIT_RX)))
    {
        ll_puts("\n##ll break reson : ");ll_u32hex(step_ex);
        ll_response_timeout_stop(link);
        ll_control_procedure_finish(link, rx, LL_CONTROL_CASE_REMAP(step_ex));
        return;
    }

    /* ll_printf("ll cur step : %02x - %04x - ", ll_ctrl_step.cnt, ll_ctrl_step.steps_ptr[ll_ctrl_step.cnt]); */
    /* ll_printf(" %04x\n", step_ex); */
    if (step_ex == ll_ctrl_step.steps_ptr[ll_ctrl_step.cnt])
    {
        ll_ctrl_step.cnt++;
        /* printf("step list : %08x\n", ll_ctrl_step.steps_ptr); */

        /* ll_printf("ll step ++: %02x - %04x\n", ll_ctrl_step.cnt, ll_ctrl_step.steps_ptr[ll_ctrl_step.cnt]); */
        if (LL_IS_FINSH())
        {
            ll_puts("##ll finish\n");
            //procedure finish
            ll_response_timeout_stop(link);
            ll_control_procedure_finish(link, rx, LL_CONTROL_SUCCESS);
        }
        return;
    }
    //unexpect case step not match
	/* ASSERT(0, "%s\n", __func__); */
}


static void echo_ll_ctrl_pdu_handler(struct le_link *link, ll_step step, 
        struct ble_rx *rx, struct ble_tx *tx);

static void ll_send_control_data_state_machine(struct le_link *link, 
        struct ble_rx *rx, struct ble_tx *tx)
{
    ll_step_extend step_ex;

    if (ll_ctrl_step.steps_ptr == NULL)
    {
        return;
    }

    step_ex = ll_ctrl_step.steps_ptr[ll_ctrl_step.cnt];
    /* printf("echo step : %04x\n", step_ex); */

    if ((step_ex & LL_HALT))
    {
        ll_puts(" - LL_HALT...");
        return;
    }

    //normal LL control data
    ll_step step;

    step = step_ex & 0xff;

    echo_ll_ctrl_pdu_handler(link, step, rx, tx);

    ll_control_step_verify(step, link, rx, tx);
}

static void ll_control_data_step_start(const ll_step_extend *steps)
{
    ll_ctrl_step.steps_ptr = steps;

    ll_ctrl_step.cnt = 0;

    /* printf("step list : %08x - %08x\n", steps, ll_ctrl_step.steps_ptr); */

    /* u8 i; */

    /* for(i = 0; i < 4; i++) */
    /* { */
        /* printf("start_encryption_req_steps list %02x - %04x\n", i, ll_ctrl_step.steps_ptr[i]); */
    /* } */

    ll_send_control_data_state_machine(NULL, NULL, NULL);
}


/********************************************************************************/


void aes128_start(u8 key[16], u8 plaintext[16], u8 encrypt[16])
{
	CPU_SR_ALLOC();

	CPU_INT_DIS();
	aes128_start_enc(key, plaintext, encrypt);
	CPU_INT_EN();
}

static struct le_link * __new_link()
{
	struct le_link * link;

	link = (struct le_link *)__malloc(sizeof(*link));

	ASSERT(link != NULL, "%s\n", "link alloc err");

	if (!link){
		return NULL;
	}

	memset(link, 0, sizeof(struct le_link));

	return link;
}

static void __free_link(struct le_link *link)
{
	list_del(&link->entry);
	
	__free(link);
}


static struct le_link *ll_create_link()
{
	struct le_link *link;

	link = __new_link();
	if (!link){
		return NULL;
	}

	link->state = LL_STANDBY;
    /* link->handle = ll.handle; */

    /* puts("\nbefore create link : "); */
    /* put_u32hex(ll.head.next); */
	list_add_tail(&link->entry, &ll.head);
    /* puts("\nafter create link : "); */
    /* put_u32hex(ll.head.next); */
    
    /* puts("\ncreate link addr: "); */
    /* put_u32hex(link); */

	return link;
}

static struct le_link *ll_link_for_address(u8 addr_type, const u8 *addr)
{
	struct le_link *link;

	list_for_each_entry(link, &ll.head, entry){
		if ((addr_type == link->peer.addr_type)
            && (!memcmp(link->peer.addr, addr, 6)))
        {
			return link;
		}
	}

	return NULL;
}

static struct le_link *ll_link_for_state(u8 state)
{
	struct le_link *link;

	list_for_each_entry(link, &ll.head, entry){
		if (link->state == state){
			return link;
		}
	}

	return NULL;
}


struct le_link *ll_link_for_handle(int handle)
{
	struct le_link *link;

	list_for_each_entry(link, &ll.head, entry){
		if (link->handle == handle){
			return link;
		}
	}

	return NULL;
}


/********************************************************************************/
/*
 *                  Link Layer Control Data (only for link control layer used)
 */
static void ll_send_control_data(struct le_link *link, int opcode, 
	   	const char *format, ...)
{
	int len = 1;

	ASSERT(__vsprintf_len(format) < sizeof(link->send_buf));

	link->send_buf[0] = opcode;

	if (format){
		va_list argptr;
		va_start(argptr, format);
		len += __vsprintf(link->send_buf+1, format, argptr);
		va_end(argptr);
	}

    //printf_buf(link->send_buf, len);
	__ble_ops->send_packet(link->hw, LL_CONTROL_PDU, link->send_buf, len);

}
/* REGISTER_LLP_CTRL_TXCHANNEL(ll_send_control_data) */

/********************************************************************************/
/*
 *                  Link Layer ACL Data (for Upper layer used)
 */

u8 g_pkt_idx;

void ll_send_acl_packet(int handle, u8 *packet, int total_len)
{
    struct le_link *link;

    link = ll_link_for_handle(handle);
    ASSERT(link != NULL, "%s\n", __func__);

    u8 pb_flag, bc_flag;

    pb_flag = packet[1];
    pb_flag >>= 4;

    bc_flag = packet[1];
    bc_flag >>= 6;

    int rf_tx_idx;
    u8 rf_tx_len;
    u8 *rf_tx_packet;

    //packet payload begin
    rf_tx_packet = packet + 4;
    rf_tx_idx = 0;

    //flow control
    //create node
    struct fragment_pkt *pkt;
    
    //fill in node info 1 fragment N RF packets
    pkt =  __malloc(sizeof(struct fragment_pkt));
    ASSERT(pkt != NULL);
    pkt->cnt = 0;

    do
    {
        /* if (total_len > link->pdu_total_len.connEffectiveMaxTxOctets) */
        /* { */
            /* puts("greater than connEffectiveMaxTxOctets error!\n"); */
        /* } */
        /* if (total_len > link->pdu_total_len.connEffectiveMaxTxTime) */
        /* { */
            /* puts("greater than connEffectiveMaxTxTime error!\n"); */
        /* } */
        //Continuing fragment of Higher Layer Message
        rf_tx_len = (total_len > link->pdu_len.connEffectiveMaxTxOctets) ? 
            link->pdu_len.connEffectiveMaxTxOctets : total_len;
        ll_puts("\nTX : DATA");
        ll_pbuf(rf_tx_packet, rf_tx_len);
        if (((pb_flag & 0x1) == 0) && (rf_tx_idx == 0))
        {
            __ble_ops->send_packet(link->hw, LL_DATA_PDU_START, rf_tx_packet, rf_tx_len);
        }
        else{
            __ble_ops->send_packet(link->hw, LL_DATA_PDU_CONTINUE, rf_tx_packet, rf_tx_len);
        }

        rf_tx_idx += rf_tx_len;
        rf_tx_packet += rf_tx_len;
        total_len -= rf_tx_len; 

        pkt->cnt++;
    }while(total_len);

    g_pkt_idx++;
    list_add_tail(&pkt->entry, &pkt_list_head);
    ll_puts("ll_send_acl_exit\n");
}
/* REGISTER_LLP_ACL_TXCHANNEL(ll_send_acl_packet) */

/********************************************************************************/

static void __hci_event_emit(u8 procedure, struct le_link *link, struct ble_rx *rx, LL_CONTROL_CASE status)
{
    if (ll.handler && ll.handler->event_handler){
        ll.handler->event_handler(procedure, link, rx, status);
    }
}

static const u8 connection_data[] = {
    //aa    
	0xe8, 0x98, 0xeb, 0x6d, 
    //crcinit
	0xae, 0x83, 0x19, 
    //winsize
	0x02, 
    //winoffset
	0x0a, 0x00,
    //interval
   	0x27, 0x00,
    //latency
   	0x00, 0x00, 
    //timeout
	0x32, 0x00, 
    //channel map
	0xff, 0xff, 0xff, 0xff, 0x1f,
    //hop
   	0x87
};



static void __read_connection_param(struct ble_conn_param *param, u8 *data)
{
	memcpy(param->aa, data, 4);
	memcpy(param->crcinit, data+4, 3);

	param->winsize = data[7];
	param->winoffset = READ_BT16(data, 8);
	param->interval = READ_BT16(data, 10);
	param->latency = READ_BT16(data, 12);
    param->timeout = READ_BT16(data, 14);

	memcpy(param->channel, data+16, 5);
	param->hop = data[21]&0x1f;
	/*printf("hop: %d\n", param->hop);*/
}

/********************************************************************************/
/*
 *                  Device Filtering(white list)
 */

static u8 ll_white_list_used SEC(.btmem_highly_available);

static void ll_white_list_init(void)
{
    ll_white_list_used = 0;
	INIT_LIST_HEAD(&le_param.white_list_head);
}

static void ll_white_list_add(const u8 *data)
{
    struct white_list *list;

    if (ll_white_list_used == LL_WHITE_LIST_SIZE)
        return;
	
    list =  __malloc(sizeof(struct white_list));
    ASSERT(list != NULL);

    memcpy((u8 *)&list->white_list_param, data, sizeof(struct white_list) - sizeof(struct list_head));
    list->cnt = 0;

    list_add_tail(&list->entry, &le_param.white_list_head);
    ll_white_list_used++;
}

static void ll_white_list_remove(const u8 *data)
{
    struct white_list *p, *n;

	list_for_each_entry_safe(p, n, &le_param.white_list_head, entry){
		if ((!memcmp(data + 1, p->white_list_param.Address, 6)) 
                && (p->white_list_param.Address_Type == data[0]))
        {
            ll_white_list_used--;
			list_del(&p->entry);
			__free(p);
		}
	}
}


static void __white_list_upadte(struct le_link *link, struct white_list *device)
{
    /* printf("white list addr type : %02x\n", device->white_list_param.Address_Type); */
    /* printf_buf(device->white_list_param.Address, 6); */

    __ble_ops->ioctrl(link->hw, BLE_WHITE_LIST_ADDR,
            device->white_list_param.Address_Type, 
            device->white_list_param.Address); 
}
//for hw resource share
static void __white_list_weighted_round_robin(struct le_link *link, struct white_list *device)
{
    if (device->cnt++ == 10)
    {
        device->cnt = 0;
        //TO*DO privacy RPA
        __white_list_upadte(link, device);
    }
    //TO*DO
    //lost device PDUs a few mins should clean device counter
	
	//TO*DO
	//one more device have the same device counter (same interval)
}

static struct white_list *ll_white_list_match(struct le_link *link, u8 addr_type, const u8 *addr)
{
    struct white_list *p;

	list_for_each_entry(p, &le_param.white_list_head, entry)
    {
        /* printf("white list addr type : %02x\n", p->white_list_param.Address_Type); */
        /* printf_buf(p->white_list_param.Address, 6); */

        /* printf("2 -white list addr type : %02x\n", addr_type); */
        /* printf_buf(addr, 6); */

		if ((!memcmp(addr, p->white_list_param.Address, 6)) 
            && (p->white_list_param.Address_Type == addr_type))
        {
            //high frequently
            /* __white_list_weighted_round_robin(link, p); */
            __white_list_upadte(link, p);
            return p;
        }
    }
}


/********************************************************************************/

/********************************************************************************/
/*
 *                  Resolvable Private Address 
 */
enum{
    NON_RESOLVABLE_PRIVATE_ADDR = 0,
    RESOLVABLE_PRIVATE_ADDR = 2,
    STATIC_DEVICE_ADDR,
}DEVICE_ADDR_TYPE;

static void irk_enc(const u8 *irk, const u8 *prand, u8 *hash)
{
    u8 r[16];
    u8 ah[16];

    memset(r, 0x0, 16);
    //padding 
    memcpy(r, prand, 3);

    aes128_start(irk, r, ah);

    memcpy(hash, ah, 3);
}


static bool device_addr_verify(const u8 *addr, u8 start, u8 len)
{
    u8 i, res;

    u8 addr_temp[6];

    memcpy(addr_temp, addr, 6);
    /* All bits of the random part of the address shall not be equal to 1  */
    res = 0xff;
    for (i = start; i < start + len; i++)
    {
        //MSB
        if (i == 5)
        {
            addr_temp[i] &= 0x3f;
            addr_temp[i] |= 0xc0;
        }
        res &= addr_temp[i];
    }
    if (res == 0xff)
    {
        puts("addr verify fail all 1!\n");
        return FALSE;
    }

    /* All bits of the random part of the address shall not be equal to 0 */
    res = 0x00;
    for (i = start; i < start + len; i++)
    {
        //MSB
        if (i == 5)
        {
            addr_temp[i] &= 0x3f;
        }
        res |= addr_temp[i];
    }
    if (res == 0x00)
    {
        puts("addr verify fail all 0!\n");
        return FALSE;
    }

    /* puts("addr verify pass!\n"); */
    return TRUE;
}

static void __random48_update(void)
{
    u8 i;
    
    for(i = 0; i < 6;)
    {
        le_privacy.random48[i++] = RAND64L;
        le_privacy.random48[i++] = RAND64H;
    }
}

static void device_addr_generate(u8 *addr, u8 *irk, u8 type)
{

    switch(type)
    {
        /* The two most significant bits of the address shall be equal to 1  */
        case STATIC_DEVICE_ADDR:
            /* A device may choose to initialize its static address to a new value after each */
            /* power cycle. A device shall not change its static address value once initialized */
            /* until the device is power cycled. */
            le_privacy.random48[5] |= 0xc0;
            memcpy(addr, le_privacy.random48, 6);
            break;

        /* The two most significant bits of the address shall be equal to 0 */
        /* The address shall not be equal to the public address */
        case NON_RESOLVABLE_PRIVATE_ADDR:
            le_privacy.random48[5] &= ~0xc0;
            memcpy(addr, le_privacy.random48, 6);
            break;

        /* The two most significant bits of rand shall be equal to 0 and 1  */
        case RESOLVABLE_PRIVATE_ADDR:
            /* hash = ah(irk, ple_privacy.random48); */
            //LSB
            le_privacy.random48[2] &= ~0xc0;
            le_privacy.random48[2] |= 0x40;
            irk_enc(irk, le_privacy.random48, addr);

            //MSB
            memcpy(addr + 3, le_privacy.random48, 3);
            /* memcpy(addr, hash, 3); */
            break;
    }
}

static void device_addr_resolve(u8 *irk, u8 *addr)
{
    u8 type;

    type = addr[5] & 0xc0;
    type >>= 6;

    if (type == RESOLVABLE_PRIVATE_ADDR)
    {
    
    }
}

/********************************************************************************/
/********************************************************************************/
/*
 *                   Privacy(resolving list)
 */

static u8 ll_resolving_list_used SEC(.btmem_highly_available);

static bool __resolve_list_IRK_verify(const u8 *irk)
{
    u8 i;
    u8 sum;

    for (i = 0, sum = 0; i < 16; i++)
    {
        sum |= irk[i];
    }

    //check not all zero
    return (sum) ? TRUE : FALSE;
}

static void ll_resolving_list_init(void)
{
    ll_resolving_list_used= 0;
	INIT_LIST_HEAD(&le_param.resolving_list_head);
}

static void ll_resolving_list_add(const u8 *data)
{
    struct resolving_list *list;

    if (ll_resolving_list_used == LL_RESOLVING_LIST_SIZE)
        return;
	
    list =  __malloc(sizeof(struct resolving_list));
    ASSERT(list != NULL);

    memcpy((u8 *)&list->resolving_list_param, data, sizeof(struct resolving_list_parameter));

    //Device privacy is violated when all zero IRKs
    /* ASSERT(((__resolve_list_IRK_verify(list->resolving_list_param.Local_IRK) == TRUE) */
            /* &&(__resolve_list_IRK_verify(list->resolving_list_param.Peer_IRK) == TRUE)), "%s\n", __func__); */

    puts("\n ----------Resolving List --------------\n");
    printf("addr type : %x\n", list->resolving_list_param.Peer_Identity_Address_Type);
    puts("addr : "); printf_buf(list->resolving_list_param.Peer_Identity_Address, 6);
    puts("local IRK : "); printf_buf(list->resolving_list_param.Local_IRK, 16);
    puts("peer IRK : "); printf_buf(list->resolving_list_param.Peer_IRK, 16);
    list_add_tail(&list->entry, &le_param.resolving_list_head);
    ll_resolving_list_used++;
    puts("\n ----------Resolving List End--------------\n");
}

static void ll_resolving_list_remove(const u8 *data)
{
    struct resolving_list *p, *n;

	list_for_each_entry_safe(p, n, &le_param.resolving_list_head, entry)
    {
		if ((!memcmp(data+1, p->resolving_list_param.Peer_Identity_Address, 6)) \
                && (p->resolving_list_param.Peer_Identity_Address_Type == data[0])) 
        {
            ll_resolving_list_used--;
			list_del(&p->entry);
			__free(p);
		}
	}
}


static struct resolving_list *ll_resolving_list_match(const u8 addr_type, const u8 *addr)
{
    struct resolving_list *p, *n;

    ASSERT(addr != NULL, "%s\n", __func__);

	list_for_each_entry_safe(p, n, &le_param.resolving_list_head, entry){
		if ((!memcmp(addr, p->resolving_list_param.Peer_Identity_Address, 6)) \
                && (p->resolving_list_param.Peer_Identity_Address_Type == addr_type)) 
        {
            return p;
        }
    }
	return NULL;
}
static void __set_ll_public_device_addr(u8 *addr)
{
    /* puts(__func__); */
    /* printf_buf(hci_param_t->priv->public_addr, 6); */
    memcpy(addr, hci_param_t->priv->public_addr, 6);
}


static bool __ll_random_device_addr_verify(const u8 *addr)
{
    /* The two most significant bits of the address shall be equal to 1  */
    if ((addr[5] & 0xC0) != 0xC0)
    {
        return FALSE;
    }
    return device_addr_verify(addr, 0, 6);
}

static void __set_ll_random_device_addr(u8 *addr)
{
    memcpy(addr, le_param.random_address, 6);
}

/* static void __set_ll_random_device_addr(struct le_link *link) */
/* { */
    /* device_addr_generate(hci_param_t->static_addr, 0, STATIC_DEVICE_ADDR); */

    /* memcpy(link->local_addr, hci_param_t->static_addr, 6); */
/* } */

static bool __ll_non_resolvable_private_addr_verify(const u8 *addr)
{
    /* The two most significant bits of the address shall be equal to 0 */
    if ((addr[5] & 0xC0) != 0x00)
    {
        return FALSE;
    }
    /* The address shall not be equal to the public address */
    if (!memcmp(addr, hci_param_t->priv->public_addr, 6))
    {
        return FALSE;
    }
    return device_addr_verify(addr, 0, 6);
}

static void __set_ll_local_non_resolvable_private_addr(u8 *addr,
        struct resolving_list *resolving_list_t)
{
    ASSERT(resolving_list_t != NULL, "%s\n", __func__);

    device_addr_generate(resolving_list_t->local_PRA,\
           0, NON_RESOLVABLE_PRIVATE_ADDR);

    memcpy(addr, resolving_list_t->local_PRA, 6);
}

static void __set_ll_peer_non_resolvable_private_addr(u8 *addr,
        struct resolving_list *resolving_list_t)
{
    ASSERT(resolving_list_t != NULL, "%s\n", __func__);

    device_addr_generate(resolving_list_t->peer_PRA,\
           0, NON_RESOLVABLE_PRIVATE_ADDR);

    memcpy(addr, resolving_list_t->peer_PRA, 6);
}

static bool __ll_resolvable_private_addr_verify(const u8 *addr)
{
    /* The two most significant bits of the address shall be equal to 0 */
    if ((addr[5] & 0xC0) != 0x40)
    {
        return FALSE;
    }
    return device_addr_verify(addr, 3, 3);
}


#ifdef BLE_PRIVACY_EN
u8 g_privacy_flag = 0;
#endif

static void __set_ll_local_resolvable_private_addr(u8 *addr,
        struct resolving_list *resolving_list_t)
{
    ASSERT(resolving_list_t != NULL, "%s\n", __func__);

    ASSERT((__resolve_list_IRK_verify(resolving_list_t->resolving_list_param.Local_IRK) == TRUE), "%s\n", __func__);

    device_addr_generate(resolving_list_t->local_PRA,\
           resolving_list_t->resolving_list_param.Local_IRK, RESOLVABLE_PRIVATE_ADDR);

   memcpy(addr, resolving_list_t->local_PRA, 6);

#ifdef BLE_PRIVACY_EN
   if(g_privacy_flag)
   {
		u8 addr_1[6] ={0x8A,0x09,0xCD,0xC1,0x82,0x6A};
		memcpy(addr, addr_1, 6);
   }
#endif
}


static void __set_ll_peer_resolvable_private_addr(u8 *addr,
        struct resolving_list *resolving_list_t)
{
    ASSERT(resolving_list_t != NULL, "%s\n", __func__);

    ASSERT((__resolve_list_IRK_verify(resolving_list_t->resolving_list_param.Peer_IRK) == TRUE), "%s\n", __func__);

    device_addr_generate(resolving_list_t->peer_PRA,\
           resolving_list_t->resolving_list_param.Peer_IRK, RESOLVABLE_PRIVATE_ADDR);

    memcpy(addr, resolving_list_t->peer_PRA, 6);
}

static void __set_ll_adv_local_RPA(struct le_link *link)
{
    struct resolving_list *resolving_list_t = NULL;

    switch (le_param.adv_param.Own_Address_Type)
    {
        case 2:
        case 3:
            //pairs ok
            puts("Adv Own_Address_Type\n");
            //override public or random address
            if (LE_FEATURES_IS_SUPPORT(LL_PRIVACY) && (le_param.resolution_enable))
            {
                resolving_list_t = 
                    ll_resolving_list_match(le_param.adv_param.Peer_Address_Type,
                        le_param.adv_param.Peer_Address);
            }
            if ((resolving_list_t != NULL)
                && (__resolve_list_IRK_verify(resolving_list_t->resolving_list_param.Local_IRK) == TRUE))
            {
				puts("set_local_RPA1\n");
                __set_ll_local_resolvable_private_addr(link->local.addr, resolving_list_t);
            }
            break; 
        default:
            return;
    }

    __ble_ops->ioctrl(link->hw, BLE_SET_LOCAL_ADDR,
            link->local.addr_type,
            link->local.addr); 
}

static void __set_ll_adv_local_addr(struct le_link *link)
{
	struct ble_adv *adv = &link->adv;

    link->local.addr_type = le_param.adv_param.Own_Address_Type;

    switch (le_param.adv_param.Own_Address_Type)
    {
        case 0:
        case 2:
			__set_ll_public_device_addr(link->local.addr);
#ifdef BLE_BQB_PROCESS_EN
			{
				u8 adv_addr[] = {0x3E, 0x3A, 0xBA, 0x98, 0x22, 0x71};
				memcpy(link->local.addr, adv_addr, 6);
			}
#endif			
			break; 
        case 1:
        case 3:
           __set_ll_random_device_addr(link->local.addr);
           break; 
    }

    __ble_ops->ioctrl(link->hw, BLE_SET_LOCAL_ADDR,
            link->local.addr_type,
            link->local.addr); 

    if (LE_FEATURES_IS_SUPPORT(LL_PRIVACY) && (le_param.resolution_enable))
    {
		puts("set_local_RPA0\n");
        __set_ll_adv_local_RPA(link);
    }
}



static void __set_ll_adv_peer_addr(struct le_link *link)
{
	struct ble_adv *adv = &link->adv;

    ASSERT((adv->adv_type == ADV_DIRECT_IND), "%s\n", __func__);

    link->peer.addr_type= le_param.adv_param.Peer_Address_Type;

    memcpy(link->peer.addr, le_param.adv_param.Peer_Address, 6);
	puts("adv_peer_addr=");
	printf_buf(link->peer.addr, 6);


    /* If Own_Address_Type equals 0x02 or 0x03, the Controller generates the */
    /* peer’s Resolvable Private Address using the peer’s IRK corresponding to the */
    /* peer’s Identity Address contained in the Peer_Address parameter and peer’s */
    /* Identity Address Type (i.e. 0x00 or 0x01) contained in the Peer_Address_Type */
    /* parameter. */
    struct resolving_list *resolving_list_t = NULL;

    if (LE_FEATURES_IS_SUPPORT(LL_PRIVACY) && (le_param.resolution_enable))
    {
        resolving_list_t = 
            ll_resolving_list_match(le_param.adv_param.Peer_Address_Type,
                le_param.adv_param.Peer_Address);
    }
    //override public or random address
    switch(le_param.adv_param.Own_Address_Type)
    {
        case 2:
        case 3:
            //pairs ok
			puts("own_addr_type\n");
            if ((resolving_list_t != NULL)
                && (__resolve_list_IRK_verify(resolving_list_t->resolving_list_param.Peer_IRK) == TRUE))
            {
                __set_ll_peer_resolvable_private_addr(link->peer.addr, resolving_list_t);
				puts("Privacy_peer_addr=");
				printf_buf(link->peer.addr, 6);
            }
			link->peer.addr_type = le_param.adv_param.Own_Address_Type; 
            break; 
    }

    __ble_ops->ioctrl(link->hw, BLE_SET_PEER_ADDR,
            link->peer.addr_type,
            link->peer.addr); 

    //specify Peer addr
    __ble_ops->ioctrl(link->hw, BLE_WHITE_LIST_ADDR,
            link->peer.addr_type,
            link->peer.addr); 
}


static void __set_ll_scan_local_addr(struct le_link *link) 
{
    link->local.addr_type = le_param.scan_param.Own_Address_Type;

    switch(le_param.scan_param.Own_Address_Type)
    {
        case 0:
        case 2:
           __set_ll_public_device_addr(link->local.addr);
#ifdef BLE_BQB_PROCESS_EN
		   {
				u8 scan_addr[] = {0x3E, 0x3A, 0xBA, 0x98, 0x11, 0x71};
				memcpy(link->local.addr, scan_addr, 6);
		   }
#endif		   
           break; 
        case 1:
        case 3:
           __set_ll_random_device_addr(link->local.addr);
           break; 
    }

    __ble_ops->ioctrl(link->hw, BLE_SET_LOCAL_ADDR,
            link->local.addr_type,
            link->local.addr); 
}

static void __set_ll_scan_local_RPA(struct le_link *link,
        struct resolving_list *resolving_list_t)
{
    //override public or random address
    switch(le_param.scan_param.Own_Address_Type)
    {
        case 2:
        case 3:
           //pairs ok
           if ((resolving_list_t != NULL)
                && (__resolve_list_IRK_verify(resolving_list_t->resolving_list_param.Local_IRK) == TRUE))
           {
                __set_ll_local_resolvable_private_addr(link->local.addr, resolving_list_t);
           }
           break; 
    }

    __ble_ops->ioctrl(link->hw, BLE_SET_LOCAL_ADDR,
            link->local.addr_type,
            link->local.addr); 
}


static void __set_ll_init_local_addr(struct le_link *link)
{
    //TO*DO
    link->local.addr_type = le_param.conn_param.own_address_type;

    switch(le_param.conn_param.own_address_type)
    {
        case 0:
        case 2:
           __set_ll_public_device_addr(link->local.addr);
#ifdef BLE_BQB_PROCESS_EN
		   {
				u8 init_addr[] = {0x3E, 0x3A, 0xBA, 0x98, 0x11, 0x71};
				memcpy(link->local.addr, init_addr, 6);
		   }
#endif
           break; 
        case 1:
        case 3:
           __set_ll_random_device_addr(link->local.addr);
           break; 
    }

    //RX InitA
    __ble_ops->ioctrl(link->hw, BLE_SET_LOCAL_ADDR,
            link->local.addr_type,
            link->local.addr); 
}

static void __set_ll_init_peer_addr(struct le_link *link)
{
	printf("--func=%s\n", __FUNCTION__);
    link->peer.addr_type = le_param.conn_param.peer_address_type;

    memcpy(link->peer.addr, le_param.conn_param.peer_address, 6);
	printf_buf(link->peer.addr, 6);

    /* puts(__func__); */
    /* printf_buf(link->peer.addr, 6); */

    __ble_ops->ioctrl(link->hw, BLE_SET_PEER_ADDR,
            link->peer.addr_type,
            link->peer.addr); 

    //specify Peer addr
    struct ble_conn *conn = &link->conn;
    struct white_list *white_list_t = NULL;
	if(conn->filter_policy == 0)
	{
		__ble_ops->ioctrl(link->hw, BLE_WHITE_LIST_ADDR,
				link->peer.addr_type,
				link->peer.addr); 
	}
	else
	{
		white_list_t = ll_white_list_match(link,
            link->peer.addr_type,
            link->peer.addr); 
		if(white_list_t != NULL)
		{
			__ble_ops->ioctrl(link->hw, BLE_WHITE_LIST_ADDR,
					link->peer.addr_type,
					link->peer.addr); 
		}
		else
		{
			struct white_list *p;

			list_for_each_entry(p, &le_param.white_list_head, entry)
			{
				__ble_ops->ioctrl(link->hw, BLE_WHITE_LIST_ADDR,
						p->white_list_param.Address_Type,
						p->white_list_param.Address);
				break;
			}
		}
	}
}


static void __set_ll_init_local_RPA(struct le_link *link, struct ble_rx *rx,
        struct resolving_list *resolving_list_t)
{
    struct ble_conn *conn = &link->conn;

    switch(le_param.conn_param.own_address_type)
    {
        case 2:
        case 3:
           //pairs ok
           if ((resolving_list_t != NULL)
                && (__resolve_list_IRK_verify(resolving_list_t->resolving_list_param.Local_IRK) == TRUE))
           {
               __set_ll_local_resolvable_private_addr(link->local.addr, resolving_list_t);
           }
           break; 
    }

    //TX InitA
    __ble_ops->ioctrl(link->hw, BLE_SET_LOCAL_ADDR,
            rx->rxadd,     
            rx->data + 6); 

    //rx peer InitA(#hw fixed#)
    __ble_ops->ioctrl(link->hw, BLE_SET_LOCAL_ADDR_RAM,
            link->local.addr_type,
            link->local.addr); 
}

static void ll_read_peer_resolvable_address(const u8 addr_type, const u8 *addr, u8 *peer_PRA)
{
    struct resolving_list *resolving_list_t;

    resolving_list_t = ll_resolving_list_match(addr_type, addr);

    if (resolving_list_t == NULL)
    {
        //Unknow Connection Identifier
        puts(__func__); 
        return; 
    }
    
    memcpy(peer_PRA, resolving_list_t->peer_PRA, 6);
}

static void ll_read_local_resolvable_address(const u8 addr_type, const u8 *addr, u8 *local_PRA)
{
    struct resolving_list *resolving_list_t;

    resolving_list_t = ll_resolving_list_match(addr_type, addr);

    if (resolving_list_t == NULL)
    {
        //Unknow Connection Identifier
        puts(__func__); 
        return; 
    }

    memcpy(local_PRA, resolving_list_t->local_PRA, 6);
}

static struct resolving_list *ll_resolve_peer_addr(const u8 *addr)
{
    struct resolving_list *p;

    u8 hash[3];

    //Address is RPA
	puts("conn_receive_adv_addr\n");
	printf_buf(addr, 6);
    if (__ll_resolvable_private_addr_verify(addr) == FALSE)
    {
        puts("ll resolve peer addr format error\n");
        return NULL;
    }

	list_for_each_entry(p, &le_param.resolving_list_head, entry)
    {
        irk_enc(p->resolving_list_param.Peer_IRK, addr + 3, hash);

        //localHash match peer hash
        if (!memcmp(hash, addr, 3)) 
        {
            return p;
        }
    }
    //try all list
    puts("RPA resolved fail\n");
    return NULL;
}

static struct resolving_list *ll_resolve_local_addr(const u8 *addr)
{
    struct resolving_list *p;

    u8 hash[3];

    //Address is RPA
    if (__ll_resolvable_private_addr_verify(addr) == FALSE)
    {
        puts("ll resolve local addr format error\n");
        return NULL;
    }

	list_for_each_entry(p, &le_param.resolving_list_head, entry)
    {
        irk_enc(p->resolving_list_param.Local_IRK, addr + 3, hash);

        //localHash match peer hash
        if (!memcmp(hash, addr, 3)) 
        {
            return p;
        }
    }
    //try all list
    return NULL;
}


static void ll_RPA_timeout_timer_handler(struct sys_timer *timer)
{
    /* struct le_link *link = sys_timer_get_user(timer); */
    //for test 
    struct le_link *link = ll_link_for_handle(0x1);
   
    ASSERT(link != NULL, "%s\n", __func__);
    puts("-----LL RPA Timeout\n");

    __random48_update();
    /* TO*DO */
    switch (link->state)    
    {
    case LL_ADVERTISING:
        __set_ll_adv_local_addr(link);
        break;
    case LL_SCANNING:
    case LL_INITIATING:
        break;
    default:
        break;
    }
    sys_timer_remove(&le_privacy.RPA_timeout);

    //Periodic Timer
    sys_timer_register(&le_privacy.RPA_timeout, le_privacy.RPA_timeout_cnt, ll_RPA_timeout_timer_handler);
    sys_timer_set_user(&le_privacy.RPA_timeout, link);
}

static void ll_RPA_timeout_start(u32 timeout)
{
    //set ll RPA timeout
    printf("RPA timeout %x\n", timeout);
    sys_timer_register(&le_privacy.RPA_timeout, timeout, ll_RPA_timeout_timer_handler);
    /* sys_timer_set_user(&le_privacy.RPA_timeout, link); */
}

static void ll_RPA_timeout_stop(void)
{
    sys_timer_remove(&le_privacy.RPA_timeout);
}

static void ll_set_resolvable_private_address_timeout(const u8 *data)
{
    struct le_link *link = sys_timer_get_user(&le_privacy.RPA_timeout);
   
    //remove already exsit
    if (link != NULL);
    {
        sys_timer_remove(&le_privacy.RPA_timeout);
    }

    le_privacy.RPA_timeout_cnt = READ_BT16(data, 0);
    le_privacy.RPA_timeout_cnt = le_privacy.RPA_timeout_cnt*1000; //1sec 

    ll_RPA_timeout_start(le_privacy.RPA_timeout_cnt);
}

static void ll_resolution_enable(void)
{
    puts(__func__);

    if (le_privacy.RPA_timeout_cnt == 0)
    {
        le_privacy.RPA_timeout_cnt = 15*60*1000;
    }

    __random48_update();
    //default 
    ll_RPA_timeout_start(le_privacy.RPA_timeout_cnt);   //update every 15mins
}

/********************************************************************************/

/********************************************************************************/
/*
 *                   DATA Length
 */
#define LL_TXMAXOCTETS_IS_EXCEED(x)     (x > le_data_length.priv->supportedMaxTxOctets)
#define LL_RXMAXOCTETS_IS_EXCEED(x)     (x > le_data_length.priv->supportedMaxRxOctets)
#define LL_TXMAXTIME_IS_EXCEED(x)       (x > le_data_length.priv->supportedMaxTxTime)
#define LL_RXMAXTIME_IS_EXCEED(x)       (x > le_data_length.priv->supportedMaxRxTime)

static void __set_ll_effective_data_args(struct le_link *link)
{
    //the lesser of connMax and connRemoteMax
    link->pdu_len.connEffectiveMaxTxOctets = 
        (link->pdu_len.connMaxTxOctets > link->pdu_len.connRemoteMaxRxOctets) ?
        link->pdu_len.connRemoteMaxRxOctets : link->pdu_len.connMaxTxOctets;
    __ble_ops->ioctrl(link->hw, BLE_SET_TX_LENGTH, link->pdu_len.connEffectiveMaxTxOctets);

    link->pdu_len.connEffectiveMaxRxOctets = 
        (link->pdu_len.connMaxRxOctets > link->pdu_len.connRemoteMaxTxOctets) ?
        link->pdu_len.connRemoteMaxTxOctets : link->pdu_len.connMaxRxOctets;
    __ble_ops->ioctrl(link->hw, BLE_SET_RX_LENGTH, link->pdu_len.connEffectiveMaxRxOctets);

    link->pdu_len.connEffectiveMaxTxTime = 
        (link->pdu_len.connMaxTxTime > link->pdu_len.connRemoteMaxRxTime) ?
        link->pdu_len.connRemoteMaxRxTime : link->pdu_len.connMaxTxTime;

    link->pdu_len.connEffectiveMaxRxTime = 
        (link->pdu_len.connMaxRxTime > link->pdu_len.connRemoteMaxTxTime) ?
        link->pdu_len.connRemoteMaxTxTime : link->pdu_len.connMaxRxTime;
}

static void __ll_set_data_length_auto(struct le_link *link)
{
    if (LE_FEATURES_IS_SUPPORT(LE_DATA_PACKET_LENGTH_EXTENSION))
    {
        if ((link->pdu_len.connMaxTxOctets != BLE_HW_MIN_TX_OCTETES) 
            || (link->pdu_len.connMaxRxOctets != BLE_HW_MIN_RX_OCTETES))
        {
            puts("issuse Data Length Update Procedure 1\n");
            ll_send_control_data(link, LL_LENGTH_REQ,
                    "2222", 
                    link->pdu_len.connMaxRxOctets,
                    link->pdu_len.connMaxRxTime,
                    link->pdu_len.connMaxTxOctets,
                    link->pdu_len.connMaxTxTime);
        }

        if ((link->pdu_len.connMaxTxTime != 328)
            || (link->pdu_len.connMaxRxTime != 328))
        {
            puts("issuse Data Length Update Procedure 2\n");

            ll_send_control_data(link, LL_LENGTH_REQ,
                    "2222", 
                    link->pdu_len.connMaxRxOctets,
                    link->pdu_len.connMaxRxTime,
                    link->pdu_len.connMaxTxOctets,
                    link->pdu_len.connMaxTxTime);
        }
    }
}
//Vol 6 Part B 4.5.10
static void __set_ll_data_length(struct le_link *link)
{
    //Vol 6 Part B 4.5.10
    le_data_length.connInitialMaxTxOctets = 
        (le_data_length.suggestedMaxTxOctets) ? 
        /* le_data_length.suggestedMaxTxOctets : 27; */
        le_data_length.suggestedMaxTxOctets : le_data_length.priv->supportedMaxTxOctets;

    if (LL_TXMAXOCTETS_IS_EXCEED(le_data_length.connInitialMaxTxOctets))
    {
        le_data_length.connInitialMaxTxOctets = le_data_length.priv->supportedMaxTxOctets;
    }
    /* printf("connInitialMaxTxOctets : %x\n", le_data_length.connInitialMaxTxOctets ); */

    le_data_length.connInitialMaxTxTime = 
        (le_data_length.suggestedMaxTxTime) ?
        /* le_data_length.suggestedMaxTxTime : 328; */
        le_data_length.suggestedMaxTxTime : le_data_length.priv->supportedMaxTxTime;

    if (LL_TXMAXTIME_IS_EXCEED(le_data_length.connInitialMaxTxTime))
    {
        le_data_length.connInitialMaxTxTime = le_data_length.priv->supportedMaxTxTime;
    }

    //local
    link->pdu_len.connMaxTxOctets = le_data_length.connInitialMaxTxOctets; //should be 27
    /* link->pdu_len.connMaxRxOctets = 27; //chosen by the Controller */
    link->pdu_len.connMaxRxOctets = le_data_length.priv->supportedMaxRxOctets; //chosen by the Controller

    link->pdu_len.connMaxTxTime = le_data_length.connInitialMaxTxTime;
    /* link->pdu_len.connMaxRxTime = 328; */
    link->pdu_len.connMaxRxTime = le_data_length.priv->supportedMaxRxTime;

    //connRemoteMaxTxOctets & connRemoteMaxRxOctets shall be 27
    link->pdu_len.connRemoteMaxTxOctets = BLE_HW_MIN_TX_OCTETES;
    link->pdu_len.connRemoteMaxRxOctets = BLE_HW_MIN_RX_OCTETES;
    //connRemoteMaxTxTime & connRemoteMaxRxTime shall be 328
    link->pdu_len.connRemoteMaxTxTime = 328;
    link->pdu_len.connRemoteMaxRxTime = 328;

    __set_ll_effective_data_args(link);

    /* __event_oneshot_add(link, __ll_set_data_length_auto, 10); */
}


static void __ll_update_connMaxTxOctets(struct le_link *link, u16 octets)
{
    if (LE_FEATURES_IS_SUPPORT(LE_DATA_PACKET_LENGTH_EXTENSION))
    {
        if (LL_TXMAXOCTETS_IS_EXCEED(octets)){
            puts("LL_TXMAXOCTETS_IS_EXCEED\n");
            return;
        }
        link->pdu_len.connMaxTxOctets = octets;
    }
}

static void __ll_update_connMaxTxTime(struct le_link *link, u16 time)
{
    if (LE_FEATURES_IS_SUPPORT(LE_DATA_PACKET_LENGTH_EXTENSION))
    {
        if (LL_TXMAXTIME_IS_EXCEED(time)){
            puts("LL_TXMAXTIME_IS_EXCEED\n");
            return;
        }
        link->pdu_len.connMaxTxTime = time;
    }
}

static void __ll_update_connMaxRxOctets(struct le_link *link, u16 octets)
{
    if (LE_FEATURES_IS_SUPPORT(LE_DATA_PACKET_LENGTH_EXTENSION))
    {
        if (LL_RXMAXOCTETS_IS_EXCEED(octets)){
            puts("LL_RXMAXOCTETS_IS_EXCEED\n");
            return;
        }
        link->pdu_len.connMaxRxOctets = octets;
    }
}

static void __ll_update_connMaxRxTime(struct le_link *link, u16 time)
{
    if (LE_FEATURES_IS_SUPPORT(LE_DATA_PACKET_LENGTH_EXTENSION))
    {
        if (LL_RXMAXTIME_IS_EXCEED(time)){
            puts("LL_RXMAXTIME_IS_EXCEED\n");
            return;
        }
        link->pdu_len.connMaxRxTime = time;
    }
}

enum{
    LL_UPDATE_TXOCTETS = 0,
    LL_UPDATE_TXTIMES,
    LL_UPDATE_RXOCTETS,
    LL_UPDATE_RXTIMES,
};

/*
 *      change by Host or Controller
 */
static const ll_step_extend data_length_update_steps[];

static void __ll_change_data_length_args(struct le_link *link, u8 type, u16 args)
{
    //spec 4.2 Vol 6,Part B,4.5.10
    switch(type) 
    {
        case LL_UPDATE_TXOCTETS:       
            __ll_update_connMaxTxOctets(link, args);
            break;
        case LL_UPDATE_TXTIMES:       
            __ll_update_connMaxTxTime(link, args);
            break;
        case LL_UPDATE_RXOCTETS:       
            __ll_update_connMaxRxOctets(link, args);
            break;
        case LL_UPDATE_RXTIMES:       
            __ll_update_connMaxRxTime(link, args);
            break;
        default:
            ASSERT(0, "%s\n", __func__);
            break;
    }

    //The Controller may change values of connMaxTxOctets... at any time after
    //entering the Connection State. Whenever it dose so,it shall communicate 
    //these values to the peer device using the Data Lenght Update Procedure.
    ll_send_control_data(link, LL_LENGTH_REQ, "2222", 
            link->pdu_len.connMaxRxOctets,
            link->pdu_len.connMaxRxTime,
            link->pdu_len.connMaxTxOctets,
            link->pdu_len.connMaxTxTime);
}
/********************************************************************************/
enum{
    LL_ADV_IND = 0,
    LL_ADV_DIRECT_IND_HIGH,
    LL_ADV_SCAN_IND,
    LL_ADV_NONCONN_IND,
    LL_ADV_DIRECT_IND_LOW,
};



static void __le_connection_complete_event(struct le_link *link, u8 status);

static void ll_adv_timeout_handler(struct sys_timer *timer)
{
    struct le_link *link = sys_timer_get_user(timer);
   
    ASSERT(link != NULL, "%s\n", __func__);

    ll_puts("LL Adv High Duty Timeout\n");

    //set slave
    link->role = 1,

    __le_connection_complete_event(link, DIRECTED_ADVERTISING_TIMEOUT);

    sys_timer_remove(&link->adv_timeout);

    //exit adv state & close link
    __ble_ops->close(link->hw);
    ll.handle &= ~(link->handle);
    __free_link(link);


    if (LE_FEATURES_IS_SUPPORT(LL_PRIVACY) && (le_param.resolution_enable))
    {
        ll_RPA_timeout_stop();
    }

}

static void ll_adv_timeout_start(struct le_link *link, u32 timeout)
{
    //set ll adv hight timeout
    sys_timer_register(&link->adv_timeout, timeout, ll_adv_timeout_handler);
    sys_timer_set_user(&link->adv_timeout, link);
}

static void ll_adv_timeout_stop(struct le_link *link)
{
    sys_timer_remove(&link->adv_timeout);
}

static void __set_ll_adv_state(struct le_link *link)
{
	struct ble_adv *adv = &link->adv;

    u8 i; 

#ifdef BLE_PRIVACY_EN
	g_privacy_flag = 1;
#endif

    adv->channel_map = le_param.adv_param.Advertising_Channel_Map;
    for (i = 0, adv->pkt_cnt = 0; i < 3; i++)
    {
        if (adv->channel_map & BIT(i))
        {
            adv->pkt_cnt++;
        }
    }

    u16 min,max;
    /* Maximum advertising interval for undirected and low duty cycle */
    /* directed advertising. */
    /* Range: 0x0020 to 0x4000 */
    /* Default: N = 0x0800 (1.28 seconds) */
    /* Time = N * 0.625 msec */
    /* Time Range: 20 ms to 10.24 sec */
    min = le_param.adv_param.Advertising_Interval_Max;
    max = le_param.adv_param.Advertising_Interval_Min;

    adv->filter_policy = le_param.adv_param.Advertising_Filter_Policy;

    switch(le_param.adv_param.Advertising_Type)
    {
        case LL_ADV_IND:
            adv->interval = min;
            adv->pdu_interval = 20;
            adv->adv_type = ADV_IND;
            break;
        case LL_ADV_DIRECT_IND_HIGH:   //exit 1.28s
            adv->interval = 6;
            adv->pdu_interval = 1;
            adv->adv_type = ADV_DIRECT_IND;
            //TO*DO timeout
            ll_adv_timeout_start(link, 1280);
            /* ll_adv_timeout_start(link, 21280); */
            /* The Advertising_Filter_Policy parameter shall be ignored when directed */
            /* advertising is enabled. */
            adv->filter_policy = 0;
            //only for ADV_DIRECT_IND
            __set_ll_adv_peer_addr(link);
            break;
        case LL_ADV_SCAN_IND:
            //not be set to less than 100ms
            min = (min < 0xA0) ? 0xA0 : min; 
            max = (max < 0xA0) ? 0xA0 : max; 
            adv->interval = min;
            adv->pdu_interval = 20;    //us
            adv->adv_type = ADV_SCAN_IND;
            break;
        case LL_ADV_NONCONN_IND:
            min = (min < 0xA0) ? 0xA0 : min; 
            max = (max < 0xA0) ? 0xA0 : max; 
            adv->interval = min;
            adv->pdu_interval = 20;    //us
            adv->adv_type = ADV_NONCONN_IND;
            break;
        case LL_ADV_DIRECT_IND_LOW:
            adv->interval = max;
            adv->pdu_interval = 10;    //us
            adv->adv_type = ADV_DIRECT_IND;
            /* The Advertising_Filter_Policy parameter shall be ignored when directed */
            /* advertising is enabled. */
            adv->filter_policy = 0;
            //only for ADV_DIRECT_IND
            __set_ll_adv_peer_addr(link);
            break;
    }

    ll_printf("adv type: %x\n", adv->adv_type);
    ll_printf("adv interval (N*625us): %x\n", adv->interval);
    ll_printf("adv pdu interval : %x\n", adv->pdu_interval);
    ll_printf("adv filter policy: %x\n", adv->filter_policy);

    __set_ll_adv_local_addr(link);

    adv->adv_len = le_param.adv_data.length;
    memset(adv->adv_data, 0x0, 31);
    memcpy(adv->adv_data, le_param.adv_data.data, adv->adv_len);

    adv->scan_rsp_len = le_param.scan_resp_data.length;
    memset(adv->scan_rsp_data, 0x0, 31);
    memcpy(adv->scan_rsp_data, le_param.scan_resp_data.data, adv->scan_rsp_len);


    __ble_ops->advertising(link->hw, adv);
} 



static void __set_ll_scan_state(struct le_link *link)
{
	struct ble_scan *scan = &link->scan;
#ifdef BLE_PRIVACY_EN
   	g_privacy_flag = 0;
#endif

    scan->type = le_param.scan_param.LE_Scan_Type;
    scan->interval = le_param.scan_param.LE_Scan_Interval;
    scan->window = le_param.scan_param.LE_Scan_Window;

    scan->filter_policy = le_param.scan_param.Scanning_Filter_Policy;
    ll_printf("scan active: %x\n", scan->type);
    ll_printf("scan interval (N*625us): %x\n", scan->interval);
    ll_printf("scan window: %x\n", scan->window);
    ll_printf("scan filter_policy: %x\n", scan->filter_policy);
    //not standard
    if (!LE_FEATURES_IS_SUPPORT(EXTENDED_SCANNER_FILTER_POLICIES))
    {
        if (scan->filter_policy == 2){
            scan->filter_policy = 0;
        }
        if (scan->filter_policy == 3){
            scan->filter_policy = 1;
        }
    }

    scan->filter_duplicates = le_param.scan_enable_param.filter_duplicates;
    //printf("filter_duplicates : %02x\n", scan->filter_duplicates);
    
    __set_ll_scan_local_addr(link);

    __ble_ops->scanning(link->hw, scan);
}


const static struct ble_conn_param sample_conn_param = {
    .aa         = {0xe8, 0x98, 0xeb, 0x6d},
    .crcinit    = {0xae, 0x83, 0x19},
    .winsize    = 0x2,
    .winoffset  = 0x0027,
    .latency    = 0x0000,
    .timeout    = 0x0032,
    .channel    = {0xff, 0xff, 0xff, 0xff, 0x1f},
    .hop        = 0x87,
};

#define LL_SLAVE_CONN_WINSIZE    800
#define LL_MASTER_CONN_WINSIZE   30

static void __set_ll_init_state(struct le_link *link)
{
    struct ble_conn *conn = &link->conn;
    struct ble_conn_param *conn_param = &link->conn.ll_data;

#ifdef BLE_PRIVACY_EN
   	g_privacy_flag = 0;
#endif

    conn->scan_interval = le_param.conn_param.le_scan_interval;
	/* printf("ll_init_state=0x%x\n", conn->scan_interval); */
	/* printf("ll_init_state=0x%x\n", le_param.conn_param.le_scan_interval); */
	printf_buf(le_param.conn_param.le_scan_interval, 6);
    conn->scan_windows = le_param.conn_param.le_scan_window;

    conn->filter_policy = le_param.conn_param.initiator_filter_policy;

    //InitA
    __set_ll_init_local_addr(link);

    //AdvA
    __set_ll_init_peer_addr(link);
    //LL DATA Part
#if 1
    __read_connection_param(conn_param, connection_data);
#else 
    memcpy(conn_param, &sample_conn_param, sizeof(struct ble_conn_param));
#endif
    conn_param->interval = le_param.conn_param.conn_interval_min;
    conn_param->latency = le_param.conn_param.conn_latency;
    conn_param->timeout = le_param.conn_param.supervision_timeout;

    conn_param->widening = LL_MASTER_CONN_WINSIZE>>1;
    
    __ble_ops->initiating(link->hw, conn);
}


static void ll_supervision_timeout_stop(struct le_link *link);
static void __set_link_state(struct le_link *link, int state)
{ 
	switch(state)
	{
    case LL_STANDBY:
        __ble_ops->standby(link->hw);
        break;
    case LL_ADVERTISING:
        __set_ll_adv_state(link);
        break;
    case LL_SCANNING:
        __set_ll_scan_state(link);
        break;
    case LL_INITIATING:
        __set_ll_init_state(link);
        break;
    case LL_CONNECTION_CREATE:
        __ble_ops->ioctrl(link->hw, BLE_SET_CONN_PARAM, &link->conn.ll_data);
        break;
    case LL_CONNECTION_ESTABLISHED:
        break;
    case LL_DISCONNECT:
        ll_supervision_timeout_stop(link);
        if (LE_FEATURES_IS_SUPPORT(LL_PRIVACY) && (le_param.resolution_enable))
        {
            ll_RPA_timeout_stop();
        }
        thread_resume(&ll.ll_thread);
        break;
	}

	link->state = state;
    /* printf("set link state : %02x\n", link->state); */
}

#define LE_IS_DISCONNECT(link)     (link->state == LL_DISCONNECT)

static void ll_disconnect_process(struct le_link *link, u8 reason)
{
    //skip already disconnect 
    if (LE_IS_DISCONNECT(link))
        return;

	struct ble_rx rx;

    __set_link_state(link, LL_DISCONNECT);

    rx.data[1] = reason;	

    __hci_event_emit(DISCONNECT_STEPS, link, &rx, reason);
}

#define LE_IS_CONNECT(link)     (link->state == LL_CONNECTION_ESTABLISHED)

// LL Supervision TimeOut
static void ll_conn_fast_supervision_timer_handler(struct le_link *link)
{
    ll_puts("\n********Fast Supervision_Timeout : CONNECTION_FAILED_TO_BE_ESTABLISHED***********\n");

    ll_disconnect_process(link, CONNECTION_FAILED_TO_BE_ESTABLISHED);
}

static void ll_conn_supervision_timer_handler(struct sys_timer *timer)
{
    struct le_link *link = (struct le_link*)sys_timer_get_user(timer);

    ASSERT(link != NULL, "%s\n", __func__);
    //DEBUG_IOB_1(0);
    //while(1);
    ll_puts("\n********Supervision_Timeout***********\n");

    u8 reason;

    reason = LE_IS_CONNECT(link) ? \
             CONNECTION_TIMEOUT : CONNECTION_FAILED_TO_BE_ESTABLISHED ;

    /* sys_timer_remove(&link->timeout); */

    ll_disconnect_process(link, reason);
}

static void ll_supervision_timeout_start(struct le_link *link, u32 timeout)
{
    //set ll connSupervision timeout
    sys_timer_register(&link->timeout, timeout, ll_conn_supervision_timer_handler);
    sys_timer_set_user(&link->timeout, link);
}

static void ll_supervision_timeout_stop(struct le_link *link)
{
    sys_timer_remove(&link->timeout);
}



static void __set_conn_winsize(struct le_link *link)
{
    int conn_winsize;

    /* DEBUG_IO_1(3); */
    if (link->role)
    {
        __ble_ops->ioctrl(link->hw, BLE_SET_WINSIZE, 50, LL_SLAVE_CONN_WINSIZE);
    }
    else //master
    {
		__ble_ops->ioctrl(link->hw, BLE_SET_WINSIZE, LL_MASTER_CONN_WINSIZE, 0);
    }
    /* DEBUG_IO_0(3); */
    /* puts("\n----widen----"); */
}


static void __connection_upadte(struct le_link *link)
{
    /* DEBUG_IO_1(4); */
	__ble_ops->ioctrl(link->hw, BLE_CONN_UPDATE, &link->conn.ll_data); 

    __rx_oneshot_add(link, __set_conn_winsize);

    //update supervision_timeout
    /* puts("\n----update---- : "); */

    ll_supervision_timeout_start(link, link->conn.ll_data.timeout*10);

    /* DEBUG_IO_0(4); */
}


static void __channel_map_upadte(struct le_link *link)
{
    __ble_ops->ioctrl(link->hw, BLE_CHANNEL_MAP, link->conn.ll_data.channel);
}

static bool __instant_link_lost(u16 instant, u16 eventcnt)
{
    u16 gap;

    /* printf("Instant : %04x - %04x\n", instant, eventcnt); */

    gap = instant - eventcnt;

    if ((gap < 32767) && (instant != eventcnt))
    {
        return FALSE;
    }
    else {
        return TRUE;
    }
}

/********************************************************************************/
/*
 *                  Link Layer Event 
 */
static void __le_connection_update_complete_event(struct le_link *link, struct ble_rx *rx, LL_CONTROL_CASE status)
{
    ll_puts("LE_CONNECTION_UPDATE_COMPLETE_EVENT\n");
    switch (status)
    {
    case LL_CONTROL_SUCCESS:
        status= 0;
        break;
    case LL_CONTROL_UNKNOW_RSP:
        status = UNSUPPORTED_REMOTE_FEATURE_UNSUPPORTED_LMP_FEATURE;
        break;
    case LL_CONTROL_REJECT:
        ASSERT(rx, "REJECT why no reason %s\n", __func__);
        status = rx->data[0];
        break;
    case LL_CONTROL_EXT_REJECT:
        ASSERT(rx, "EXT REJECT why no reason %s\n", __func__);
        status = rx->data[1];
        break;
    default:
        ASSERT(0, "%s\n", __func__);
        break;
    }

    __hci_emit_le_meta_event(LE_CONNECTION_UPDATE_COMPLETE_EVENT,
            "1H222", 
            status,
            link->handle,
            link->conn.ll_data.interval,
            link->conn.ll_data.latency,
            link->conn.ll_data.timeout);
}

static void __le_read_remote_used_features_complete_event(struct le_link *link, struct ble_rx *rx, LL_CONTROL_CASE status)
{
    ll_puts("LE_READ_REMOTE_USED_FEATURES_COMPLETE_EVENT\n");
    switch (status)
    {
    case LL_CONTROL_SUCCESS:
        status= 0;
        break;
    case LL_CONTROL_UNKNOW_RSP:
        status = UNSUPPORTED_REMOTE_FEATURE_UNSUPPORTED_LMP_FEATURE;
        break;
    case LL_CONTROL_REJECT:
    case LL_CONTROL_EXT_REJECT:
    default:
        ASSERT(0, "%s\n", __func__);
        break;
    }
    if (rx == NULL)
    {
        __hci_emit_le_meta_event(LE_READ_REMOTE_USED_FEATURES_COMPLETE_EVENT, "1Hc08", status,
                link->handle,
                0);
    }
    else {
        __hci_emit_le_meta_event(LE_READ_REMOTE_USED_FEATURES_COMPLETE_EVENT, "1Hc08", status,
                link->handle,
                rx->data[1]);
    }
}

static void __hci_read_remote_version_information_complete_event(struct le_link *link, struct ble_rx *rx, LL_CONTROL_CASE status)
{
    ll_puts("LE_READ_REMOTE_USED_FEATURES_COMPLETE_EVENT\n");
    switch (status)
    {
    case LL_CONTROL_SUCCESS:
        status = 0;
        break;
    case LL_CONTROL_TIMEOUT:
        status = LMP_RESPONSE_TIMEOUT_LL_RESPONSE_TIMEOUT;
        break;
    case LL_CONTROL_UNKNOW_RSP:
    case LL_CONTROL_REJECT:
    case LL_CONTROL_EXT_REJECT:
    default:
        ASSERT(0, "%s\n", __func__);
        break;
    }
    __hci_event_emit(VERSION_IND_STEPS, link, rx, status);
}

static bool __le_long_term_key_request_event(struct le_link *link, struct ble_rx *rx)
{
    ll_puts("LE_LONG_TERM_KEY_REQUEST_EVENT\n");
    return __hci_emit_le_meta_event(LE_LONG_TERM_KEY_REQUEST_EVENT,
        "Hc08c02",
        link->handle,
        rx->data+1,     //rand
        rx->data+1+8);  //ediv
}

static bool __le_remote_connection_parameter_request_event(struct le_link *link, struct ble_rx *rx) 
{
    u8 *data = &rx->data[1];

    ll_puts("LE_REMOTE_CONNECTION_PARAMETER_REQUEST_EVENT\n");

    return __hci_emit_le_meta_event(LE_REMOTE_CONNECTION_PARAMETER_REQUEST_EVENT, 
            "1H2222", 
            0,
            link->handle,
            READ_BT16(data, 0),
            READ_BT16(data, 2),
            READ_BT16(data, 4),
            READ_BT16(data, 6));
}

static void __le_enhanced_connection_complete_event(struct le_link *link, u8 status)
{
    u8 peer_RPA[6];
    u8 local_RPA[6];

    ll_puts("LE_ENHANCED_CONNECTION_COMPLETE_EVENT\n");
    ll_read_local_resolvable_address(link->peer.addr_type, 
            link->peer.addr, local_RPA);
    ll_read_peer_resolvable_address(link->peer.addr_type, 
            link->peer.addr, peer_RPA);

    __hci_emit_le_meta_event(LE_ENHANCED_CONNECTION_COMPLETE_EVENT,
            "1H11AAA2221", 
            status,
            link->handle,
            link->role,
            link->peer.addr_type,
            link->peer.addr,
            local_RPA,
            peer_RPA,
            link->conn.ll_data.interval,
            link->conn.ll_data.latency,
            link->conn.ll_data.timeout,
            link->master_clock_accuracy);
}

static void __le_connection_complete_event(struct le_link *link, u8 status)
{
    ll_puts("LE_CONNECTION_COMPLETE_EVENT\n");

    __hci_emit_le_meta_event(LE_CONNECTION_COMPLETE_EVENT,
            "1H11A2221", 
            status,
            link->handle,
            link->role,
            link->peer.addr_type,
            link->peer.addr,
            link->conn.ll_data.interval,
            link->conn.ll_data.latency,
            link->conn.ll_data.timeout,
            link->master_clock_accuracy);
}

static void __le_advertising_report_event(struct le_link *link, struct ble_rx *rx, u8 event_type)
{
	struct ble_scan *scan = &link->scan;

    if (scan->filter_duplicates == 1)
        return;

    ll_puts("LE_ADVERTISING_REPORT_EVENT\n");

    /* printf("event type %02x/ address type %02x/address \n",event_type, rx->txadd); */
    /* printf_buf(rx->data, 6); */
    /* printf("data length : %02x\n", rx->len - 6); */
    /* printf_buf(rx->data+6, rx->len - 6); */
    link->rssi = 0xff;

    __hci_emit_le_meta_event_static(LE_ADVERTISING_REPORT_EVENT,
            "111A1LB1", 
            1,
            event_type,         //event type
            rx->txadd,          //address type
            rx->data,           //address
            rx->len - 6,
            rx->len - 6,
            rx->data + 6,
            link->rssi);
}


static void __le_direct_advertising_report_event(struct le_link *link, struct ble_rx *rx)
{
	struct ble_scan *scan = &link->scan;

    if (scan->filter_duplicates == 1)
        return;

    ll_puts("LE_DIRECT_ADVERTISING_REPORT_EVENT\n");

    link->rssi = 0xff;
    __hci_emit_le_meta_event(LE_DIRECT_ADVERTISING_REPORT_EVENT,
            "111A1A1", 
            1,
            0x1,          //event type : ADV_DIRECT_IND
            rx->txadd,  //address type
            rx->data,   //address
            rx->txadd,  //address type
            rx->data,   //address
            link->rssi);
}

static void __le_data_length_change_event(struct le_link *link)
{
    ll_puts("LE_DATA_LENGTH_CHANGE_EVENT\n");


    __hci_emit_le_meta_event(LE_DATA_LENGTH_CHANGE_EVENT,
            "122222", 
            link->handle,
            link->pdu_len.connEffectiveMaxTxOctets,
            link->pdu_len.connEffectiveMaxTxTime,
            link->pdu_len.connEffectiveMaxRxOctets,
            link->pdu_len.connEffectiveMaxRxTime);
}

static void __le_disconnect_complete_event(struct le_link *link, LL_CONTROL_CASE status)
{
    switch (status)
    {
    case LL_CONTROL_SUCCESS:
        status = CONNECTION_TERMINATED_BY_LOCAL_HOST;
        break;
    case LL_CONTROL_TIMEOUT:
        status = LMP_RESPONSE_TIMEOUT_LL_RESPONSE_TIMEOUT;
        break;

    case LL_CONTROL_UNKNOW_RSP:
    case LL_CONTROL_REJECT:
    case LL_CONTROL_EXT_REJECT:
    default:
        ASSERT(0, "%s\n", __func__);
        break;
    }
    ll_disconnect_process(link, status);
}

static void __le_connection_complete_event_emit(struct le_link *link)
{
    if (LE_FEATURES_IS_SUPPORT(LL_PRIVACY) && (le_param.resolution_enable))
    {
        __le_enhanced_connection_complete_event(link, 0x0);
    }
    else
    {
        __le_connection_complete_event(link, 0x0);
    }
}

/*----------------------------------------------------------------------
 *
 * 							handler
 *
 * --------------------------------------------------------------------*/
enum{
    ADDR_PASS = 0,
    RESOLVE_ADDR_FAIL,
    DEVICE_FILTER_FAILED,
    ADVA_VERIFY_FAILED,
    RESOLVE_LIST_NOT_SAME_ENTRY,
};

#define ADDR_IS_FAIL()          (ll_error_code != ADDR_PASS)

static struct resolving_list *le_ll_adv_addr_process(struct le_link *link, struct ble_rx *rx)
{
    ASSERT(((rx->type == SCAN_REQ) || (rx->type == CONNECT_REQ)), "packet filter err %s\n", __func__);

    ll_error_code = ADDR_PASS;

    //privacy
    struct resolving_list *resolving_list_t = NULL;

    if (LE_FEATURES_IS_SUPPORT(LL_PRIVACY) && (le_param.resolution_enable))
    {
        //resolve ScanA or InitA
        resolving_list_t = ll_resolve_peer_addr(rx->data);

        if (resolving_list_t == NULL)
        {
            //resolve addr fail
            ll_error_code = RESOLVE_ADDR_FAIL;
            return NULL;
        }
    }

    //filter policy
    struct white_list *white_list_t = NULL;

    ll_error_code = ADDR_PASS;

	struct ble_adv *adv = &link->adv;

    if (adv->filter_policy & 0x1)
    {
        if (rx->type == SCAN_REQ)
        {
            /* puts("filter policy : SCAN_REQ\n"); */
            //check ScanA is in white list
            if (LE_FEATURES_IS_SUPPORT(LL_PRIVACY) && (le_param.resolution_enable))
            {
                white_list_t = ll_white_list_match(link,
                        resolving_list_t->resolving_list_param.Peer_Identity_Address_Type,
                        resolving_list_t->resolving_list_param.Peer_Identity_Address);
            }
            else{
                white_list_t = ll_white_list_match(link,
                        rx->txadd, rx->data);
            }

            if (white_list_t == NULL)    
            {
                ll_error_code = DEVICE_FILTER_FAILED;
            }
        }
    }
    if (adv->filter_policy & 0x2)
    {
        if (rx->type == CONNECT_REQ)
        {
            /* puts("filter policy : CONNECT_REQ\n"); */
            //check InitA is in white list
            if (LE_FEATURES_IS_SUPPORT(LL_PRIVACY) && (le_param.resolution_enable))
            {
                white_list_t = ll_white_list_match(link,
                        resolving_list_t->resolving_list_param.Peer_Identity_Address_Type,
                        resolving_list_t->resolving_list_param.Peer_Identity_Address);
            }
            else{
                white_list_t = ll_white_list_match(link,
                        rx->txadd, rx->data);
            }

            if (white_list_t == NULL)    
            {
                ll_error_code = DEVICE_FILTER_FAILED;
            }
        }
    }
    return resolving_list_t;
}

static void le_ll_scan_addr_process(struct le_link *link, struct ble_rx *rx)
{
    ASSERT(((rx->type != SCAN_REQ) && (rx->type != CONNECT_REQ)), "packet filter err %s\n", __func__);

	struct ble_scan *scan = &link->scan;
    
    ll_error_code = ADDR_PASS;

    //Passive Scan
    if (scan->type == 0x0)
    {
        //TO*DO
        return;
    }

    struct resolving_list *resolving_list_t = NULL;

    if (LE_FEATURES_IS_SUPPORT(LL_PRIVACY) && (le_param.resolution_enable))
    {
        //resolve AdvA
        resolving_list_t = ll_resolve_peer_addr(rx->data);

        if (resolving_list_t == NULL)
        {
            //resolve addr fail
            ll_error_code = RESOLVE_ADDR_FAIL;
            return;
        }
        //Generated ScanA
        __set_ll_scan_local_RPA(link, resolving_list_t);
    }

    //filter policy
    struct white_list *white_list_t = NULL;

    ll_error_code = ADDR_PASS;

    switch(scan->filter_policy)
    {
        case 0:
            break;
        case 1:
            //check AdvA is in white list
            if (LE_FEATURES_IS_SUPPORT(LL_PRIVACY) && (le_param.resolution_enable))
            {
                white_list_t = ll_white_list_match(link,
                        resolving_list_t->resolving_list_param.Peer_Identity_Address_Type,
                        resolving_list_t->resolving_list_param.Peer_Identity_Address);
            }
            else{
                white_list_t = ll_white_list_match(link,
                        rx->txadd, rx->data);
            }
            if (white_list_t == NULL)    
            {
                ll_error_code = DEVICE_FILTER_FAILED;
            }
            break;
        case 2:
            if (LE_FEATURES_IS_SUPPORT(EXTENDED_SCANNER_FILTER_POLICIES))
            {
            }else{
                ASSERT(0, "%s\n", __func__);
            }
            break;
        case 3:
            if (LE_FEATURES_IS_SUPPORT(EXTENDED_SCANNER_FILTER_POLICIES))
            {
                //check AdvA RPA is in white list
                if (LE_FEATURES_IS_SUPPORT(LL_PRIVACY) && (le_param.resolution_enable))
                {
                    white_list_t = ll_white_list_match(link,
                            resolving_list_t->resolving_list_param.Peer_Identity_Address_Type,
                            resolving_list_t->resolving_list_param.Peer_Identity_Address);
                }
                else{
                    white_list_t = ll_white_list_match(link,
                            rx->txadd, rx->data);
                }
                if (white_list_t == NULL)    
                {
                    ll_error_code = DEVICE_FILTER_FAILED;
                }
            }else{
                ASSERT(0, "%s\n", __func__);
            }
            break;
    }
}

static void le_ll_init_addr_process(struct le_link *link, struct ble_rx *rx)
{
    ASSERT(((rx->type != SCAN_REQ) && (rx->type != CONNECT_REQ)), "packet filter err %s\n", __func__);

    ll_error_code = ADDR_PASS;

    struct resolving_list *resolving_list_t = NULL;

    if (LE_FEATURES_IS_SUPPORT(LL_PRIVACY) && (le_param.resolution_enable))
    {
        //resolve AdvA
        /* if ((link->conn.peer_addr_type == 2) */
            /* || (link->conn.peer_addr_type == 3)) */
        {
            resolving_list_t = ll_resolve_peer_addr(rx->data);

            if (resolving_list_t == NULL)
            {
                //resolve AdvA fail
                ll_error_code = RESOLVE_ADDR_FAIL;
                return;
            }


            //resolve InitA
            /* if ((link->conn.own_addr_type == 2) */
                /* || (link->conn.own_addr_type == 3)) */
            {
                if (rx->type == ADV_DIRECT_IND)
                {
					/* putchar('f'); */
                    struct resolving_list *resolving_list_t1 = NULL;

                    //resolve InitA
                    resolving_list_t1 = ll_resolve_local_addr(rx->data + 6);

                    if (resolving_list_t1 == NULL)
                    {
                        //resolve InitA fail
                        ll_error_code = RESOLVE_ADDR_FAIL;
                        return;
                    }

                    if (resolving_list_t != resolving_list_t1)
                    {
                        //not the same entry
                        ll_error_code = RESOLVE_LIST_NOT_SAME_ENTRY;
                        return;
                    }
                }
            }
            //Generated local tx InitA
            __set_ll_init_local_RPA(link, rx, resolving_list_t);
        }
    }

    //filter policy
    struct white_list *white_list_t = NULL;

    ll_error_code = ADDR_PASS;

    struct ble_conn *conn = &link->conn;

    switch(conn->filter_policy)
    {
        case 0:
            break;
        case 1:
            //check AdvA is in white list
            if (LE_FEATURES_IS_SUPPORT(LL_PRIVACY) && (le_param.resolution_enable))
            {
                white_list_t = ll_white_list_match(link,
                        resolving_list_t->resolving_list_param.Peer_Identity_Address_Type,
                        resolving_list_t->resolving_list_param.Peer_Identity_Address);
            }
            else{
                white_list_t = ll_white_list_match(link,
                        rx->txadd, rx->data);
            }
            if (white_list_t == NULL)    
            {
                ll_error_code = DEVICE_FILTER_FAILED;
            }
            break;
    }

    return ;
}





static void slave_set_connection_param(struct le_link *link, struct ble_rx *rx)
{
	struct ble_conn *conn = &link->conn;

	link->role = 1;

    link->peer.addr_type = rx->txadd;
    memcpy(link->peer.addr, rx->data, 6);
	__read_connection_param(&conn->ll_data, rx->data+12);

    conn->ll_data.widening = LL_SLAVE_CONN_WINSIZE>>1;

    /* link->master_clock_accuracy = (rx->data)[34]; */
	__set_link_state(link, LL_CONNECTION_CREATE);

    __rx_oneshot_add(link, __set_conn_winsize);

    __le_connection_complete_event_emit(link);
    /* __rx_oneshot_add(link, __le_connection_complete_event_emit); */

}




static void rx_probe_adv_pdu_handler(struct le_link *link, struct ble_rx *rx)
{
    /* u32 supervison_timeout; */

	struct ble_adv *adv = &link->adv;

    switch (rx->type)
    {
        case SCAN_REQ:
            break;
        case CONNECT_REQ:
            putchar('g');
            ll_adv_timeout_stop(link);
            slave_set_connection_param(link, rx);
            //LL_CONNECTION_ESTABLISHED set ll connSupervision timeout 6*interval
            /* supervison_timeout = (link->conn.ll_data.interval*1250*6L)/1000; */
            /* [> put_u32hex(supervison_timeout); <] */
            /* ll_supervision_timeout_start(link, supervison_timeout); */
            __event_oneshot_add(link, ll_conn_fast_supervision_timer_handler, 6);
            break;
    }
}


static void rx_probe_scan_pdu_handler(struct le_link *link, struct ble_rx *rx)
{
    switch (rx->type)
    {
    case ADV_IND:
    case ADV_SCAN_IND:
        break;

    case SCAN_RSP:
    case ADV_NONCONN_IND:
    case ADV_DIRECT_IND:
        break;
    }
}

static void master_set_connection_param(struct le_link *link, struct ble_rx *rx)
{
    link->role = 0;

    __set_link_state(link, LL_CONNECTION_CREATE);

    __rx_oneshot_add(link, __set_conn_winsize);

    /* u16 instant = __ble_ops->get_conn_event(link->hw) + 1; */

    /* [> putchar('@'); <] */
    /* [> put_u16hex(instant); <] */
    /* __event_oneshot_add(link, __le_connection_complete_event_emit, instant); */
    __le_connection_complete_event_emit(link);
}


static void rx_probe_init_pdu_handler(struct le_link *link, struct ble_rx *rx)
{
    /* u32 supervison_timeout; */

    switch (rx->type)
    {
    case ADV_DIRECT_IND:
    case ADV_IND:
		if(LE_FEATURES_IS_SUPPORT(LL_PRIVACY))
		{
			if(!__ble_ops->is_init_enter_conn(link->hw))
			{
				return;
			}
		}
		/* putchar('A'); */
		master_set_connection_param(link, rx);
		//LL_CONNECTION_ESTABLISHED set ll connSupervision timeout
        //1.25ms + winsize + winoffset | 6 interval
		/* supervison_timeout = (1250+link->conn.ll_data.interval*1250*6L)/1000; */
		/* ll_supervision_timeout_start(link, supervison_timeout); */
        __event_oneshot_add(link, ll_conn_fast_supervision_timer_handler, 6);
        break;
    case ADV_NONCONN_IND:
    case ADV_SCAN_IND:
    case SCAN_RSP:
        break;
    }
}


static void le_ll_probe_pdu_handler(struct le_link *link, struct ble_rx *rx)
{
    /* printf("probe link state : %02x\n", link->state); */
    switch (link->state)
    {
        case LL_ADVERTISING:
            rx_probe_adv_pdu_handler(link, rx);
            break;
        case LL_SCANNING:
			/* putchar('S'); */
            rx_probe_scan_pdu_handler(link, rx);
            break;
        case LL_INITIATING:
            /* putchar('I'); */
            rx_probe_init_pdu_handler(link, rx);
            break;
    }
}



static void le_ll_probe_data_pdu_handler(struct le_link *link, struct ble_rx *rx)
{
    if (link->state == LL_CONNECTION_ESTABLISHED)
    {
        __event_oneshot_remove(link, ll_conn_fast_supervision_timer_handler, 6);
        /* putchar('#'); */
        //set ll supervisonTO to connSupervision timeout when receive first packet
        ll_supervision_timeout_start(link, link->conn.ll_data.timeout*10);
    }
    else if (link->state == LL_CONNECTION_CREATE)
    {
        putchar('%');
        /* put_u32hex(link->conn.ll_data.timeout*10); */
        ll_supervision_timeout_start(link, link->conn.ll_data.timeout*10);
        __set_link_state(link, LL_CONNECTION_ESTABLISHED);
    }

    __rx_oneshot_run(link);
}

static void le_ll_probe_data_pdu_crc_handler(struct le_link *link, struct ble_rx *rx)
{
    //CRC error emit connection event & change LL state
    if (link->state == LL_CONNECTION_CREATE)
    {
        putchar('@');
        /* put_u32hex(link->conn.ll_data.timeout*10); */
        __set_link_state(link, LL_CONNECTION_ESTABLISHED);
    }

    __rx_oneshot_run(link);
}

//emit ll command
static void le_ll_ctrl_pdu_handler(struct le_link *link, struct ble_rx *rx)
{
}

static void le_ll_unknow_pdu_handler(struct le_link *link, struct ble_rx *rx)
{
    //MIC error
    if (link->encrypt_enable)
    {
        __set_link_state(link, LL_DISCONNECT);
    }
}

static void ll_rx_probe_handler(void *priv, struct ble_rx *rx)
{
	struct le_link *link = (struct le_link *)priv;

    switch(rx->llid)
    {
        case LL_RESERVED:
			/* putchar('l'); */
            le_ll_probe_pdu_handler(link, rx);
            break;
        case LL_DATA_PDU_START:
        case LL_DATA_PDU_CONTINUE:
        case LL_DATA_PDU_SN:
        case LL_DATA_PDU_CRC:
            le_ll_probe_data_pdu_handler(link, rx);
            break;
        case LL_CONTROL_PDU:
            /* le_ll_ctrl_pdu_handler(link, rx); */
            break;
            /* le_ll_probe_data_pdu_crc_handler(link, rx); */
            /* break; */
    }
    
    //empty packet not notify upper layer
	if (RX_PACKET_IS_VALID(rx))
    {
        //resume ll thread
        thread_resume(&ll.ll_thread);
    }
}

static void debug_info(struct le_link *link)
{
    struct ble_conn *conn = &link->conn;

    /* printf("role is slave : %02x\n",   ROLE_IS_SLAVE(link)); */
    puts("\naa"); printf_buf(conn->ll_data.aa, 4);
    puts("\ncrcinit"); printf_buf(conn->ll_data.crcinit, 3);
    printf("winsize : %02x\n",    conn->ll_data.winsize);
    printf("winoffset : %04x\n",  conn->ll_data.winoffset);
    printf("interval: %04x\n",    conn->ll_data.interval);
    printf("latency : %04x\n",    conn->ll_data.latency);
    printf("timeout : %04x\n",    conn->ll_data.timeout);
    printf("widening : %04x\n",   conn->ll_data.widening);
    puts("channel map"); printf_buf(conn->ll_data.channel, 5);
    printf("hop: %02x\n",         conn->ll_data.hop);
}


static bool rx_adv_direct_verify(struct le_link *link, struct ble_rx *rx)
{
	struct ble_scan *scan = &link->scan;

    //InitA match ScanA
    if ((!memcmp(rx->data + 6, link->local.addr, 6)) 
        && (rx->rxadd == link->local.addr_type))
    {
        return TRUE;
    }

    return FALSE;
}


static void rx_adv_state_handler(struct le_link *link, struct ble_rx *rx)
{
    struct resolving_list *resolving_list_t = NULL;
    //advertising privacy & filter policy
    resolving_list_t = le_ll_adv_addr_process(link, rx);
    //ScanA
    if (ADDR_IS_FAIL())
    {
        puts("adv resolve fail\n");
        return;
    }
    //ScanA resolve 
    __ble_ops->ioctrl(link->hw, BLE_SET_RPA_RESOLVE_RESULT, rx, ADDR_IS_FAIL());
}

/* extern u8 conn_peer_addr[6];  */
u8 conn_peer_addr[6] = {0x71, 0x22, 0x98, 0xBA, 0x3A, 0x9E};
static void rx_scan_state_handler(struct le_link *link, struct ble_rx *rx)
{
    //AdvA
    le_ll_scan_addr_process(link, rx);
    if (ADDR_IS_FAIL())
    {
        puts("AdvA resolve fail\n");
        return;
    }
	/* putchar('c'); */
    //AdvA resolve 
    __ble_ops->ioctrl(link->hw, BLE_SET_RPA_RESOLVE_RESULT, rx, ADDR_IS_FAIL());
	/* puts(__FUNCTION__); */
	/* printf_buf(rx->data, 6); */
	memcpy(conn_peer_addr, rx->data, 6);

    switch(rx->type)
    {
    case ADV_IND:
        __le_advertising_report_event(link, rx, 0x0);
        break;
    case ADV_DIRECT_IND:
        if (LE_FEATURES_IS_SUPPORT(EXTENDED_SCANNER_FILTER_POLICIES))
        {
            struct ble_scan *scan = &link->scan;

            if ((scan->filter_policy == 0x2) 
                ||(scan->filter_policy == 0x3))
            {
                //InitA is RPA?
                if (__ll_resolvable_private_addr_verify(rx->data + 6) == TRUE)
                {
                    __le_direct_advertising_report_event(link, rx);
                    break;
                }
            }

        }
        //InitA match ScanA
        if (rx_adv_direct_verify(link, rx) == TRUE)
        {
            __le_advertising_report_event(link, rx, 0x1);
        }
        break;
    case ADV_SCAN_IND:
        __le_advertising_report_event(link, rx, 0x2);
        break;
    case ADV_NONCONN_IND:
        __le_advertising_report_event(link, rx, 0x3);
        break;
    case SCAN_RSP:
        __le_advertising_report_event(link, rx, 0x4);
        break;
    }
}

static void rx_init_state_handler(struct le_link *link,struct ble_rx *rx)
{
    u32 ll_timeout;

    //AdvA 
    le_ll_init_addr_process(link, rx);
    if (ADDR_IS_FAIL())
    {
        //TO*DO
        puts("AdvA resolve fail\n");
        return;
    }
    if (LE_FEATURES_IS_SUPPORT(LL_PRIVACY) && (le_param.resolution_enable))
	{
		int res;
		res = __ble_ops->is_init_enter_conn(link->hw);
		if(!res)
		{
			//AdvA resolve && InitA resolve
			__ble_ops->ioctrl(link->hw, BLE_SET_RPA_RESOLVE_RESULT, rx, ADDR_IS_FAIL());
			return;	
		}
	}

    switch (rx->type)
    {
    case ADV_IND:
    case ADV_DIRECT_IND:
        break;
    case ADV_SCAN_IND:
    case ADV_NONCONN_IND:
    case SCAN_RSP:
        break;
    }
}

static void rx_conn_state_handler(struct le_link *link, struct ble_rx *rx)
{
    u32 ll_timeout;

    if (ROLE_IS_SLAVE(link))
    {
        struct resolving_list *resolving_list_t = NULL;
        //advertising privacy & filter policy
        resolving_list_t = le_ll_adv_addr_process(link, rx);
        //InitA
        if (ADDR_IS_FAIL())
        {
            puts("InitA resolve fail\n");
            __set_link_state(link, LL_DISCONNECT);
            __set_link_state(link, LL_ADVERTISING);
            return;
        }

        //InitA resolve 
        __ble_ops->ioctrl(link->hw, BLE_SET_RPA_RESOLVE_RESULT, rx, ADDR_IS_FAIL());

        struct ble_adv *adv = &link->adv;
        //InitA
        if (adv->adv_type == ADV_IND)
        {
            link->peer.addr_type = rx->txadd;
            memcpy(link->peer.addr, rx->data, 6);
        }
    }
    else
    {
        //init privacy & filter policy
        le_ll_init_addr_process(link, rx);
        //AdvA 
        if (ADDR_IS_FAIL())
        {
            puts("AdvA resolve fail\n");
            return;
        }
    }


    debug_info(link);
}


static bool rx_pdu_handler(struct le_link *link, struct ble_rx *rx)
{
    /* printf_buf(rx, sizeof(*rx)+rx->len); */

    /* printf("post link state : %02x\n", link->state); */
    bool upper_pass = FALSE;

    switch (link->state)
    {
        case LL_ADVERTISING:
            rx_adv_state_handler(link, rx);
            break;
        case LL_SCANNING:
			/* putchar('a'); */
            rx_scan_state_handler(link, rx);
            break;
        case LL_INITIATING:
			/* putchar('d'); */
            rx_init_state_handler(link, rx);
            break;
        case LL_CONNECTION_CREATE:
			/* puts("LL_CONNECTION_CREATE\n"); */
            rx_conn_state_handler(link, rx);
            break;
    }

    return upper_pass;
}

static void rx_data_pdu_handler(struct le_link *link, struct ble_rx *rx)
{
}




/********************************************************************************/
/*
 *                      LL Control Procedure 
 */
static void ll_response_timeout_handler(struct sys_timer *timer)
{
    struct le_link *link = sys_timer_get_user(timer);

    ASSERT(link != NULL, "%s\n", __func__);

    ll_puts("\n********Response_Timeout***********\n");

    //emit LE Event
    ll_control_procedure_finish(link, NULL, LL_CONTROL_TIMEOUT);

    ll_disconnect_process(link, LMP_RESPONSE_TIMEOUT_LL_RESPONSE_TIMEOUT);
}

static void ll_response_timeout_start(struct le_link *link)
{
    //If the procedure response timeout timer reaches 40 seconds, the connection is considered lost
    sys_timer_register(&link->response_timeout, 40000, ll_response_timeout_handler);
    sys_timer_set_user(&link->response_timeout, link);
}

static void ll_response_timeout_stop(struct le_link *link)
{
    sys_timer_remove(&link->response_timeout);
}

struct connection_param_request{
    u16 interval_max;
    u16 interval_min;
    u16 latency;
    u16 timeout;
    u8  PreferredPeriodicity;
    u16 ReferenceConnEventCount;
    u16 offset[6];
};


static struct connection_param_request conn_param_req;
/*
 *      LL Control Procedure - Connection parameter request
 */
static void __ll_send_conn_param_req_auto(struct le_link *link)
{
    //don't change
    conn_param_req.latency = link->conn.ll_data.latency;
    conn_param_req.timeout = link->conn.ll_data.timeout;

    //depend on anchor move management
    conn_param_req.interval_min = link->conn.ll_data.interval;
    conn_param_req.interval_max = link->conn.ll_data.interval;
    conn_param_req.PreferredPeriodicity = 0x0;
    conn_param_req.ReferenceConnEventCount = __ble_ops->get_conn_event(link->hw) + 15; //15 default 

    memset(conn_param_req.offset, 0xff, ARRAY_SIZE(conn_param_req.offset));
    conn_param_req.offset[0] = 0x0003;     //depend on the achor point management, 0x3 is for example

    if (conn_param_req.timeout < 
            (2 * conn_param_req.interval_max * (conn_param_req.latency + 1)))
    {
        ASSERT(0, "Conn Param Request err TimeOut in %s\n", __func__);
    }

    //Master procedure record
    if (!ROLE_IS_SLAVE(link))
    {
        ll_control_procedure_push(LL_CONNECTION_PARAM_REQ);
    }

    ll_send_control_data(link, LL_CONNECTION_PARAM_REQ,
            "222212222222",
            conn_param_req.interval_min,
            conn_param_req.interval_max,
            conn_param_req.latency,
            conn_param_req.timeout,
            conn_param_req.PreferredPeriodicity,   //
            conn_param_req.ReferenceConnEventCount,
            conn_param_req.offset[0],
            conn_param_req.offset[1],
            conn_param_req.offset[2],
            conn_param_req.offset[3],
            conn_param_req.offset[4],
            conn_param_req.offset[5]);
}

static void __ll_send_conn_param_req(void)
{
    struct connection_update_parameter *param;

    param = le_param.conn_update_param;

    ASSERT(param != NULL, "%s\n", __func__);

    if (param->Supervision_Timeout < 
            (2 * param->Conn_Interval_Max * (param->Conn_Latency + 1)))
    {
        ASSERT(0, "Conn Param Request err TimeOut in %s\n", __func__);
    }

	struct le_link *link = ll_link_for_handle(param->connection_handle);

    ASSERT(link != NULL, "%s\n", __func__);

    //Master procedure record
    if (!ROLE_IS_SLAVE(link))
    {
        ll_control_procedure_push(LL_CONNECTION_PARAM_REQ);
    }

    conn_param_req.interval_min = param->Conn_Interval_Min;
    conn_param_req.interval_max = param->Conn_Interval_Max;
    conn_param_req.latency = param->Conn_Latency;
    conn_param_req.timeout = param->Supervision_Timeout;

    if (conn_param_req.interval_max == conn_param_req.interval_min)
    {
        conn_param_req.PreferredPeriodicity = 0x0;
    }
    else {
        conn_param_req.PreferredPeriodicity = conn_param_req.interval_max/2;
    }

    conn_param_req.ReferenceConnEventCount = __ble_ops->get_conn_event(link->hw) + 15; //15 default 

    memset(conn_param_req.offset, 0xff, ARRAY_SIZE(conn_param_req.offset));
    conn_param_req.offset[0] = 0x0003;     //depend on the achor point management, 0x3 is for example

    ll_send_control_data(link, LL_CONNECTION_PARAM_REQ,
            "222212222222",
            conn_param_req.interval_min,
            conn_param_req.interval_max,
            conn_param_req.latency,
            conn_param_req.timeout,
            conn_param_req.PreferredPeriodicity,   //
            conn_param_req.ReferenceConnEventCount,
            conn_param_req.offset[0],
            conn_param_req.offset[1],
            conn_param_req.offset[2],
            conn_param_req.offset[3],
            conn_param_req.offset[4],
            conn_param_req.offset[5]);

    //Master procedure record
    if (!ROLE_IS_SLAVE(link))
    {
        __hci_param_free(le_param.conn_update_param);
    }
}

typedef enum{
    CONN_PARAM_REQ_AUTO = 0,
    CONN_PARAM_REQ_HOST,
    CONN_PARAM_REQ_REJECT,
    CONN_PARAM_REQ_INVALID,
}CONN_PARAM_RES;


static CONN_PARAM_RES __ll_receive_connection_param_req(struct le_link *link, struct ble_rx *rx)
{
    CONN_PARAM_RES res;

    u8 *data = &rx->data[1];

    /* memset(conn_param_req , 0x0, sizeof(connection_param_request)); */

    //update conn param
    conn_param_req.interval_min = READ_BT16(data, 0);
    conn_param_req.interval_max = READ_BT16(data, 2);
    conn_param_req.latency  = READ_BT16(data, 4);
    conn_param_req.timeout  = READ_BT16(data, 6);

    conn_param_req.PreferredPeriodicity  = data[8];
    conn_param_req.ReferenceConnEventCount = READ_BT16(data, 9);

    u8 i;
    for (i = 0; i < 6; i++){
        conn_param_req.offset[i] = READ_BT16(data, 11 + 2*i);
    }

    if ((conn_param_req.latency == link->conn.ll_data.latency)
        && (conn_param_req.timeout == link->conn.ll_data.timeout))
    {
        res = CONN_PARAM_REQ_AUTO;
    }
    else{
        res = CONN_PARAM_REQ_HOST;
    }

    return res;
}

static void __slave_ll_send_conn_param_rsp_auto(struct le_link *link)
{
    //containing alternative parameters default use master parameters
    ll_send_control_data(link, LL_CONNECTION_PARAM_RSP,
            "222212222222",
            conn_param_req.interval_min,
            conn_param_req.interval_max,
            conn_param_req.latency,
            conn_param_req.timeout,
            conn_param_req.PreferredPeriodicity,   //
            conn_param_req.ReferenceConnEventCount,
            conn_param_req.offset[0],
            conn_param_req.offset[1],
            conn_param_req.offset[2],
            conn_param_req.offset[3],
            conn_param_req.offset[4],
            conn_param_req.offset[5]);
}


static void __master_ll_receive_connection_param_rsp(struct le_link *link, struct ble_rx *rx)
{
    //TO*DO
    u8 *data = &rx->data[1];

    conn_param_req.interval_min = READ_BT16(data, 0);
    conn_param_req.interval_max = READ_BT16(data, 2);
    conn_param_req.latency  = READ_BT16(data, 4);
    conn_param_req.timeout  = READ_BT16(data, 6);

    conn_param_req.PreferredPeriodicity  = data[8];
    conn_param_req.ReferenceConnEventCount = READ_BT16(data, 9);

    u8 i;
    for (i = 0; i < 6; i++){
        conn_param_req.offset[i] = READ_BT16(data, 11 + 2*i);
    }
}


/*
 *      LL Control Procedure - Connection Update
 */
static void __master_ll_send_conn_update_req_auto_done(struct le_link *link)
{
    __le_connection_update_complete_event(link, NULL, LL_CONTROL_SUCCESS);
}

static void __master_ll_send_conn_update_req_auto(struct le_link *link)
{
    //TO*DO
	/* struct le_link *link = ll_link_for_handle(param->connection_handle); */

    /* ASSERT(link != NULL, "%s\n", __func__); */

    u16 instant = __ble_ops->get_conn_event(link->hw) + 10;

    u16 interval_old = link->conn.ll_data.interval;

    //calc interval new by Preferredperiodicity * n
    if (conn_param_req.interval_max == conn_param_req.interval_min)
    {
        link->conn.ll_data.interval = conn_param_req.interval_min;
    }
    else 
    {
        u8 i;

        for (i = 1; i < 255; i++)
        {
            u16 interval;

            interval = conn_param_req.PreferredPeriodicity * i;
            if ((interval >= conn_param_req.interval_min) 
                && (interval <= conn_param_req.interval_max))
            {
                link->conn.ll_data.interval = interval;
                break;
            }
        }
    }
    u16 t;

    if (conn_param_req.ReferenceConnEventCount > instant)
    {
        /* Δt = ( (ReferenceConnEventCount – Instant)*connIntervalOLD + offset0 ) % connIntervalNEW */
        t = ((conn_param_req.ReferenceConnEventCount - instant) * interval_old + conn_param_req.offset[0]) % link->conn.ll_data.interval ; 
    }
    else
    {
        /* Δt = connIntervalNEW - ( (Instant - ReferenceConnEventCount )*connIntervalOLD - offset0 ) % connIntervalNEW */
        t = link->conn.ll_data.interval - ((instant - conn_param_req.ReferenceConnEventCount)*interval_old - conn_param_req.offset[0]) % link->conn.ll_data.interval;
    }

    //Option 1: the first packet sent after the instant by the master is inside the Transmit Window and 3.75 ms from the beginning of the Transmit Window.
    if (t > 3)
    {
        link->conn.ll_data.winoffset = t - 1;
        link->conn.ll_data.winsize = 2;
    }
    else 
    {
        // t = 3~0
        link->conn.ll_data.winoffset = 0;
        link->conn.ll_data.winsize = 4;
    }

    //assume not chanege respond autonomously
    /* link->conn.ll_data.latency = conn_param_req.latency; */
    /* link->conn.ll_data.timeout = conn_param_req.timeout; */

    ll_send_control_data(link, LL_CONNECTION_UPDATE_REQ, 
            "122222",
            link->conn.ll_data.winsize,
            link->conn.ll_data.winoffset,
            link->conn.ll_data.interval,
            link->conn.ll_data.latency,
            link->conn.ll_data.timeout,
            instant);

	__ble_ops->ioctrl(link->hw, BLE_SET_INSTANT, instant);

    __event_oneshot_add(link, __connection_upadte, instant - 1);

    __event_oneshot_add(link, __master_ll_send_conn_update_req_auto_done, instant);
}

static void __master_ll_send_conn_update_req_done(struct le_link *link)
{
    ll_control_procedure_pop(LL_CONNECTION_UPDATE_REQ);

    __le_connection_update_complete_event(link, NULL, LL_CONTROL_SUCCESS);
}

static void __master_ll_send_conn_update_req(void)
{
    struct connection_update_parameter *param;

    param = le_param.conn_update_param;

    ASSERT(param != NULL, "%s\n", __func__);

	struct le_link *link = ll_link_for_handle(param->connection_handle);

    ASSERT(link != NULL, "%s\n", __func__);

    u16 instant = __ble_ops->get_conn_event(link->hw) + 10;

    //avoid procedure collisions
    ll_control_procedure_push(LL_CONNECTION_UPDATE_REQ);

    __event_oneshot_add(link, __master_ll_send_conn_update_req_done, instant);

    if (master_ll_control_current_procedure == LL_CONNECTION_PARAM_REQ)
    {
        ll_control_procedure_pop(LL_CONNECTION_PARAM_REQ);

        u16 interval_old = link->conn.ll_data.interval;

        //calc interval new by Preferredperiodicity * n
        if (conn_param_req.interval_max == conn_param_req.interval_min)
        {
            link->conn.ll_data.interval = conn_param_req.interval_min;
        }
        else 
        {
            u8 i;

            for (i = 1; i < 255; i++)
            {
                u16 interval;

                interval = conn_param_req.PreferredPeriodicity * i;
                if ((interval >= conn_param_req.interval_min) 
                    && (interval <= conn_param_req.interval_max))
                {
                    link->conn.ll_data.interval = interval;
                    break;
                }
            }
        }
        u16 t;

        if (conn_param_req.ReferenceConnEventCount > instant)
        {
            /* Δt = ( (ReferenceConnEventCount – Instant)*connIntervalOLD + offset0 ) % connIntervalNEW */
            t = ((conn_param_req.ReferenceConnEventCount - instant) * interval_old + conn_param_req.offset[0]) % link->conn.ll_data.interval ; 
        }
        else
        {
            /* Δt = connIntervalNEW - ( (Instant - ReferenceConnEventCount )*connIntervalOLD - offset0 ) % connIntervalNEW */
            t = link->conn.ll_data.interval - ((instant - conn_param_req.ReferenceConnEventCount)*interval_old - conn_param_req.offset[0]) % link->conn.ll_data.interval;
        }

        //Option 1: the first packet sent after the instant by the master is inside the Transmit Window and 3.75 ms from the beginning of the Transmit Window.
        if (t > 3)
        {
            link->conn.ll_data.winoffset = t - 1;
            link->conn.ll_data.winsize = 2;
        }
        else 
        {
            // t = 3~0
            link->conn.ll_data.winoffset = 0;
            link->conn.ll_data.winsize = 4;
        }

        //chanege respond by Local Host
        link->conn.ll_data.latency = conn_param_req.latency;
        link->conn.ll_data.timeout = conn_param_req.timeout;
    }
    else{

        //update conn param
        link->conn.ll_data.latency  = param->Conn_Latency;
        link->conn.ll_data.interval = param->Conn_Interval_Min;
        link->conn.ll_data.timeout  = param->Supervision_Timeout;

        __hci_param_free(le_param.conn_update_param);
    }

    ll_send_control_data(link, LL_CONNECTION_UPDATE_REQ, 
            "122222",
            link->conn.ll_data.winsize,
            link->conn.ll_data.winoffset,
            link->conn.ll_data.interval,
            link->conn.ll_data.latency,
            link->conn.ll_data.timeout,
            instant);

	__ble_ops->ioctrl(link->hw, BLE_SET_INSTANT, instant);

    __event_oneshot_add(link, __connection_upadte, instant - 1);

}

static void __slave_ll_receive_conn_update_req_done(struct le_link *link)
{
    __le_connection_update_complete_event(link, NULL, LL_CONTROL_SUCCESS);
}

static void __slave_ll_receive_conn_update_req(struct le_link *link, struct ble_rx *rx)
{
    u8 *data = &rx->data[1];

	int instant = READ_BT16(data, 9);

    if (__instant_link_lost(instant, rx->event_count) == TRUE)
    {
        puts(__func__);
        ll_disconnect_process(link, INSTANT_PASSED);
        return;
    }

	struct ble_conn_param *conn_param = &link->conn.ll_data;

	conn_param->winsize   = data[0];
	conn_param->winoffset = READ_BT16(data, 1); 
	conn_param->interval  = READ_BT16(data, 3);
	conn_param->latency   = READ_BT16(data, 5);
	conn_param->timeout   = READ_BT16(data, 7);

    conn_param->widening = LL_SLAVE_CONN_WINSIZE>>1;

	__ble_ops->ioctrl(link->hw, BLE_SET_INSTANT, instant);

    __event_oneshot_add(link, __connection_upadte, instant - 1);

    __event_oneshot_add(link, __slave_ll_receive_conn_update_req_done, instant);
}

static const ll_step_extend slave_connection_parameter_request_steps[] = {
    LL_CONNECTION_PARAM_REQ,
    LL_CONNECTION_UPDATE_REQ|WAIT_RX,

    LL_DONE(SLAVE_CONNECTION_PARAMETER_REQUEST_STEPS),
};

static const ll_step_extend master_connection_parameter_request_steps[] = {
    LL_CONNECTION_PARAM_REQ,
    LL_CONNECTION_PARAM_RSP|WAIT_RX,
    LL_CONNECTION_UPDATE_REQ,

    LL_DONE(MASTER_CONNECTION_PARAMETER_REQUEST_STEPS),
};

static const ll_step_extend master_connection_update_steps[] = {
    LL_CONNECTION_UPDATE_REQ,

    LL_DONE(MASTER_CONNECTION_UPDATE_STEPS),
};

static void le_connection_update(
        const struct connection_update_parameter *param)
{
	struct le_link *link = ll_link_for_handle(param->connection_handle);

    ASSERT(link != NULL, "%s\n", __func__);
    
    if (LE_IS_CONNECT(link))
    {
        int malloc_len;

        malloc_len = sizeof(struct connection_update_parameter);
        //HCI parameter
        le_param.conn_update_param = __hci_param_malloc(malloc_len);

        memcpy(le_param.conn_update_param, param, malloc_len);
        
        if (LE_FEATURES_IS_SUPPORT(CONNECTION_PARAMETER_REQUEST))
        {
            if (ROLE_IS_SLAVE(link)){
                ll_control_data_step_start(slave_connection_parameter_request_steps);
            }
            else{
                ll_control_data_step_start(master_connection_parameter_request_steps);
            }
        }
        else
        {
            if (ROLE_IS_SLAVE(link)){
                ASSERT(0, "%s\n", __func__);
            }
            else{
                ll_control_data_step_start(master_connection_update_steps);
            }
        }
    }
    else
    {
        //error
        ASSERT(0, "%s\n", __func__);
    }
}

static void __slave_ll_send_conn_param_rsp(void)
{
    struct connection_update_parameter *param;

    param = le_param.conn_update_param;

    ASSERT(param != NULL, "%s\n", __func__);

	struct le_link *link = ll_link_for_handle(param->connection_handle);

    ASSERT(link != NULL, "%s\n", __func__);

    ll_send_control_data(link, LL_CONNECTION_PARAM_RSP,
            "222212222222",
            param->Conn_Interval_Min,
            param->Conn_Interval_Max,
            param->Conn_Latency,
            param->Supervision_Timeout,
            conn_param_req.PreferredPeriodicity,   //
            conn_param_req.ReferenceConnEventCount,
            conn_param_req.offset[0],
            conn_param_req.offset[1],
            conn_param_req.offset[2],
            conn_param_req.offset[3],
            conn_param_req.offset[4],
            conn_param_req.offset[5]);

    __hci_param_free(le_param.conn_update_param);
}


static const ll_step_extend slave_connection_parameter_respond_steps[] = {
    LL_CONNECTION_PARAM_RSP,
    LL_CONNECTION_UPDATE_REQ|WAIT_RX,

    LL_DONE(SLAVE_CONNECTION_PARAMETER_RESPOND_STEPS),
};

//remote_connection_parameter_request_reply_parameter re-use conn_update_param
static void le_connection_paramter_respond(
        const struct connection_update_parameter *param)
{
	struct le_link *link = ll_link_for_handle(param->connection_handle);

    ASSERT(link != NULL, "%s\n", __func__);

    int malloc_len;

    malloc_len = sizeof(struct connection_update_parameter);
    //for futrue used
    le_param.conn_update_param = __hci_param_malloc(malloc_len);
   
    memcpy(le_param.conn_update_param, param, malloc_len);
    
    if (ROLE_IS_SLAVE(link)){
        ll_control_data_step_start(slave_connection_parameter_respond_steps);
    }
    else{
        //re-use master conn update procedure
        ll_control_data_step_start(master_connection_update_steps);
    }
}

/*
 *      LL Control Procedure - Reject Ext
 */

static void __ll_send_reject_ind_ext_auto(struct le_link *link, u8 opcode, u8 reason)
{
    ll_send_control_data(link, LL_REJECT_IND_EXT, 
            "11", opcode, reason);
}

static void __ll_send_reject_ind_ext(
        const struct remote_connection_parameter_request_negative_reply_parameter *param)
{
    struct le_link *link = ll_link_for_handle(param->connection_handle);

    ASSERT(link != NULL, "%s\n", __func__);

    ll_send_control_data(link, LL_REJECT_IND_EXT, "11", link->last_opcode, param->reason);
}

static void __ll_receive_reject_ind_ext(struct le_link *link, struct ble_rx *rx)
{
    //TO*DO 
}

static const ll_step_extend slave_reject_ext_steps[] = {
    LL_REJECT_IND_EXT,

    LL_DONE(SLAVE_REJECT_EXT_STEPS),
};

static void le_reject_ext(
        const struct remote_connection_parameter_request_negative_reply_parameter *param)
{
    //no malloc
    __ll_send_reject_ind_ext(param);
}
/*
 *      LL Control Procedure - Channel Map Update
 */
static void __master_ll_send_channel_map_req_done(struct le_link *link)
{
    ll_control_procedure_pop(LL_CHANNEL_MAP_REQ);    
}

static void __master_ll_send_channel_map_req(void)
{
    struct le_link *link = ll_link_for_state(LL_CONNECTION_ESTABLISHED);

    ASSERT(link != NULL, "%s\n", __func__);

    struct host_channel_classification *param;
    
    param = le_param.channel_map_update_param;

    ASSERT(param != NULL, "%s\n", __func__);

    u16 instant = __ble_ops->get_conn_event(link->hw) + 10;

    ll_control_procedure_push(LL_CHANNEL_MAP_REQ);

    __event_oneshot_add(link, __master_ll_send_channel_map_req_done, instant);

    memcpy(link->conn.ll_data.channel, param->channel_map, 5);

    ll_send_control_data(link, LL_CHANNEL_MAP_REQ, "c052", 
            param->channel_map,
            instant);

    __hci_param_free(le_param.channel_map_update_param);

    __event_oneshot_add(link, __channel_map_upadte, instant - 1);

}

static void __slave_ll_receive_channel_map_req(struct le_link *link, struct ble_rx *rx)
{
    u8 *data = &rx->data[1];

	int instant = READ_BT16(data, 5);

    if (__instant_link_lost(instant, rx->event_count) == TRUE)
    {
        puts(__func__);
        ll_disconnect_process(link, INSTANT_PASSED);
        return;
    }

	struct ble_conn_param *conn_param = &link->conn.ll_data;

    memcpy(conn_param->channel, data, 5);

	__ble_ops->ioctrl(link->hw, BLE_SET_INSTANT, instant);

    __event_oneshot_add(link, __channel_map_upadte, instant - 1);
    
}

static const ll_step_extend master_channel_map_request_steps[] = {
    LL_CHANNEL_MAP_REQ,

    LL_DONE(MASTER_CHANNEL_MAP_REQUEST_STEPS),
};

static void le_channel_map_update(const struct host_channel_classification *param)
{
    struct le_link *link = ll_link_for_state(LL_CONNECTION_ESTABLISHED);

    ASSERT(link != NULL, "%s\n", __func__);

    ASSERT(!ROLE_IS_SLAVE(link), "%s\n", __func__);

    int malloc_len;

    malloc_len = sizeof(struct host_channel_classification);

    le_param.channel_map_update_param = __hci_param_malloc(malloc_len);
   
    memcpy(le_param.channel_map_update_param, param, malloc_len);

    /* __hci_param_free(le_param.channel_map_update_param); */
    
    ll_control_data_step_start(master_channel_map_request_steps);
}

/*
 *      LL Control Procedure - Features Exchange
 */

static void __ll_send_feature_req(void)
{
    struct read_remote_used_features_paramter *param;

    param = le_param.read_remote_used_features_param;

    ASSERT(param != NULL, "%s\n", __func__);

	struct le_link *link = ll_link_for_handle(param->connection_handle);

    ASSERT(link != NULL, "%s\n", __func__);

    __hci_param_free(le_param.read_remote_used_features_param);
    //slave
    if (ROLE_IS_SLAVE(link))
    {
        ll_send_control_data(link, LL_SLAVE_FEATURE_REQ, "c08", 
                le_read_param.features);
    }
    else{
        ll_send_control_data(link, LL_FEATURE_REQ, "c08", 
                le_read_param.features);
    }
}

static void __ll_receive_feature_req(struct le_link *link, struct ble_rx *rx)
{
    u8 *data = &rx->data[1];

    memcpy(le_param.remote_features, data, 8);
}

static void __ll_send_feature_rsp_auto(struct le_link *link)
{
    ll_send_control_data(link, LL_FEATURE_RSP, "c08", 
            le_read_param.features);
}

static void __ll_receive_feature_rsp(struct le_link *link, struct ble_rx *rx)
{
    u8 *data = &rx->data[1];

    memcpy(le_param.remote_features, data, 8);
}

static const ll_step_extend master_features_exchange_steps[] = {
    LL_FEATURE_REQ,
    LL_FEATURE_RSP|WAIT_RX,
    
    LL_DONE(MASTER_FEATURES_EXCHANGE_STEPS),
};

static const ll_step_extend slave_features_exchange_steps[] = {
    LL_SLAVE_FEATURE_REQ,
    LL_FEATURE_RSP|WAIT_RX,
    
    LL_DONE(SLAVE_FEATURES_EXCHANGE_STEPS),
};

/* static  */
void le_features_exchange(
        const struct read_remote_used_features_paramter *param)
{
    /* puts(__func__); */
    /* printf_buf(param, sizeof(struct read_remote_used_features_paramter)); */
    /* put_u16hex(param->connection_handle); */

	struct le_link *link = ll_link_for_handle(param->connection_handle);

    ASSERT(link != NULL, "%s\n", __func__);

    int malloc_len;

    malloc_len = sizeof(struct read_remote_used_features_paramter);

    le_param.read_remote_used_features_param = __hci_param_malloc(malloc_len);

    memcpy(le_param.read_remote_used_features_param, param, malloc_len);

    /* __hci_param_free(le_param.read_remote_used_features_param); */
    
    if (ROLE_IS_SLAVE(link)) 
    {
        if (LE_FEATURES_IS_SUPPORT(LE_SLAVE_INIT_FEATURES_EXCHANGE))
        {
            ll_control_data_step_start(slave_features_exchange_steps);
        }
        else{
            ASSERT(0, "%s\n", __func__);
        }
    }
    else{
        ll_control_data_step_start(master_features_exchange_steps);
    }
}

/*
 *      LL Control Procedure - Version Exchange
 */

static void __ll_send_version_ind_auto(struct le_link *link)
{
    ll_send_control_data(link, LL_VERSION_IND, 
            "122", 
            le_read_param.versnr,
            le_read_param.compld,
            le_read_param.subversnr);
}

static void __ll_send_version_ind(void)
{
    struct read_remote_version_paramter *param;
    
    param = hci_param_t->read_remote_version_param;

    ASSERT(param != NULL, "%s\n", __func__);

	struct le_link *link = ll_link_for_handle(param->connection_handle);

    ASSERT(link != NULL, "%s\n", __func__);

    __hci_param_free(hci_param_t->read_remote_version_param);

    ll_send_control_data(link, LL_VERSION_IND, 
            "122", 
            le_read_param.versnr,
            le_read_param.compld,
            le_read_param.subversnr);
}

static void __ll_receive_version_ind(struct le_link *link, struct ble_rx *rx)
{
    u8 *data = &rx->data[1];

    le_param.remote_versnr = data[0];
    le_param.remote_compld = READ_BT16(data, 1);
    le_param.remote_subversnr = READ_BT16(data, 3);
}

static const ll_step_extend version_ind_steps[] = {
    LL_VERSION_IND,
    LL_VERSION_IND|WAIT_RX,
    
    LL_DONE(VERSION_IND_STEPS),
};

static void le_version_exchange(
        const struct read_remote_version_paramter *param)
{
	struct le_link *link = ll_link_for_handle(param->connection_handle);

    ASSERT(link != NULL, "%s\n", __func__);

    ll_response_timeout_start(link);

    int malloc_len;

    malloc_len = sizeof(struct read_remote_version_paramter);

    hci_param_t->read_remote_version_param = __hci_param_malloc(malloc_len);

    memcpy(hci_param_t->read_remote_version_param, param, malloc_len);

    /* __hci_param_free(hci_param_t->read_remote_version_param); */
    
    ll_control_data_step_start(version_ind_steps);
}

/*
 *      LL Control Procedure - Start Encryption
 */

static void __pseudo_random_number_generate(u8 *data, u8 len)
{
    u8 i;
    
    for(i = 0; i < len;)
    {
        data[i++] = RAND64L;
        data[i++] = RAND64H;
    }
}

//LSB->MSB
static const u8 sample_ivm[4] = {0xba, 0xdc, 0xab, 0x24};
static const u8 sample_skdm[8] = {0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13};

static void __master_ll_send_enc_req(void)
{
    struct start_encryption_parameter *param;

    param = le_param.start_encryption_param;

    ASSERT(param != NULL, "%s\n", __func__);

	struct le_link *link = ll_link_for_handle(param->connection_handle);

    ASSERT(link != NULL, "%s\n", __func__);

    ASSERT(!ROLE_IS_SLAVE(link), "%s\n", __func__);

    u8 ivm[32/8];
    u8 skdm[64/8];

    //LSB - ivm
    __pseudo_random_number_generate(ivm, 32/8);
    /* memcpy(link->ivm, sample_ivm, 4); */
    memcpy(link->ivm, ivm, 4);

    //LSB - skdm 
    __pseudo_random_number_generate(skdm, 64/8);
    /* memcpy(link->skdm, sample_skdm, 8); */
    memcpy(link->skdm, skdm, 8);

    memcpy(link->long_term_key, param->long_term_key, 16);

    ll_send_control_data(link, LL_ENC_REQ, "c08c02c08c04",
           param->random_number,
           param->encrypted_diversifier,
           link->skdm,
           link->ivm);

    __hci_param_free(le_param.start_encryption_param);
}

static void __slave_ll_receive_enc_req(struct le_link *link, struct ble_rx *rx)
{
    ASSERT(ROLE_IS_SLAVE(link), "%s\n", __func__);
    u8 ivm[32/8];
    u8 skdm[64/8];
    //LSB - skdm 
    memcpy(link->skdm, rx->data+1+8+2, 8);
    //LSB - ivm 
    memcpy(link->ivm, rx->data+1+8+2+8, 4);
}

//LSB->MSB
static const u8 sample_ivs[4] = {0xbe, 0xba, 0xaf, 0xde};
static const u8 sample_skds[8] = {0x79, 0x68, 0x57, 0x46, 0x35, 0x24,0x13, 0x02};

/* static const u8 sample_ivs[4] = {0xde, 0xaf, 0xba, 0xbe}; */
/* static const u8 sample_skds[8] = {0x02, 0x13, 0x24, 0x35, 0x46, 0x57,0x68, 0x79}; */

static void __slave_ll_send_enc_rsp_auto(struct le_link *link)
{
    ASSERT(ROLE_IS_SLAVE(link), "%s\n", __func__);

    u8 ivs[32/8];
    u8 skds[64/8];

    //MSB - ivs
    __pseudo_random_number_generate(ivs, 32/8);
    /* memcpy(link->ivs, sample_ivs, 4); */
    memcpy(link->ivs, ivs, 4);

    //MSB - skds 
    __pseudo_random_number_generate(skds, 64/8);
    /* memcpy(link->skds, sample_skds, 8); */
    memcpy(link->skds, skds, 8);

    /* swap32(link->ivs, ivs); */
    /* swap64(link->skds, skds); */
    ll_send_control_data(link, LL_ENC_RSP, 
            "c08c04", 
            link->skds, 
            link->ivs);
}

static void __master_ll_receive_enc_rsp(struct le_link *link, struct ble_rx *rx)
{
    ASSERT(!ROLE_IS_SLAVE(link), "%s\n", __func__);
    //MSB - skds 
    memcpy(link->skds, rx->data+1, 8);
    //MSB - ivs 
    memcpy(link->ivs, rx->data+1+8, 4);
}


//Spec Ver 4.2 Vol 6 Part B 5.1.3
#define SEND_ENCRYPTED         1
#define SEND_UNENCRYPTED       0
#define RECEIVE_ENCRYPTED      1
#define RECEIVE_UNENCRYPTED    0

static void __master_ll_receive_start_enc_req(struct le_link *link)
{
	u8 iv[8];

    //LSB - ivm 
    memcpy(iv, link->ivm, 4);
    //MSB - ivs 
	memcpy(iv+4, link->ivs, 4);

    /* ll_puts("IV :"); */
    /* ll_pbuf(iv, 8); */
    __ble_ops->ioctrl(link->hw, BLE_SET_IV, iv);

	u8 session_key[16];
	u8 skd[16];

    //LSB - skdm
	memcpy(skd, link->skdm, 8);
    //MSB - skds
	memcpy(skd+8, link->skds, 8);
    /* ll_puts("SDK :"); */
    /* ll_pbuf(skd, 16); */
	aes128_start(link->long_term_key, skd, session_key);
    ll_puts("STK : \n");
    printf_buf(link->long_term_key, 16);
	swap128(session_key, skd);
	__ble_ops->ioctrl(link->hw, BLE_SET_SKD, skd);

    struct ble_hw *hw = link->hw;

    __ble_ops->ioctrl(link->hw, BLE_SET_SEND_ENCRYPT, SEND_ENCRYPTED);
    __ble_ops->ioctrl(link->hw, BLE_SET_RECEIVE_ENCRYPT, RECEIVE_ENCRYPTED);

    link->encrypt_enable = 1;
}

static void __slave_ll_receive_start_enc_rsp(struct le_link *link)
{
    struct ble_hw *hw = link->hw;

    __ble_ops->ioctrl(link->hw, BLE_SET_SEND_ENCRYPT, SEND_ENCRYPTED);
    link->encrypt_enable = 1;
}

static void __ll_send_start_enc_rsp_auto(struct le_link *link)
{
    ll_send_control_data(link, LL_START_ENC_RSP, 0);
}


static void __master_ll_send_pause_enc_req(void)
{
    struct start_encryption_parameter *param;

    param = le_param.start_encryption_param;

    ASSERT(param != NULL, "%s\n", __func__);

	struct le_link *link = ll_link_for_handle(param->connection_handle);

    ASSERT(link != NULL, "%s\n", __func__);

    ASSERT(!ROLE_IS_SLAVE(link), "%s\n", __func__);

    ll_send_control_data(link, LL_PAUSE_ENC_REQ, 0);
}




#define LE_IS_ENCRYPT(link)     ((struct le_link *)link->encrypt_enable)

static const ll_step_extend start_encryption_steps[] = {
    LL_ENC_REQ,
    LL_ENC_RSP|WAIT_RX,

    LL_START_ENC_REQ|WAIT_RX,
    LL_START_ENC_RSP,
    LL_START_ENC_RSP|WAIT_RX,
    LL_DONE(START_ENCRYPTION_STEPS),
};

/*
 *      LL Control Procedure - Restart Encryption
 */
static void __slave_ll_receive_pause_enc_req(struct le_link *link)
{
    struct ble_hw *hw = link->hw;

    __ble_ops->ioctrl(link->hw, BLE_SET_RECEIVE_ENCRYPT, RECEIVE_UNENCRYPTED);
}

static void __ll_send_pause_enc_rsp_auto(struct le_link *link)
{
    ll_send_control_data(link, LL_PAUSE_ENC_RSP, 0);
}

static void __master_ll_receive_pause_enc_rsp(struct le_link *link)
{
    struct ble_hw *hw = link->hw;

    __ble_ops->ioctrl(link->hw, BLE_SET_SEND_ENCRYPT, SEND_UNENCRYPTED);
    __ble_ops->ioctrl(link->hw, BLE_SET_RECEIVE_ENCRYPT, RECEIVE_UNENCRYPTED);
    link->encrypt_enable = 0;
}

static void __slave_ll_receive_pause_enc_rsp(struct le_link *link)
{
    struct ble_hw *hw = link->hw;

    __ble_ops->ioctrl(link->hw, BLE_SET_SEND_ENCRYPT, SEND_UNENCRYPTED);
    link->encrypt_enable = 0;
}

static const ll_step_extend restart_encryption_steps[] = {
    LL_PAUSE_ENC_REQ,
    LL_PAUSE_ENC_RSP|WAIT_RX,
    
    LL_PAUSE_ENC_RSP,
    LL_PAUSE_ENC_RSP|WAIT_TX,

    LL_ENC_REQ,
    LL_ENC_RSP|WAIT_RX,

    LL_START_ENC_REQ|WAIT_RX,
    LL_START_ENC_RSP,
    LL_START_ENC_RSP|WAIT_RX,
    LL_DONE(RESTART_ENCRYPTION_STEPS),
};


static void le_start_encrypt(
        const struct start_encryption_parameter *param)
{
    ASSERT(LE_FEATURES_IS_SUPPORT(LE_ENCRYPTION), "%s\n", __func__);

	struct le_link *link = ll_link_for_handle(param->connection_handle);

    ASSERT(link != NULL, "%s\n", __func__);
    
    ASSERT(!ROLE_IS_SLAVE(link), "%s\n", __func__);

    int malloc_len;

    malloc_len = sizeof(struct start_encryption_parameter);
    //HCI parameter
    le_param.start_encryption_param = __hci_param_malloc(malloc_len);

    memcpy(le_param.start_encryption_param, param, malloc_len);

    /* __hci_param_free(le_param.start_encryption_param); */


    if (LE_IS_ENCRYPT(link))
    {
        //for futrue used
        ll_control_data_step_start(restart_encryption_steps);
    }
    else
    {
        ll_control_data_step_start(start_encryption_steps);
    }
}

static void __slave_ll_send_start_enc_req(void)
{
    struct long_term_key_request_reply_parameter *param;

    param = le_param.long_term_key_request_reply_param;

    ASSERT(param != NULL, "%s\n", __func__);

	struct le_link *link = ll_link_for_handle(param->connection_handle);

    ASSERT(link != NULL, "%s\n", __func__);

    u8 iv[8];

    //LSB - ivm
    memcpy(iv, link->ivm, 4);
    //MSB - ivs
    memcpy(iv+4, link->ivs, 4);
    /* ll_puts("IV :"); */
    /* ll_pbuf(iv, 8); */
    __ble_ops->ioctrl(link->hw, BLE_SET_IV, iv);

	u8 session_key[16];
	u8 skd[16];

    //LSB - skdm
	memcpy(skd, link->skdm, 8);
    //MSB - skds
	memcpy(skd+8, link->skds, 8);
    memcpy(link->long_term_key, param->long_term_key, 16);
    /* ll_puts("SDK :"); */
    /* ll_pbuf(skd, 16); */
	aes128_start(link->long_term_key, skd, session_key);
    ll_puts("STK : \n");
    printf_buf(link->long_term_key, 16);
	swap128(session_key, skd);
	__ble_ops->ioctrl(link->hw, BLE_SET_SKD, skd);

    struct ble_hw *hw = link->hw;

    __ble_ops->ioctrl(link->hw, BLE_SET_RECEIVE_ENCRYPT, RECEIVE_ENCRYPTED);

    ll_send_control_data(link, LL_START_ENC_REQ, 0);

    /* printf("long_term_key_request_reply_param free : %08x\n",  le_param.long_term_key_request_reply_param); */
    __hci_param_free(le_param.long_term_key_request_reply_param);
}

static const ll_step_extend start_encryption_req_steps[] = {
    LL_START_ENC_REQ,
    LL_START_ENC_RSP|WAIT_RX,
    LL_START_ENC_RSP,

    LL_DONE(START_ENCRYPTION_REQ_STEPS),
};

static const ll_step_extend restart_encryption_req_steps[] = {
    LL_START_ENC_REQ,
    LL_START_ENC_RSP|WAIT_RX,
    LL_START_ENC_RSP,

    LL_DONE(RESTART_ENCRYPTION_REQ_STEPS),
};


static void le_start_encrypt_req(
        const struct long_term_key_request_reply_parameter *param)
{
    /* puts(__func__); */
    /* printf_buf(param, sizeof(struct long_term_key_request_reply_parameter)); */
    /* put_u16hex(param->connection_handle); */

	struct le_link *link = ll_link_for_handle(param->connection_handle);

    ASSERT(link != NULL, "%s\n", __func__);
    
    /* printf("role is slave : %02x\n",   ROLE_IS_SLAVE(link)); */

    ASSERT(ROLE_IS_SLAVE(link), "%s\n", __func__);

    int malloc_len;

    malloc_len = sizeof(struct long_term_key_request_reply_parameter);
    //HCI parameter
    le_param.long_term_key_request_reply_param = __hci_param_malloc(malloc_len);

    /* printf("long_term_key_request_reply_param malloc : %08x\n",  le_param.long_term_key_request_reply_param); */

    memcpy(le_param.long_term_key_request_reply_param, param, malloc_len);

    if (LE_IS_ENCRYPT(link))
    {
        ll_control_data_step_start(restart_encryption_req_steps);
    }
    else
    {
        ll_control_data_step_start(start_encryption_req_steps);
    }
}

/*
 *      LL Control Procedure - Reject
 */
static const ll_step_extend slave_reject_steps[] = {
    LL_REJECT_IND,

    LL_DONE(SLAVE_REJECT_STEPS),
};

static void __slave_ll_send_reject_ind_auto(struct le_link *link, u8 reason) 
{
    ll_send_control_data(link, LL_REJECT_IND, "1", reason);
}

static void __ll_send_reject_ind( 
        const struct long_term_key_request_negative_reply_parameter *param)
{
    struct le_link *link = ll_link_for_handle(param->connection_handle);

    ASSERT(link != NULL, "%s\n", __func__);

    ASSERT(ROLE_IS_SLAVE(link), "%s\n", __func__);

    ll_send_control_data(link, LL_REJECT_IND, "1", PIN_OR_KEY_MISSING);
}

static void __master_ll_receive_reject_ind(struct le_link *link, struct ble_rx *rx)
{
    //TO*DO
}

static void le_reject(
        const struct long_term_key_request_negative_reply_parameter *param)
{
    __ll_send_reject_ind(param);
}

/*
 *      LL Control Procedure - Disconnect
 */
static void __ll_send_terminate_ind()
{
    struct disconnect_paramter *param;

    param = hci_param_t->disconn_param;

    ASSERT(param != NULL, "%s\n", __func__);

    struct le_link *link = ll_link_for_handle(param->connection_handle);

    ASSERT(link != NULL, "%s\n", __func__);

    ll_send_control_data(link, LL_TERMINATE_IND, "1", param->reason);

    __hci_param_free(hci_param_t->disconn_param);
}

u8 g_master_reason;

static void __master_receive_terminte_ind_done(struct le_link *link)
{
    ll_disconnect_process(link, g_master_reason);
}

static void __master_receive_terminte_ind(struct le_link *link, struct ble_rx *rx)
{
    g_master_reason = rx->data[1];

    u16 instant = __ble_ops->get_conn_event(link->hw);

    __event_oneshot_add(link, __master_receive_terminte_ind_done, instant);
}

static void __ll_receive_terminate_ind(struct le_link *link, struct ble_rx *rx)
{
    u8 reason = rx->data[1];

    //reason : REMOTE_USER_TERMINATED_CONNECTION
    ll_disconnect_process(link, reason);
}

static const ll_step_extend disconnect_steps[] = {
    LL_TERMINATE_IND,
    LL_TERMINATE_IND|WAIT_TX,

    LL_DONE(DISCONNECT_STEPS),
};


/* static  */
void le_disconnect(
        const struct disconnect_paramter *param)
{
    /* puts(__func__); */
    /* printf_buf(param, sizeof(struct disconnect_paramter)); */
    /* put_u16hex(param->connection_handle); */

    struct le_link *link = ll_link_for_handle(param->connection_handle);

    ASSERT(link != NULL, "%s\n", __func__);

    if (LE_IS_CONNECT(link))
    {
        ll_response_timeout_start(link);

        int malloc_len;

        malloc_len = sizeof(struct disconnect_paramter);

        hci_param_t->disconn_param = __hci_param_malloc(malloc_len);

        memcpy(hci_param_t->disconn_param, param, malloc_len);

        ll_control_data_step_start(disconnect_steps);
    }
    else{
        ASSERT(0, "%s\n", __func__);
    }
}


/*
 *      LL Control Procedure - Data Length Update
 */
static void __ll_send_length_req()
{
    struct set_data_length_parameter *param;

    param = le_param.set_data_length_param;

    ASSERT(param != NULL, "%s\n", __func__);

    struct le_link *link = ll_link_for_handle(param->connection_handle);

    ASSERT(link != NULL, "%s\n", __func__);

    link->pdu_len.connMaxTxOctets   = param->txoctets;
    link->pdu_len.connMaxTxTime     = param->txtime;

    ll_send_control_data(link, LL_LENGTH_REQ,
            "2222", 
            link->pdu_len.connMaxRxOctets,
            link->pdu_len.connMaxRxTime,
            param->txoctets,
            param->txtime);

    __hci_param_free(le_param.set_data_length_param);
}

static void __ll_receive_length_req(struct le_link *link, struct ble_rx *rx)
{
    //TO*DO
    u8 *data = &rx->data[1];

    u16 remote_rxoctets;
    u16 remote_rxtime;
    u16 remote_txoctets;
    u16 remote_txtime;
    
    //It shall immediately start using the updated values for all New Data Channel PDUs 
    link->pdu_len.connRemoteMaxRxOctets       = READ_BT16(data, 0);
    link->pdu_len.connRemoteMaxRxTime         = READ_BT16(data, 2);
    link->pdu_len.connRemoteMaxTxOctets       = READ_BT16(data, 4);
    link->pdu_len.connRemoteMaxTxTime         = READ_BT16(data, 6);

    printf("connRemoteMaxRxOctets %x\n", link->pdu_len.connRemoteMaxRxOctets);
    printf("connRemoteMaxRxTime   %x\n", link->pdu_len.connRemoteMaxRxTime  );
    printf("connRemoteMaxTxOctets %x\n", link->pdu_len.connRemoteMaxTxOctets);
    printf("connRemoteMaxTxTime   %x\n", link->pdu_len.connRemoteMaxTxTime  );


    __set_ll_effective_data_args(link);
}

static void __ll_send_length_rsp_auto(struct le_link *link)
{
    ll_send_control_data(link, LL_LENGTH_RSP,
            "2222", 
            link->pdu_len.connMaxRxOctets,
            link->pdu_len.connMaxRxTime,
            link->pdu_len.connMaxTxOctets,
            link->pdu_len.connMaxTxTime);
}

static void __ll_receive_length_rsp(struct le_link *link, struct ble_rx *rx)
{
    //TO*DO
    u8 *data = &rx->data[1];

    u16 remote_rxoctets;
    u16 remote_rxtime;
    u16 remote_txoctets;
    u16 remote_txtime;
    
    link->pdu_len.connRemoteMaxRxOctets = READ_BT16(data, 0);
    link->pdu_len.connRemoteMaxRxTime   = READ_BT16(data, 2);
    link->pdu_len.connRemoteMaxTxOctets = READ_BT16(data, 4);
    link->pdu_len.connRemoteMaxTxTime   = READ_BT16(data, 6);

    printf("connRemoteMaxRxOctets %x\n", link->pdu_len.connRemoteMaxRxOctets);
    printf("connRemoteMaxRxTime   %x\n", link->pdu_len.connRemoteMaxRxTime  );
    printf("connRemoteMaxTxOctets %x\n", link->pdu_len.connRemoteMaxTxOctets);
    printf("connRemoteMaxTxTime   %x\n", link->pdu_len.connRemoteMaxTxTime  );

    __set_ll_effective_data_args(link);
}


static const ll_step_extend data_length_update_steps[] = {
    LL_LENGTH_REQ,
    LL_LENGTH_RSP|WAIT_RX,

    LL_DONE(DATA_LENGTH_UPDATE_STEPS),
};

static void le_data_length_update(
        const struct set_data_length_parameter *param)
{
    ASSERT(LE_FEATURES_IS_SUPPORT(LE_DATA_PACKET_LENGTH_EXTENSION), "%s\n", __func__);

    struct le_link *link = ll_link_for_handle(param->connection_handle);

    ASSERT(link != NULL, "%s\n", __func__);

    if (LE_IS_CONNECT(link)){

        int malloc_len;

        malloc_len = sizeof(struct set_data_length_parameter);

        le_param.set_data_length_param = __hci_param_malloc(malloc_len);

        memcpy(le_param.set_data_length_param, param, malloc_len);

        ll_control_data_step_start(data_length_update_steps);
    }else{
        
        ASSERT(0, "%s\n", __func__);
    }
}
/*
 *      LL Control Procedure - Unknow Respond
 */
static void __ll_send_unknow_rsp_auto(struct le_link *link, struct ble_rx *rx)
{
    ll_send_control_data(link, LL_UNKNOWN_RSP, "1", rx->data[0]);
}


/*------------------------------------------------------------------
 *
 * 					LL Control Procedure Emit Event	
 *
 * -----------------------------------------------------------------*/

static void ll_control_procedure_finish_emit_event(struct le_link *link, struct ble_rx *rx, LL_CONTROL_CASE status)
{
    ll_step procedure = LL_STEPS();
    
    if (!LL_IS_FINSH())
    {
        //accident break
        procedure =  ll_control_current_procedure_by_localhost();
    }
    
    switch(procedure)
    {
        case SLAVE_CONNECTION_PARAMETER_REQUEST_STEPS:
            __le_connection_update_complete_event(link, rx, status);
            break;
        case MASTER_CONNECTION_PARAMETER_REQUEST_STEPS:
            __le_connection_update_complete_event(link, rx, status);
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
            __le_read_remote_used_features_complete_event(link, rx, status);
            break;
        case SLAVE_FEATURES_EXCHANGE_STEPS:
            __le_read_remote_used_features_complete_event(link, rx, status);
            break;
        case VERSION_IND_STEPS:
            __hci_read_remote_version_information_complete_event(link, rx, status);
            break;
        case START_ENCRYPTION_STEPS:
            __hci_event_emit(procedure, link, rx, status);
            break;
        case RESTART_ENCRYPTION_STEPS:
            __hci_event_emit(procedure, link, rx, status);
            break;
        case START_ENCRYPTION_REQ_STEPS:
            __hci_event_emit(procedure, link, rx, status);
            break;
        case RESTART_ENCRYPTION_REQ_STEPS:
            __hci_event_emit(procedure, link, rx, status);
            break;
        case SLAVE_REJECT_STEPS:
            break;
        case DISCONNECT_STEPS:
            __le_disconnect_complete_event(link, status);
            break;
        case DATA_LENGTH_UPDATE_STEPS:
            __le_data_length_change_event(link);
            break;
    }
}

/*------------------------------------------------------------------
 *
 * 							master
 *
 * -----------------------------------------------------------------*/

static void master_rx_ctrl_pdu_handler(struct le_link *link, struct ble_rx *rx)
{
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
            __master_receive_terminte_ind(link, rx);
			break;
        
        case LL_ENC_REQ:
            ASSERT(0, "%s\n", "M LL_ENC_REQ");
            break;
		case LL_ENC_RSP:
            __master_ll_receive_enc_rsp(link, rx);
			break;
		case LL_START_ENC_REQ:
            __master_ll_receive_start_enc_req(link);
            __ll_send_start_enc_rsp_auto(link);
			break;
        case LL_START_ENC_RSP:
            break;

        case LL_UNKNOWN_RSP:
            ll_puts("--LL_UNKNOWN_RSP\n");
			printf_buf(rx->data, rx->len);
            break;

		case LL_FEATURE_REQ:
            ASSERT(0, "%s\n", "M LL_FEATURE_REQ");
			break;
		case LL_FEATURE_RSP:
            __ll_receive_feature_rsp(link, rx);
			break;

        case LL_PAUSE_ENC_REQ:
            ASSERT(0, "%s\n", "M LL_PAUSE_ENC_REQ");
            break;
		case LL_PAUSE_ENC_RSP:
            __master_ll_receive_pause_enc_rsp(link);
            __ll_send_pause_enc_rsp_auto(link);
			break;

		case LL_VERSION_IND:
            //depend steps
            __ll_receive_version_ind(link, rx);
            if (LL_CTRL_NO_STEPS()){
                __ll_send_version_ind_auto(link);
            }
			break;
        case LL_REJECT_IND:
            __master_ll_receive_reject_ind(link, rx);
            break;

        case LL_SLAVE_FEATURE_REQ:
            __ll_receive_feature_req(link, rx);
            __ll_send_feature_rsp_auto(link);
            break;

		case LL_CONNECTION_PARAM_REQ: //4.2 new feature
			/* master_conn_param_req(link, data, rx->event_count); */
            switch(master_ll_control_current_procedure)
            {
                //autonomously & indicate by LocalHost procedure
                case LL_CONNECTION_PARAM_REQ:
                case LL_CONNECTION_UPDATE_REQ:
                    if (LE_FEATURES_IS_SUPPORT(EXTENDED_REJECT_INDICATION)){
                        __ll_send_reject_ind_ext_auto(link, opcode, LMP_ERROR_TRANSACTION_COLLISION);
                    }
                    break;
                case LL_CHANNEL_MAP_REQ:
                    if (LE_FEATURES_IS_SUPPORT(EXTENDED_REJECT_INDICATION)){
                        __ll_send_reject_ind_ext_auto(link, opcode, DIFFERENT_TRANSACTION_COLLISION);
                    }
                    break;
            }
            if (LE_FEATURES_IS_SUPPORT(CONNECTION_PARAMETER_REQUEST))
            {
                CONN_PARAM_RES res;
                res = __ll_receive_connection_param_req(link, rx);
                switch(res)
                {
                case CONN_PARAM_REQ_AUTO:
                    //if move anchor point
                    __master_ll_send_conn_update_req_auto(link);
                    break;
                case CONN_PARAM_REQ_HOST:
                    //change conn param
                    {
                        //is masked
                        if (FALSE == __le_remote_connection_parameter_request_event(link, rx))
                        {
                            if (LE_FEATURES_IS_SUPPORT(EXTENDED_REJECT_INDICATION)){
                                __ll_send_reject_ind_ext_auto(link, opcode, UNSUPPORTED_REMOTE_FEATURE_UNSUPPORTED_LMP_FEATURE);
                            }
                        }
                    }
                    break;
                case CONN_PARAM_REQ_REJECT:
                    if (LE_FEATURES_IS_SUPPORT(EXTENDED_REJECT_INDICATION)){
                        __ll_send_reject_ind_ext_auto(link, opcode, UNSUPPORTED_LMP_PARAMETER_VALUE_UNSUPPORTED_LL_PARAMETER_VALUE);
                    }
                    break;
                case CONN_PARAM_REQ_INVALID:
                    if (LE_FEATURES_IS_SUPPORT(EXTENDED_REJECT_INDICATION)){
                        __ll_send_reject_ind_ext_auto(link, opcode, INVALID_LMP_PARAMETERS_INVALID_LL_PARAMETERS);
                    }
                    break;
                }
            }
            else
            {
                __ll_send_unknow_rsp_auto(link, rx);
                /* ASSERT(0, "%s\n", "M LL_CONNECTION_PARAM_REQ"); */
            }
			break;
		case LL_CONNECTION_PARAM_RSP: //4.2 new feature
            /* master_conn_param_req(link, data, rx->event_count); */
            __master_ll_receive_connection_param_rsp(link, rx);
            if (LL_CTRL_NO_STEPS()){
                __master_ll_send_conn_update_req_auto(link);
            }
			break;
        case LL_REJECT_IND_EXT:
            __ll_receive_reject_ind_ext(link, rx);
            break;

        case LL_PING_REQ:
            break;
        case LL_PING_RSP:
            break;
        
        case LL_LENGTH_REQ:
            ll_puts("--LL_LENGTH_REQ\n");
            if (LE_FEATURES_IS_SUPPORT(LE_DATA_PACKET_LENGTH_EXTENSION))
            {
                __ll_receive_length_req(link, rx);
                __ll_send_length_rsp_auto(link);


                __le_data_length_change_event(link);
            }
            else{
                __ll_send_unknow_rsp_auto(link, rx);
            }
            break;
        case LL_LENGTH_RSP:
            ll_puts("--LL_LENGTH_RSP\n");
            __ll_receive_length_rsp(link, rx);
            break;

		default:
			printf("unkonw-opcode: %x\n", opcode);
			printf_buf(rx->data, rx->len);
            __ll_send_unknow_rsp_auto(link, rx);
			break;
	}
}





/*-----------------------------------------------------------
 *
 *  					slave
 *
 * ----------------------------------------------------------*/


static void slave_rx_ctrl_pdu_handler(struct le_link *link, struct ble_rx *rx)
{
	u8 opcode = rx->data[0];
	u8 *data = &rx->data[1];

	switch(opcode)
	{
		case LL_CONNECTION_UPDATE_REQ:
            ll_puts("--LL_CONNECTION_UPDATE_REQ\n");
            //avoid the ll_control_procedure_collisions
            __slave_ll_receive_conn_update_req(link, rx);
			break;
		case LL_CHANNEL_MAP_REQ:
            ll_puts("--LL_CHANNEL_MAP_REQ\n");
            __slave_ll_receive_channel_map_req(link, rx);
			break;
		case LL_TERMINATE_IND:
            ll_puts("--LL_TERMINATE_IND\n");
            __ll_receive_terminate_ind(link, rx);
			break;

		case LL_ENC_REQ:
            ll_puts("--LL_ENC_REQ\n");
			__slave_ll_receive_enc_req(link, rx);
            if (LE_FEATURES_IS_SUPPORT(LE_ENCRYPTION))
            {
                __slave_ll_send_enc_rsp_auto(link);
            }else{
                __slave_ll_send_reject_ind_auto(link, UNSUPPORTED_REMOTE_FEATURE_UNSUPPORTED_LMP_FEATURE);
                break;
            }

            if (FALSE == __le_long_term_key_request_event(link, rx))
            {
                if (LE_FEATURES_IS_SUPPORT(EXTENDED_REJECT_INDICATION)){
                    __ll_send_reject_ind_ext_auto(link, opcode, PIN_OR_KEY_MISSING);
                }else{
                    __slave_ll_send_reject_ind_auto(link, PIN_OR_KEY_MISSING);
                }
            }
			break;
		case LL_ENC_RSP:
            ASSERT(0, "%s\n", "S LL_ENC_RSP");
			break;
		case LL_START_ENC_REQ:
            ASSERT(0, "%s\n", "S LL_START_ENC_REQ");
            break;
		case LL_START_ENC_RSP:
            ll_puts("--LL_START_ENC_RSP\n");
            __slave_ll_receive_start_enc_rsp(link);
            __ll_send_start_enc_rsp_auto(link);
			break;

		case LL_UNKNOWN_RSP:
            ll_puts("--LL_UNKNOWN_RSP\n");
			printf_buf(rx->data, rx->len);
			break;

		case LL_FEATURE_REQ:
            ll_puts("--LL_FEATURE_REQ\n");
            __ll_receive_feature_req(link, rx);
            __ll_send_feature_rsp_auto(link);
			break;
		case LL_FEATURE_RSP:
            ll_puts("--LL_FEATURE_RSP\n");
            __ll_receive_feature_rsp(link, rx);
			break;

		case LL_PAUSE_ENC_REQ:
            ll_puts("--LL_PAUSE_ENC_REQ\n");
            __slave_ll_receive_pause_enc_req(link);
            __ll_send_pause_enc_rsp_auto(link);
			break;
		case LL_PAUSE_ENC_RSP:
            ll_puts("--LL_PAUSE_ENC_RSP\n");
            __slave_ll_receive_pause_enc_rsp(link);
			break;


		case LL_VERSION_IND:
            ll_puts("--LL_VERSION_IND\n");
            __ll_receive_version_ind(link, rx);
            if (LL_CTRL_NO_STEPS()){
                __ll_send_version_ind_auto(link);
            }else{
            }
			break;
		case LL_REJECT_IND:
            ASSERT(0, "%s\n", "S LL_REJECT_IND");
			break;

        case LL_SLAVE_FEATURE_REQ:
            ASSERT(0, "%s\n", "S LL_SLAVE_FEATURE_REQ");
            break;

		case LL_CONNECTION_PARAM_REQ: //4.2 new feature
            ll_puts("--LL_CONNECTION_PARAM_REQ\n");
            if (LE_FEATURES_IS_SUPPORT(CONNECTION_PARAMETER_REQUEST))
            {
                CONN_PARAM_RES res;
                res = __ll_receive_connection_param_req(link, rx);
                switch(res)
                {
                case CONN_PARAM_REQ_AUTO:
                    //if move anchor point
                    __slave_ll_send_conn_param_rsp_auto(link);
                    break;
                case CONN_PARAM_REQ_HOST:
                    //change conn param
                    {
                        //is masked
                        if (FALSE == __le_remote_connection_parameter_request_event(link, rx))
                        {
                            if (LE_FEATURES_IS_SUPPORT(EXTENDED_REJECT_INDICATION)){
                                __ll_send_reject_ind_ext_auto(link, opcode, UNSUPPORTED_REMOTE_FEATURE_UNSUPPORTED_LMP_FEATURE);
                            }
                        }
                    }
                    break;
                case CONN_PARAM_REQ_REJECT:
                    if (LE_FEATURES_IS_SUPPORT(EXTENDED_REJECT_INDICATION)){
                        __ll_send_reject_ind_ext_auto(link, opcode, UNSUPPORTED_LMP_PARAMETER_VALUE_UNSUPPORTED_LL_PARAMETER_VALUE);
                    }
                    break;
                case CONN_PARAM_REQ_INVALID:
                    if (LE_FEATURES_IS_SUPPORT(EXTENDED_REJECT_INDICATION)){
                        __ll_send_reject_ind_ext_auto(link, opcode, INVALID_LMP_PARAMETERS_INVALID_LL_PARAMETERS);
                    }
                    break;
                }
            }else{
                __ll_send_unknow_rsp_auto(link, rx);
                ASSERT(0, "%s\n", "S LL_CONNECTION_PARAM_REQ");
            }
			break;
		case LL_CONNECTION_PARAM_RSP: //4.2 new feature
            ASSERT(0, "%s\n", "S LL_SLAVE_FEATURE_REQ");
			break;
        case LL_REJECT_IND_EXT:
            ll_puts("--LL_REJECT_IND_EXT\n");
            __ll_receive_reject_ind_ext(link, rx);
            break;

        case LL_PING_REQ:
            ll_puts("--LL_PING_REQ\n");
            break;
        case LL_PING_RSP:
            ll_puts("--LL_PING_RSP\n");
            break;
        
        case LL_LENGTH_REQ:
            ll_puts("--LL_LENGTH_REQ\n");
            if (LE_FEATURES_IS_SUPPORT(LE_DATA_PACKET_LENGTH_EXTENSION))
            {
                __ll_receive_length_req(link, rx);
                __ll_send_length_rsp_auto(link);

                __le_data_length_change_event(link);
            }
            else{
                __ll_send_unknow_rsp_auto(link, rx);
            }
            break;
        case LL_LENGTH_RSP:
            ll_puts("--LL_LENGTH_RSP\n");
            __ll_receive_length_rsp(link, rx);
            break;

		default:
			printf("unkonw-opcode: %x\n", opcode);
			printf_buf(rx->data, rx->len);
            __ll_send_unknow_rsp_auto(link, rx);
			break;
	}
}



void le_ll_push_control_data(u8 opcode, const u8 *param)
{
    switch(opcode) 
    {
        case HCI_LE_CONNECTION_UPDATE:
            le_connection_update(param);
            break;
        case HCI_LE_SET_HOST_CHANNEL_CLASSIFICATION:
            le_channel_map_update(param);
            break;
        case HCI_LE_READ_REMOTE_USED_FEATURES:
            le_features_exchange(param);
            break;
        case HCI_READ_REMOTE_VERSION_INFORMATION:
            le_version_exchange(param);
            break;
        case HCI_LE_START_ENCRYPT:
            le_start_encrypt(param);
            break;
        case HCI_LE_LONG_TERM_KEY_REQUEST_REPLY:
            le_start_encrypt_req(param);
            break;
        case HCI_LE_LONG_TERM_KEY_REQUEST_NAGATIVE_REPLY:
            le_reject(param);
            break;
        case HCI_DISCONNECT:
            le_disconnect(param);
            break;
        case HCI_LE_REMOTE_CONNECTION_PARAMETER_REQUEST_REPLY:
            le_connection_paramter_respond(param);
            break;
        case HCI_LE_REMOTE_CONNECTION_PARAMETER_REQUEST_NEGATIVE_REPLY:
            //only use UNACCEPTABLE_CONNECTION_PARAMETERS 
            le_reject_ext(param);
            break;
        /* case : */
            /* le_ping(param); */
            /* break; */
        case HCI_LE_SET_DATA_LENGTH:
			ll_puts("LL_SET_DATA_LENGTH\n");
            le_data_length_update(param);
            break;
    }
}

static void echo_ll_ctrl_pdu_handler(struct le_link *link, ll_step step, 
        struct ble_rx *rx, struct ble_tx *tx)
{
    ll_puts("\n echo - ");
	switch(step)
	{
		case LL_CONNECTION_UPDATE_REQ:
            ll_puts("LL_CONNECTION_UPDATE_REQ\n");
            __master_ll_send_conn_update_req();
			break;
        case LL_CHANNEL_MAP_REQ:
            ll_puts("LL_CHANNEL_MAP_REQ\n");
            __master_ll_send_channel_map_req();
            break;
		case LL_TERMINATE_IND:
            ll_puts("LL_TERMINATE_IND\n");
            __ll_send_terminate_ind();
			break;

        case LL_ENC_REQ:
            ll_puts("LL_ENC_REQ\n");
            __master_ll_send_enc_req();
            break;
		case LL_ENC_RSP:
            ll_puts("LL_ENC_RSP\n");
			break;
		case LL_START_ENC_REQ:
            ll_puts("LL_START_ENC_REQ\n");
            __slave_ll_send_start_enc_req();
			break;
        case LL_START_ENC_RSP:
            ll_puts("LL_START_ENC_RSP\n");
            break;

        case LL_SLAVE_FEATURE_REQ:
        case LL_FEATURE_REQ:
            ll_puts("LL_FEATURE_REQ\n");
            __ll_send_feature_req();
            break;
		case LL_FEATURE_RSP:
            ll_puts("LL_FEATURE_RSP\n");
			break;

        case LL_PAUSE_ENC_REQ:
            ll_puts("LL_PAUSE_ENC_REQ\n");
            __master_ll_send_pause_enc_req();
            break;
		case LL_PAUSE_ENC_RSP:
            ll_puts("LL_PAUSE_ENC_RSP\n");
			break;

		case LL_VERSION_IND:
            ll_puts("LL_VERSION_IND\n");
            __ll_send_version_ind();
			break;
        case LL_REJECT_IND:
            ll_puts("LL_REJECT_IND\n");
            break;

		case LL_CONNECTION_PARAM_REQ: //4.2 new feature
            ll_puts("LL_CONNECTION_PARAM_REQ\n");
            __ll_send_conn_param_req();
			break;
        case LL_CONNECTION_PARAM_RSP:
            ll_puts("LL_CONNECTION_PARAM_RSP\n");
            __slave_ll_send_conn_param_rsp();
            break;

        case LL_REJECT_IND_EXT:
            ll_puts("LL_REJECT_IND_EXT\n");
            break;
            
        case LL_PING_REQ:
            ll_puts("LL_PING_REQ\n");
            break;
        case LL_PING_RSP:
            ll_puts("LL_PING_RSP\n");
            break;

        case LL_LENGTH_REQ:
            ll_puts("LL_LENGTH_REQ\n");
            __ll_send_length_req();
            break;

		case LL_UNKNOWN_RSP:
            ll_puts("LL_UNKNOWN_RSP\n");
			break;

		default:
			printf("unkonw-opcode: %x\n", step);
			break;
	}
}



//emit le meta event
static void rx_ctrl_pdu_handler(struct le_link *link, struct ble_rx *rx)
{
    //process receive 
    if (ROLE_IS_SLAVE(link)) {
        slave_rx_ctrl_pdu_handler(link, rx);
    } else {
        master_rx_ctrl_pdu_handler(link, rx);
    }

    ll_control_step_verify((rx->data[0] | WAIT_RX), link, rx, NULL);

    //process send 
    ll_send_control_data_state_machine(link, rx, NULL);
}

static void rx_unknow_pdu_handler(struct le_link *link, struct ble_rx *rx)
{
    //MIC error
    if (link->encrypt_enable)
    {
        puts(__func__);
        rx->data[1] = CONNECTION_TERMINATED_DUE_TO_MIC_FAILURE;
        __hci_event_emit(DISCONNECT_STEPS, link, rx, NULL);
    }
}

static void ll_rx_post_handler(void *priv, struct ble_rx *rx)
{
	struct le_link *link = (struct le_link *)priv;

    bool upper_pass = FALSE;

    switch(rx->llid)
    {
        case LL_RESERVED:
            /* putchar('b'); */
            upper_pass = rx_pdu_handler(link, rx);
            break;
        case LL_DATA_PDU_START:
        case LL_DATA_PDU_CONTINUE:
            rx_data_pdu_handler(link, rx);
            break;
        case LL_CONTROL_PDU:
            rx_ctrl_pdu_handler(link, rx);
            break;
        default:
            break;
    }
    //notify upper layer
    //TO*DO skip notify
    if (upper_pass == TRUE)
    {
        return;
    }

    //llp_acl_rxchannel(link, rx);
    if (ll.handler && ll.handler->rx_handler){
        ll.handler->rx_handler(link, rx);
    }
}

static int ll_rx_upload_data(void)
{
    __ble_ops->upload_data(ll_rx_post_handler);

    
    /* if (__upper_channel()->rx_async_pop()) */
        /* __upper_channel()->rx_async_pop(); */

    /* __lower_channel()->rx_sync_callback(); */
}



/********************************************************************************/






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
            /* tx_adv_pdu_handler(link, tx); */
            break;
        case LL_SCANNING:
            /* tx_scan_pdu_handler(link, tx); */
            break;
        case LL_INITIATING:
            break;
        case LL_CONNECTION_CREATE:
            /* tx_init_pdu_handler(link, tx); */
            break;
    }
}

static void slave_tx_ctrl_pdu_handler(struct le_link *link, struct ble_tx *tx)
{
	u8 opcode = tx->data[0];
	u8 *data = &tx->data[1];

	printf("\no: %x\n", opcode);

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
			break;
		case LL_START_ENC_REQ:
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
			printf_buf(tx->data, tx->len);
			break;
	}
}

static void master_tx_ctrl_pdu_handler(struct le_link *link, struct ble_tx *tx)
{
    u8 opcode = tx->data[0];

    switch(opcode) 
    {

    }
}

static void tx_ctrl_pdu_handler(struct le_link *link, struct ble_tx *tx)
{
    //process receive 
#if 0
    if (ROLE_IS_SLAVE(link)) {
        slave_tx_ctrl_pdu_handler(link, tx);
    } else {
        master_tx_ctrl_pdu_handler(link, tx);
    }
#endif

    ll_control_step_verify((tx->data[0] | WAIT_TX), link, NULL, tx);

    //process send 
    ll_send_control_data_state_machine(link, NULL, tx);
}

static void ll_tx_probe_handler(void *priv, struct ble_tx *tx)
{
    struct le_link *link = (struct le_link *)priv;

    switch (tx->llid)
    {
        case LL_RESERVED:
            /* tx_pdu_handler(link, tx); */
            break;
        case LL_DATA_PDU_START:
        case LL_DATA_PDU_CONTINUE:
            //LL Ack
            /* le_send_event(HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS, "1H2", 1, */
                    /* link->handle, 1); */
            break;
        case LL_CONTROL_PDU:
            tx_ctrl_pdu_handler(link, tx);
            break;

    }

    if (ll.handler && ll.handler->tx_handler){
        ll.handler->tx_handler(link, tx);
    }

    //resume ll thread
    thread_resume(&ll.ll_thread);
}

static void ll_event_handler(struct le_link *link, int event)
{
    putchar('#');put_u16hex(event);
	__event_oneshot_run(link, event);
}

//Bottom upload from Baseband API
static const struct ble_handler ll_handler={
    .rx_probe_handler   = ll_rx_probe_handler,
    .tx_probe_handler   = ll_tx_probe_handler,
	.event_handler      = ll_event_handler,
};


static int ll_open(int state);
static void ll_thread_process(struct thread *th)
{
	/* u8 local_addr[6]; */
	struct le_link *link, *n;
    int awake = 0;

    ll_rx_upload_data();

    //le meta event pop
    le_meta_event_upload();

	list_for_each_entry_safe(link, n, &ll.head, entry)
	{
		if (link->state == LL_DISCONNECT)
		{
			puts("\nll_disconnect");
            put_u8hex(link->handle);

            __ble_ops->close(link->hw);

			/* memcpy(local_addr, link->local.addr, 6); */

            ll.handle &= ~(link->handle);
            
            __free_link(link);

            /* goto __re_adv; */
		}
	}

    //wait all channel idle
    if (__ble_ops->upload_is_empty()
        && (lbuf_empty(le_event_buf)))
    {
        //suspend ll thread now
        /* ll_puts("ll suspend\n"); */
        thread_suspend(th, 0);
    }
    
	return;

__re_adv:
    ll_open(LL_ADVERTISING);
    return;
}



static int ll_open(int state)
{
	struct le_link *link;

	ll_puts("ll_open\n");

	link = ll_create_link();
	if (!link){
		return -1;
	}

	INIT_LIST_HEAD(&link->rx_oneshot_head);
	INIT_LIST_HEAD(&link->event_oneshot_head);


	link->hw = __ble_ops->open();
    ASSERT(link->hw != NULL);

    link->handle = __ble_ops->get_id(link->hw);

    ll.handle |= link->handle;
    if (ll.handle == 0xff)
    {
        __free_link(link);
        puts("link over 8 err\n");
        return NULL;
    }

	__ble_ops->handler_register(link->hw, link, &ll_handler);
    
    //feature - privacy
    if (LE_FEATURES_IS_SUPPORT(LL_PRIVACY))
    {
        __ble_ops->ioctrl(link->hw, BLE_SET_PRIVACY_ENABLE, le_param.resolution_enable);
    }

    //feature - data length extension
    __set_ll_data_length(link);

	__set_link_state(link, state);

	puts("exit\n");
	return 0;
}


static void ll_close(int state)
{
	struct le_link *link, *n;

	list_for_each_entry_safe(link, n, &ll.head, entry)
	{
        if (link->state != state)   continue;

        switch (link->state)
        {
            case LL_INITIATING:
                __le_connection_complete_event(link, UNKNOWN_CONNECTION_IDENTIFIER);
            case LL_ADVERTISING:
            case LL_SCANNING:
                __ble_ops->close(link->hw);
                ll.handle &= ~(link->handle);
                __free_link(link);
                break;
        }
	}
}




void ll_read_channel_map(u8 *channel_map, const u8 *data)
{ 
	struct le_link *link = ll_link_for_handle(READ_CONNECTION_HANDLE(data));

    ASSERT(link != NULL, "%s\n", __func__);

    memcpy(channel_map, link->conn.ll_data.channel, 5);
}


static void ll_ioctrl(int ocf, ...)
{
    va_list argptr;
    va_start(argptr, ocf);

    va_arg(argptr, int);

    va_end(argptr);
}

static int ll_handler_register(const struct lc_handler *handler)
{
	ll.handler = handler;
}


static void ll_data_length_init(void)
{
    memset(le_data_length, 0x0, sizeof(struct ll_data_pdu_length));

    /* printf("connInitialMaxTxTime: %x\n", le_data_length.connInitialMaxTxTime); */
    le_data_length.priv = &ll_data_pdu_length_ro;
}


static void *ll_init(struct hci_parameter *hci_param)
{

    /* printf("addr : %08x\n", mem_pool); */
	/* lbuf_init(mem_pool, sizeof(mem_pool)); */

	/* lbuf_init(hci_mem_pool, sizeof(hci_mem_pool)); */

    if (!bt_power_is_poweroff_post())
    {
        puts("ll_init X\n");
        ll.handle = 0x00;   //bitmap index - HW_ID
        ll.handler = NULL;

        INIT_LIST_HEAD(&ll.head);

        //filter policy
        ll_white_list_init();

        //privacy
        ll_resolving_list_init();
    }
    INIT_LIST_HEAD(&pkt_list_head);

    ll_data_length_init();
    
	le_event_buf = lbuf_init(ll_buf, 512);

	__ble_ops->init();

	thread_create(&ll.ll_thread, ll_thread_process, 3);

    hci_param_t = hci_param; 
    le_param.priv = &le_read_param;

    return &le_param;
}

static const struct ll_interface ll_api = {
    .init = ll_init,
    .open = ll_open,
    .close = ll_close,
    .handler_register = ll_handler_register,
    //device filtering
    .white_list_init    = ll_white_list_init,
    .white_list_add     = ll_white_list_add, 
    .white_list_remove  = ll_white_list_remove, 
    //privacy
    .resolving_list_init    = ll_resolving_list_init,
    .resolving_list_add     = ll_resolving_list_add,
    .resolving_list_remove  = ll_resolving_list_remove,

    .read_peer_RPA  = ll_read_peer_resolvable_address,
    .read_local_RPA = ll_read_local_resolvable_address,

    .resolution_enable  = ll_resolution_enable,
    .set_RPA_timeout    = ll_set_resolvable_private_address_timeout,

    .ioctrl = ll_ioctrl,
};
REGISTER_LL_INTERFACE(ll_api);


/*-------------------------------------------------------------------------
 *
 * 							API
 *
 * ----------------------------------------------------------------------*/
struct ll_data_pdu_length_read_only *ll_read_maximum_data_length(void)
{
    return le_data_length.priv;
}

struct ll_data_pdu_length *ll_read_suggested_default_data_length(void)
{
    return &le_data_length;
}

void ll_write_suggested_default_data_length(const u8 *data)
{
    //Range 0x001B - 0x00FB
    le_data_length.suggestedMaxTxOctets = READ_BT16(data, 0);
    printf("suggestedMaxTxOctets : %x\n", le_data_length.suggestedMaxTxOctets);

    //Range 0x0148 - 0x0848
    le_data_length.suggestedMaxTxTime = READ_BT16(data, 2);
    printf("suggestedMaxTxOctets : %x\n", le_data_length.suggestedMaxTxTime);
}

u8 ll_packet_is_send(void)
{
    struct fragment_pkt *p, *n;

	list_for_each_entry_safe(p, n, &pkt_list_head, entry){
        /* printf("ll_is_packet_send pkt cnt %x\n", p->cnt); */
        p->cnt--;
        if (!p->cnt)
        {
            g_pkt_idx--;
            list_del(&p->entry);
            __free(p);
            return 1;
        }
        return 0;
    }
}
/* static const struct le_link_control_interface le_link_control_api */
/* { */
    /* .phy_link_init      = ; */
    /* .create_phy_link    = ; */
    /* .delete_phy_link    = ;  */
/* } */

/* static const struct llp_interface llp_api */
/* { */
    

struct rpa{
    u8 addr[6];
    u8 peer_irk[16];
    u8 local_irk[16];
};

struct rpa rpa_set[] = {
    //Nexus 5
    [0] = {
        .addr = {0x76, 0x41, 0x46, 0x31, 0x3a, 0x4d}, //LSB->MSB
        .peer_irk = {0x74, 0x01, 0x90, 0x4A, 0xF7, 0x35, 0xA4, 0x38, 0xD3, 0x20, 0x9A, 0xCD, 0x48, 0xC0, 0x54, 0x40},
        .local_irk = {0xE6, 0xEA, 0xEE, 0x60, 0x31, 0x7B, 0xFC, 0xA2, 0x3F, 0xA5, 0x79, 0x59, 0xE7, 0x41, 0xCF, 0xC7},
    },
    [1] = {
        .addr = {0xA9, 0x3E, 0xD4, 0x37, 0xB5, 0x75}, //LSB->MSB
        .peer_irk = {0x74, 0x01, 0x90, 0x4A, 0xF7, 0x35, 0xA4, 0x38, 0xD3, 0x20, 0x9A, 0xCD, 0x48, 0xC0, 0x54, 0x40},
    },
};

void debug_ll_privacy(void)
{
    u8 hash[3];
    const u8 *addr;
    const u8 *pirk;
    const u8 *lirk;

    puts("---------------LL privacy debug---------\n");
    addr = rpa_set[0].addr;
    pirk = rpa_set[0].peer_irk;
    puts("RPA : ");
    printf_buf(addr, 6);
    puts("peer IRK : ");
    printf_buf(pirk, 16);

    if (FALSE == __ll_resolvable_private_addr_verify(addr))
    {
        puts("ll resolve addr format error\n");
        return;
    }

    puts("prand ");
    printf_buf(addr + 3, 3);
    irk_enc(pirk, addr + 3, hash);
    puts("hash ");
    printf_buf(hash, 3);

    //localHash match peer hash
    if (!memcmp(hash, addr, 3)) 
    {
        puts("ll peer private addr resolved!\n");
    }

    u8 rpa[6];
    lirk = rpa_set[0].local_irk;
    puts("local IRK : ");
    printf_buf(lirk, 16);
    device_addr_generate(rpa,\
           lirk, RESOLVABLE_PRIVATE_ADDR);
    printf_buf(rpa, 6);

    puts("prand ");
    printf_buf(rpa + 3, 3);
    irk_enc(lirk, rpa + 3, hash);
    puts("hash ");
    printf_buf(hash, 3);

    //localHash match peer hash
    if (!memcmp(hash, rpa, 3)) 
    {
        puts("ll local private addr resolved!\n");
    }

    puts("---------------LL privacy debug end---------\n");
}

void ll_vendor_shutdown(void)
{   
    struct le_link *link, *n;

    if (ll.handle == 0x00)
        return;

    thread_delete(&ll.ll_thread);
	list_for_each_entry_safe(link, n, &ll.head, entry)
	{
        puts("\nll_shutdown");
        put_u8hex(link->handle);

        __ble_ops->close(link->hw);

        /* memcpy(local_addr, link->local.addr, 6); */

        ll.handle &= ~(link->handle);
        
        __free_link(link);
	}
}
