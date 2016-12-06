#include "RF_ble.h"
#include "ble_interface.h"
#include "interrupt.h"
#include "init.h"
/*#include "platform_device.h"*/
#include "stdarg.h"
#include "RF.h"
#include "uart.h"
#include "bt_power.h"
#include "bt_memory.h"

/************************RF DEBUG CONTROL**************************/
/* #define RF_DEBUG */

#ifdef RF_DEBUG
#define rf_putchar(x)   putchar(x)
#define rf_puts         puts
#define rf_u32hex(x)    put_u32hex(x)
#define rf_u8hex(x)     put_u8hex(x)
#define rf_deg          printf
#define rf_buf(x,y)     printf_buf(x,y)

#else
#define rf_putchar(x) 
#define rf_puts(...)     
#define rf_u32hex(x)    
#define rf_u8hex(x)     
#define rf_deg(...)      
#define rf_buf(...)

#endif

static volatile int ble_debug_signal[BLE_HW_NUM];

#define READ_BT16(p, offset)  (((u16)p[(offset)+1]<<8) | p[offset])


/*#define PHY_TO_BLE(a) \*/
		/*((u32)(a) - ((u32)&ble_base))*/
#define PHY_TO_BLE(a) \
		({ ASSERT((u32)(a)-(u32)&ble_base < 0xffff); \
		((u32)(a) - ((u32)&ble_base)); \
		})

#define HW_ID(hw) \
	((hw) - ble_base.hw)

#define HW_TX_INDEX(ble_fp)    ((struct ble_param *)ble_fp->TXTOG & BIT(0))


static struct ble_hw_base ble_base SEC(.btmem_highly_available);

struct baseband{
    struct list_head head;
};

static struct baseband bb SEC(.btmem_highly_available);

struct ble_handle {
	u8 store_regs;
	u32 fine_cnt;
	u32 regs[16];
};

static struct ble_handle  handle SEC(.btmem_highly_available);

#define __this  (&handle)

#define T_IFS        150
#define T_RXDLY      9
#define T_TXDLY      5
#define T_BSBCNT_DLY 3
#define T_WAIT       30  //WAIT CNT
#define T_RXSTAT     10                       //MIN 4US, NEED MORE MARGIN FOR ACCESS RAM
#define T_TXSTAT     5                        //MIN 0.5US, NEED MORE MARGIN FOR ACCESS RAM

#define T_RXTX      (T_IFS - T_RXDLY - T_TXDLY - T_BSBCNT_DLY) 
#define T_TXRX      (T_IFS + T_TXDLY - T_BSBCNT_DLY - 2)        //TIFS-2
#define T_IFSCNT    (T_TXRX | (T_RXTX<<8))

#define T_TXEN      (T_RXTX - T_WAIT - T_RXSTAT)
#define T_RXEN      (T_TXRX - T_WAIT - T_TXSTAT)

#define TXRXEN      (T_TXEN | (T_RXEN<<8))

#define T_TXLDO_MAX (T_TXEN + T_WAIT)
#define T_RXLDO_MAX (T_RXEN + T_WAIT)

#if (TXLDO_T > T_TXLDO_MAX)
#error "error setting TXLDO_T > T_TXLDO_MAX"
#endif

#if (RXLDO_T > T_RXLDO_MAX)
#error "error setting RXLDO_T > T_RXLDO_MAX"
#endif


#define TXPWRUP     (PLL_T + PLL_RST)
#define RXPWRUP     (PLL_T + PLL_RST)

#define SLOT_UNIT_US	625

#define MASTER_WINDEN_MAX   (SLOT_UNIT_US - TXPWRUP - T_WAIT - 40)
#define SLAVE_WINDEN_MAX    (SLOT_UNIT_US - RXPWRUP - T_WAIT - 40)

/********************************************************************************/
/*
 *                   HW layer Mempool
 */
struct ble_rx * ble_hw_alloc_rx(struct ble_hw *hw, int len)
{
	struct ble_rx *rx;

	rx = lbuf_alloc(hw->lbuf_rx, RX_PACKET_SIZE(len));
    if(rx == NULL)
    {
        /* ASSERT(rx != NULL, "%s\n", "rx alloc err\n"); */
        /* rf_puts("flow control reasons : lack of receive buffer space\n"); */
        rf_putchar('>');
        return NULL;
    }
	ASSERT(((u32)rx & 0x03) == 0, "%s\n", "rx not align\n");

	memset(rx, 0, sizeof(*rx));

	return  rx;
}

static u32 ble_hw_rx_underload(struct ble_hw *hw)
{
	return lbuf_remain_len(hw->lbuf_rx, BLE_HW_RX_BUF_SIZE * 3);
}


static struct ble_tx * le_hw_alloc_tx(struct ble_hw *hw,
	   int type, int llid, int len)
{
	struct ble_tx *tx;

	tx = lbuf_alloc(hw->lbuf_tx, TX_PACKET_SIZE(len));

	ASSERT(tx != NULL, "%s\n", "RF tx alloc err\n");
	memset(tx, 0, sizeof(*tx));
	tx->type = type;
	tx->llid = llid;
    /* rf_deg("hw alloc tx : %x\n", tx->len); */
    tx->len = len;
    /* rf_deg("hw alloc tx 2 : %x\n", tx->len); */

	return tx;
}


u8 pmalloc_rx[BLE_HW_NUM][BLE_HW_RX_SIZE];
u8 pmalloc_tx[BLE_HW_NUM][BLE_HW_TX_SIZE];

//features 4.2 fix maximum buffer size for hw
static void ble_rx_init(struct ble_hw *hw)
{
	hw->lbuf_rx = lbuf_init(pmalloc_rx[HW_ID(hw)], BLE_HW_RX_SIZE);
	ASSERT(((u32)hw->rx_buf[0] & 0x03)==0, "%x\n", hw->rx_buf[0]);
	ASSERT(((u32)hw->rx_buf[1] & 0x03)==0, "%x\n", hw->rx_buf[0]);
	hw->ble_fp.RXPTR0 = PHY_TO_BLE(hw->rx_buf[0] + sizeof(struct ble_rx));
	hw->ble_fp.RXPTR1 = PHY_TO_BLE(hw->rx_buf[1] + sizeof(struct ble_rx));
}

static void ble_tx_init(struct ble_hw *hw)
{
	hw->tx[0] = NULL;
	hw->tx[1] = NULL;
	ASSERT(((u32)hw->tx_buf[0] & 0x03)==0, "%x\n", hw->tx_buf[0]);
	ASSERT(((u32)hw->tx_buf[1] & 0x03)==0, "%x\n", hw->tx_buf[0]);
	hw->lbuf_tx = lbuf_init(pmalloc_tx[HW_ID(hw)], BLE_HW_TX_SIZE);
}

static void ble_hw_tx(struct ble_hw *hw, struct ble_tx *tx, int index)
{
    u8 sn;
	if (index == 0){
		if (tx->len){
			hw->ble_fp.TXPTR0 = PHY_TO_BLE(tx->data);
		}
		hw->ble_fp.TXAHDR0 = (tx->rxadd<<5)|(tx->txadd<<4)|tx->type;
        /* hw->ble_fp.TXDHDR0 = (tx->len<<8)|(tx->md<<3)|(tx->sn<<2)|tx->llid; */
        sn = hw->ble_fp.TXDHDR0 & BIT(2);
        hw->ble_fp.TXDHDR0 = (tx->len<<8)|(tx->md<<3)|sn|tx->llid;
	} else {
		if (tx->len){
			hw->ble_fp.TXPTR1 = PHY_TO_BLE(tx->data);
		}
		hw->ble_fp.TXAHDR1 = (tx->rxadd<<5)|(tx->txadd<<4)|tx->type;
        /* hw->ble_fp.TXDHDR1 = (tx->len<<8)|(tx->md<<3)|(tx->sn<<2)|tx->llid; */
        sn = hw->ble_fp.TXDHDR1 & BIT(2);
        hw->ble_fp.TXDHDR1 = (tx->len<<8)|(tx->md<<3)|sn|tx->llid;
	}
}

/********************************************************************************/
/*
 *                   HW Baisc Sfr Setting 
 */
static void __set_init_end(struct ble_hw *hw)
{
	//不再接收adv包
	BLE_ANCHOR_CON1 = (1<<5);      
	BLE_ANCHOR_CON0 = (9<<12)|(HW_ID(hw)<<8)|(1<<2)|1; 
}

static void __set_anchor_cnt(struct ble_hw *hw, int slot)
{
	BLE_ANCHOR_CON1 = 0;
	BLE_ANCHOR_CON0 = (0<<12)|(HW_ID(hw)<<8)|(1<<2)|1; 

	BLE_ANCHOR_CON1 = (1<<15)|(slot);       /*anchor_en/anchor_cnt N*625us*/ 
	BLE_ANCHOR_CON0 = (0<<12)|(HW_ID(hw)<<8)|(1<<2)|1;
}

static void __set_interval(struct ble_hw *hw, int interval, int rand)
{
	BLE_ANCHOR_CON1 = (rand<<15)|(interval/SLOT_UNIT_US);
	BLE_ANCHOR_CON0 = (1<<12)|(HW_ID(hw)<<8)|(1<<2)|1;
}

static void __set_hw_state(struct ble_hw *hw, int state,
	   	int latency_en, int latency)
{
	hw->state = state;
	hw->latency = latency;
	BLE_ANCHOR_CON1 = (state<<12)|((latency_en)<<11)|latency;
	BLE_ANCHOR_CON0 = (2<<12)|(HW_ID(hw)<<8)|(1<<2)|1;
}

static void __set_latency_suspend(struct ble_hw *hw)
{
	BLE_ANCHOR_CON1 = (hw->state<<12);
	BLE_ANCHOR_CON0 = (2<<12)|(HW_ID(hw)<<8)|(1<<2)|1;
}

static void __set_latency_resume(struct ble_hw *hw)
{
	BLE_ANCHOR_CON1 = (hw->state<<12)|((!!hw->latency)<<11)|hw->latency;
	BLE_ANCHOR_CON0 = (2<<12)|(HW_ID(hw)<<8)|(1<<2)|1;
}

static void __set_event_count(struct ble_hw *hw, int count)
{
	BLE_ANCHOR_CON1 = count;     //conn_event_count
	BLE_ANCHOR_CON0 = (3<<12)|(HW_ID(hw)<<8)|(1<<2)|1;
}

static void __set_winoffset(struct ble_hw *hw, int winoffset)
{
    if (winoffset)
    {
        BLE_ANCHOR_CON1 = (1<<15)|winoffset;  //N*625us
    }
    else
    {
        BLE_ANCHOR_CON1 = (0<<15)|winoffset;  //N*625us
    }
	BLE_ANCHOR_CON0= (4<<12)|(HW_ID(hw)<<8)|(1<<2)|1;
}

static void __set_event_instant(struct ble_hw *hw, int instant)
{
	BLE_ANCHOR_CON1 =  instant;
	BLE_ANCHOR_CON0 = (5<<12) | (HW_ID(hw)<<8) | (1<<2)  | 1;
}

static void __set_adv_channel_index(struct ble_hw *hw, u8 index)
{
    BLE_ANCHOR_CON1 = (0<<15)|(1<<14)|(1<<8)|index;
    BLE_ANCHOR_CON0 = (6<<12)|(HW_ID(hw)<<8)|(1<<2)|1;
}

static void __set_scan_channel_index(struct ble_hw *hw, u8 index)
{
    BLE_ANCHOR_CON1 = (0<<15)|(1<<13)|(1<<8)|index;
    BLE_ANCHOR_CON0 = (6<<12)|(HW_ID(hw)<<8)|(1<<2)|1;
}

static void __set_conn_channel_index(struct ble_hw *hw, u16 hop)
{
    BLE_ANCHOR_CON1 = (1<<15)|hop;
    BLE_ANCHOR_CON0 = (6<<12)|(HW_ID(hw)<<8)|(1<<2)|1;
}

static void __set_fixed_channel_index(struct ble_hw *hw, u8 index)
{
    BLE_ANCHOR_CON1 = (1<<13)|index;
    BLE_ANCHOR_CON0 = (6<<12)|(HW_ID(hw)<<8)|(1<<2)|1;
}

static void __set_bitoffset(struct ble_hw *hw, int bitoffset)
{
    BLE_ANCHOR_CON1 = bitoffset;
    BLE_ANCHOR_CON0 = (7<<12)|(HW_ID(hw)<<8)|(1<<2)|1;
}

static void __set_adv_random_us(struct ble_hw *hw)
{
	BLE_ANCHOR_CON1 = (1<<14);  //N*625us
	BLE_ANCHOR_CON0 = (8<<12)|(HW_ID(hw)<<8)|(1<<2)|1;
    /* BLE_CON0 |= BIT(3);     //rand fast en */
}

static void __set_adv_random_slot(struct ble_hw *hw)
{
	/* BLE_ANCHOR_CON1 = (1<<15)|(slot_rand_max<<8)|slot_rand_min;  //N*625us */
	BLE_ANCHOR_CON1 = (1<<15)|(1<<14);  //N*625us
	BLE_ANCHOR_CON0 = (8<<12)|(HW_ID(hw)<<8)|(1<<2)|1;
}

static void __disable_adv_random(struct ble_hw *hw)
{
	BLE_ANCHOR_CON1 = 0;
	BLE_ANCHOR_CON0 = (8<<12)|(HW_ID(hw)<<8)|(1<<2)|1;
}

static void ble_hw_enable(struct ble_hw *hw, int slot)
{ 
    __set_anchor_cnt(hw, slot);
    //clr pending
	SFR(BLE_INT_CON1, HW_ID(hw), 1, 1);         /*open event int*/ 
	SFR(BLE_INT_CON1, HW_ID(hw)+8, 1, 1);         /*open event int*/ 

	SFR(BLE_INT_CON0, HW_ID(hw), 1, 1);         /*open event int*/ 
	SFR(BLE_INT_CON0, (HW_ID(hw)+8), 1, 1);     /*open rx int*/ 
}

static void ble_hw_disable(struct ble_hw *hw)
{
	SFR(BLE_INT_CON0, HW_ID(hw), 1, 0);  		/*open event int*/ 
	SFR(BLE_INT_CON0, (HW_ID(hw)+8), 1, 0);  	/*open rx int*/   
}

void __debug_interval(struct ble_hw *hw)
{
    BLE_ANCHOR_CON0 = (1<<12) | (HW_ID(hw)<<8) | BIT(1);
    rf_deg("1 - BLE_ANCHOR_CON0 : %02x - %04x\n", HW_ID(hw), BLE_ANCHOR_CON2);
    rf_deg("ADVIDX : %04x\n", hw->ble_fp.ADVIDX);
	BLE_ANCHOR_CON0 = (0<<12) | (HW_ID(hw)<<8) | BIT(1);
    rf_deg("0 - BLE_ANCHOR_CON0 : %04x\n", BLE_ANCHOR_CON2 & 0x7fff);
}

static void __store_ble_regs(struct ble_hw *hw)
{
	int i;

	if (__this->store_regs == 0)
	{
		__this->store_regs = 1;

		memcpy(__this->regs, (u8 *)BLE_AHB_BASE_ADR, 6*4);
		__this->regs[0] &= ~BIT(0);
		__this->regs[6] = BLE_INT_CON0;
	}

	for (i=1; i<10; i++)
	{
		BLE_ANCHOR_CON0 = (i<<12) | (HW_ID(hw)<<8) | BIT(1);
		hw->regs[i] = BLE_ANCHOR_CON2;
	}
}

static void __restore_ble_regs(struct ble_hw *hw)
{
	int i;

	if (__this->store_regs == 1)
	{
		__this->store_regs = 0;

		memcpy((u8 *)BLE_AHB_BASE_ADR, __this->regs, 6*4);
		BLE_INT_CON0 = __this->regs[6];
	}

	BLE_ANCHOR_CON1 = (hw->regs[9]>>7);
	BLE_ANCHOR_CON0 = (1<<12) | (HW_ID(hw)<<8) | BIT(0);

	BLE_ANCHOR_CON1 = BIT(6)| (hw->regs[9]&0x37);
	BLE_ANCHOR_CON0 = (9<<12) | (HW_ID(hw)<<8) | BIT(0);

	for (i=1; i<9; i++)
	{
		BLE_ANCHOR_CON1 = hw->regs[i];
		BLE_ANCHOR_CON0 = (i<<12) | (HW_ID(hw)<<8) | BIT(0);
	}
}

static u32 __power_get_timeout(void *priv)
{
	u32 clkn_cnt;
	struct ble_hw *hw = (struct ble_hw *)priv;

	//hw busy
	if (BLE_DEEPSL_CON & BIT(1))
	{
		rf_putchar('~');
		return 0;
	}
	BLE_ANCHOR_CON0 = (0<<12) | (HW_ID(hw)<<8) | BIT(1);
    /* rf_deg("0 - BLE_ANCHOR_CON0 : %04x\n", BLE_ANCHOR_CON2 & 0x7fff); */

	//BLE_ANCHOR_CON0 = (0<<12) | (HW_ID(hw)<<8) | BIT(1);
	clkn_cnt = BLE_ANCHOR_CON2 & 0x7fff;

    return clkn_cnt < 4 ? 0 : (clkn_cnt - 4)*SLOT_UNIT_US;
}

static u32 __get_slots_before_next_instant(void *priv)
{
	u32 clkn_cnt;
	struct ble_hw *hw = (struct ble_hw *)priv;

	BLE_ANCHOR_CON0 = (0<<12) | (HW_ID(hw)<<8) | BIT(1);
    /* rf_deg("0 - BLE_ANCHOR_CON0 : %04x\n", BLE_ANCHOR_CON2 & 0x7fff); */

	//BLE_ANCHOR_CON0 = (0<<12) | (HW_ID(hw)<<8) | BIT(1);
	clkn_cnt = BLE_ANCHOR_CON2 & 0x7fff;

    return clkn_cnt < 14 ? 0 : (clkn_cnt - 4)*SLOT_UNIT_US;
}

static void __power_suspend_probe(void *priv)
{
	BLE_DEEPSL_CON &= ~BIT(2);
	BLE_DEEPSL_CON |= BIT(5)|BIT(0);
}

static void __power_suspend_post(void *priv, u32 usec)
{
        /* rf_putchar('a'); */
	struct ble_hw *hw = (struct ble_hw *)priv;

	while(!(BLE_DEEPSL_CON & BIT(3)));  // baseband stop
        /* rf_putchar('b'); */
    DEBUG_IO_1(0);
    DEBUG_IO_1(1);
	while(BT_PHCOM_CNT & BIT(11));

	__this->fine_cnt = usec + (BT_PHCOM_CNT&0x7ff)*SLOT_UNIT_US/10000;

	u32 clkn_cnt;
	BLE_ANCHOR_CON0 = (0<<12) | (HW_ID(hw)<<8) | BIT(1);
	clkn_cnt = (BLE_ANCHOR_CON2 & 0x7fff)*SLOT_UNIT_US;

    /* rf_deg("clk_cnt : %08d / fine_cnt - %08d / usec - %08d\n", clkn_cnt, __this->fine_cnt, usec); */

    ASSERT((clkn_cnt > __this->fine_cnt), "%s\n", __func__);

}

static void __power_resume(void *priv)
{

}

//request power off
static void __power_off_probe(void *priv)
{
	BLE_DEEPSL_CON &= ~BIT(2);
	BLE_DEEPSL_CON |= BIT(5)|BIT(0);
}

static void __power_off_post(void *priv, u32 usec)
{
	struct ble_hw *hw = (struct ble_hw *)priv;

    //wait dual module all idle
	while(!(BLE_DEEPSL_CON & BIT(3)));
	while(BT_PHCOM_CNT & BIT(11));

    __this->fine_cnt = BLE_FINETIMECNT + usec + (BT_PHCOM_CNT&0x7ff)*SLOT_UNIT_US/10000;

	BLE_ANCHOR_CON0 = (0<<12) | (HW_ID(hw)<<8) | BIT(1);
	hw->clkn_cnt = BLE_ANCHOR_CON2 & 0x7fff;

	__store_ble_regs(hw);
}

static void __power_on(void *priv)
{
    /* rf_puts("__power_on\n"); */
	struct ble_hw *hw = (struct ble_hw *)priv;

	BLE_DEEPSL_CON = BIT(5)|BIT(0);

	BLE_ANCHOR_CON1 = BIT(15) | (hw->clkn_cnt+1);
	BLE_ANCHOR_CON0 = (0<<12) | (HW_ID(hw)<<8) | BIT(0);

	__restore_ble_regs(hw);

	BLE_CON0 |= BIT(0);
}


static const struct bt_power_operation ble_power_ops  = {
	.get_timeout 	= __power_get_timeout,
	.suspend_probe 	= __power_suspend_probe,
	.suspend_post 	= __power_suspend_post,
	.resume 		= __power_resume,
	.off_probe 		= __power_off_probe,
	.off_post  		= __power_off_post,
	.on 			= __power_on,
};

static inline void ble_power_get(struct ble_hw *hw)
{
	hw->power_ctrl = bt_power_get(hw, &ble_power_ops);
}

static inline void ble_power_on(struct ble_hw *hw)
{
	bt_power_on(hw->power_ctrl);
}

static inline void ble_power_off(struct ble_hw *hw)
{
	bt_power_off(hw->power_ctrl);
}

static inline void ble_power_put(struct ble_hw *hw)
{
	bt_power_put(hw->power_ctrl);
}

static inline void ble_all_power_on()
{
	int i;

	for (i=0; i<8; i++) {
		if (ble_base.inst[i]){
			ble_power_on(&ble_base.hw[i]);
            /* __debug_interval(&ble_base.hw[i]); */
		}
	}
}
/********************************************************************************/
/*
 *                   HW Extend Sfr Setting 
 */

static void __set_winsize(struct ble_hw *hw, int winsize, int nwinsize)
{
    /* rf_deg("\nwinsize : %x - 1 : %x \n", winsize, nwinsize); */
	hw->ble_fp.WINCNTL0 = nwinsize;//N*us 
    //50us Active Clock Accuracy / Sleep Clock Accuracy
	hw->ble_fp.WINCNTL1 = (winsize<<8)|(nwinsize>>16);    //50 depend on 
}


//only slave
static void __set_widen(struct ble_hw *hw, int widen)
{
    u16 slot;
    u16 us;

    slot = (widen / SLOT_UNIT_US);
    us = (widen % SLOT_UNIT_US);

    if ((us > SLAVE_WINDEN_MAX))
    {
        slot += 1;
        us = 0;
    }
    //windowWidening = ((masterSCA + slaveSCA) / 1000000) * timeSinceLastAnchor
    hw->ble_fp.WIDEN0 = slot;                // [11:0] slotoffset_trim
    //[12:15] only establish connetion & winoffset
    //[11:0] connetion interval
    hw->ble_fp.WIDEN1 = (5<<12) | us; //[15:12]slave_Rx_first_bitoffset
    /* rf_deg("\nwiden0 : %x - 1 : %x \n", hw->ble_fp.WIDEN0, hw->ble_fp.WIDEN1); */
}

static void __set_local_addr(struct ble_hw *hw, u8 addr_type, const u8 *addr)
{
    rf_puts(__func__);rf_buf(addr, 6);
    hw->ble_fp.FORMAT |= BIT(3);    //set AdvA/ScanA/InitA from LOCALADR

    hw->local.addr_type = (addr_type) ? 1 : 0;

	memcpy(hw->local.addr, addr, 6);

	hw->ble_fp.LOCALADRL = (addr[1] << 8) + addr[0];
	hw->ble_fp.LOCALADRM = (addr[3] << 8) + addr[2];
	hw->ble_fp.LOCALADRU = (addr[5] << 8) + addr[4];
}

static void __set_local_addr_ram(struct ble_hw *hw, u8 addr_type, const u8 *addr)
{
    hw->ble_fp.FORMAT &= ~BIT(3);       //set AdvA/ScanA/InitA from RAM
    hw->local.addr_type = (addr_type) ? 1 : 0;

	memcpy(hw->local.addr, addr, 6);
	rf_deg("\n--func=%s\n", __FUNCTION__);
	rf_buf(hw->local.addr, 6);

    //next tx buf to be send
    u16 *ptr;

    ptr = (u32)hw->ble_fp.TXPTR0 + ((u32)&ble_base);
	//rf_deg("ptr0=0x%x\n", ptr);
	memcpy(ptr, addr, 6);

    ptr = (u32)hw->ble_fp.TXPTR1 + ((u32)&ble_base);
	//rf_deg("ptr1=0x%x\n", ptr);
	memcpy(ptr, addr, 6);
}

static void __show_remote_addr(struct ble_hw *hw)
{
    put_u16hex(hw->ble_fp.TARGETADRU);
    put_u16hex(hw->ble_fp.TARGETADRM);
    put_u16hex(hw->ble_fp.TARGETADRL);
}

static void __set_peer_addr(struct ble_hw *hw, u8 addr_type, const u8 *addr)
{
    hw->peer.addr_type = (addr_type) ? 1 : 0;

	memcpy(hw->peer.addr, addr, 6);

	hw->ble_fp.TARGETADRL = (addr[1] << 8) | addr[0];
	hw->ble_fp.TARGETADRM = (addr[3] << 8) | addr[2];
	hw->ble_fp.TARGETADRU = (addr[5] << 8) | addr[4];
}

#define get_pll_param_for_frq(x)    get_bta_pll_bank(x)



static void __set_channel_map(struct ble_hw *hw, u8 *channel)
{
	int i;
	int num = 0;
	int frq_mid;   // 89 mid 2402  2M MID_FREQ_UP;
	struct ble_param *ble_fp = &hw->ble_fp;
	u8 pll_param = 0;
	u8 m[3]={0, 24, 78};
	//	u8 *channel = hw->conn_param.channel;

	int k = 0;
	for(i=0; i<=36; i++)
	{
		if(i==0) 
		{ //2404
			frq_mid = 81;
			k=0;
		} 
		else if(i==11)  
		{ //2428
			frq_mid =  83;
			k=2;
		}
#ifndef FPGA
		pll_param = get_pll_param_for_frq(frq_mid+i*2);
#endif
		ble_fp->FRQ_IDX0[i] = i;
		ble_fp->FRQ_TBL0[i] = (i + 1) * 2 + k;
		if (channel[i>>3] & BIT(i&0x7)){
			ble_fp->FRQ_IDX1[num] = i;
			ble_fp->FRQ_TBL1[num++] = (i + 1) * 2 + k; 
		}
	}
    for (i=0; i<3; i++)
    {
        int j = m[i];
#ifndef FPGA
        pll_param = get_pll_param_for_frq(81-2+j*2);
#endif
        ble_fp->FRQ_IDX0[37+i] = 37+i;
        ble_fp->FRQ_TBL0[37+i] = j;
        ble_fp->FRQ_IDX1[37+i] = 37+i;
        ble_fp->FRQ_TBL1[37+i] = j;
    }

	ble_fp->CHMAP0 = (channel[1]<<8) | channel[0];
	ble_fp->CHMAP1 = (channel[3]<<8) | channel[2];
	ble_fp->CHMAP2 = (num<<5) | (channel[4]&0x1f);
}

static void __set_adv_channel_map_patch(struct ble_hw *hw, u8 channel_map)
{
	struct ble_param *ble_fp = &hw->ble_fp;

	int i;
	u8 pll_param = 0;
	u8 m[3]={0, 24, 78};

    if (channel_map != 0x5)
        return;
    rf_puts("****adv channel map patch***\n");
    for (i=0; i<3; i++)
    {
        int j = m[i];
#ifndef FPGA
        pll_param = get_pll_param_for_frq(81-2+j*2);
#endif
        if (i == 0)
        {
            ble_fp->FRQ_IDX0[37] = 37;
            ble_fp->FRQ_TBL0[37] = j;
        }
        else if (i == 2)
        {
            ble_fp->FRQ_IDX0[38] = 39;
            ble_fp->FRQ_TBL0[38] = j;
        }
        ble_fp->FRQ_IDX1[37+i] = 37+i;
        ble_fp->FRQ_TBL1[37+i] = j;
    }
}

static void __set_scan_req_enable(struct ble_param *ble_fp);
static void __set_scan_req_disable(struct ble_param *ble_fp);
static void __set_scan_active(struct ble_hw *hw, u8 active)
{
	struct ble_param *ble_fp = &hw->ble_fp;

    if ((active) && (hw->privacy_enable == 0)){
        __set_scan_req_enable(ble_fp);
    }
    else{
        __set_scan_req_disable(ble_fp);
    }
}

static void __set_connection_param(struct ble_hw *hw, 
		struct ble_conn_param *conn_param)
{
	int role;
	struct ble_param *ble_fp = &hw->ble_fp;


	ble_fp->ANCHOR = (1<<15); 		// updat event interval
	ble_fp->TXTOG &= ~BIT(1);      	// CLR NESN
	ble_fp->RXTOG = (0<<2)|(0<<1)|0; // rxmd rxseq/rxtog

	ble_fp->BDADDR0 = READ_BT16(conn_param->aa, 0);
	ble_fp->BDADDR1 = READ_BT16(conn_param->aa, 2);

	ble_fp->CRCWORD0 = READ_BT16(conn_param->crcinit, 0);
	ble_fp->CRCWORD1 = conn_param->crcinit[2];

    __set_event_count(hw, 0);

    __set_event_instant(hw, 0);

	if (hw->state == INIT_ST)  //master
	{
        BLE_ANCHOR_CON1 = (0<<15)| 0;
        BLE_ANCHOR_CON0 = (4<<12)|(HW_ID(hw)<<8)|(1<<2)|1;

        //1.25ms + winoffset
        //update anchor cnt must disable first
        __set_anchor_cnt(hw, (conn_param->winoffset*2+2+2));

        //----HW state 
		__set_hw_state(hw, MASTER_CONN_ST, !!conn_param->latency, conn_param->latency);
	}
	else
	{
        __disable_adv_random(hw);
        //----Winoffset
        __set_winoffset(hw, (conn_param->winoffset) ? conn_param->winoffset*2-1 : 0);
        /* rf_puts("winoffset : ");rf_u8hex(winoffset); */
        //----HW state 
		__set_hw_state(hw, SLAVE_CONN_ST, !!conn_param->latency, conn_param->latency);
        //----Channel map
        __set_widen(hw, conn_param->widening);
	}
    //----Winsize
    __set_winsize(hw, 50, conn_param->winsize*1250 + SLOT_UNIT_US);
    //----Interval
    __set_interval(hw, conn_param->interval*1250, 0);
    //----Channel map
    __set_channel_map(hw, conn_param->channel);
    //----Hop
    __set_conn_channel_index(hw, (conn_param->hop<<8)|conn_param->hop);
    //BLE_ANCHOR_CON0 = (7<<12)|(HW_ID(hw)<<8)|(1<<1)|0;

    /* rf_puts("\nbitoffset : ");put_u16hex(BLE_ANCHOR_CON2); */
}

static void __connection_update(struct ble_hw *hw, struct ble_conn_param *param)
{
	int winoffset;

	if (hw->state == MASTER_CONN_ST){
		winoffset = param->winoffset*2;
		__set_winsize(hw, 50, (param->winsize)*1250+SLOT_UNIT_US);
		__set_hw_state(hw, MASTER_CONN_ST, !!param->latency, param->latency);
	} else {
		winoffset = (param->winoffset) ? param->winoffset*2-1 : 0;
		__set_winsize(hw, 50, (param->winsize)*1250+SLOT_UNIT_US);
		__set_hw_state(hw, SLAVE_CONN_ST, !!param->latency, param->latency);
        __set_widen(hw, param->widening);
	}

    /* winoffset = 30;  */
	__set_winoffset(hw, winoffset);
	__set_interval(hw, param->interval*1250, 0);
}

static void __set_white_list_addr(struct ble_hw *hw, u8 addr_type, const u8 *addr)
{
	struct ble_param *ble_fp = &hw->ble_fp;

    rf_puts(__func__);

    addr_type = (addr_type) ? 1 : 0;
    ble_fp->FILTERCNTL = ble_fp->FILTERCNTL & 0x7f | (addr_type&0x1)<<8 | 1;
    ble_fp->FILTERCNTL |= BIT(4);

	ble_fp->WHITELIST0L = (addr[1] << 8) | addr[0];
	ble_fp->WHITELIST0M = (addr[3] << 8) | addr[2];
	ble_fp->WHITELIST0U = (addr[5] << 8) | addr[4];

    rf_deg("addr type %x\n", addr_type);

    rf_deg("addr L %x\n", ble_fp->WHITELIST0L);
    rf_deg("addr M %x\n", ble_fp->WHITELIST0M);
    rf_deg("addr U %x\n", ble_fp->WHITELIST0U);
}

static void __set_receive_encrypted(struct ble_hw *hw, u8 rx_enable)
{
    hw->encrypt.rx_enable = rx_enable;
    //reset when RECEIVE_UNENCRYPTED    
    if (hw->encrypt.rx_enable == 0x0)
    {
        hw->encrypt.rx_counter_l = 0x0;
        hw->encrypt.rx_counter_h = 0x0;
    }
}

static void __set_send_encrypted(struct ble_hw *hw, u8 tx_enable)
{
    hw->encrypt.tx_enable = tx_enable;
    //reset when SEND_UNENCRYPTED       
    if (hw->encrypt.tx_enable == 0x0)
    {
        hw->encrypt.tx_counter_l = 0x0;
        hw->encrypt.tx_counter_h = 0x0;
    }
}

//--------------Privacy

static void __set_hw_tx_enable(struct ble_param *ble_fp)
{
    ble_fp->FORMAT &= ~BIT(1);                
}

static void __set_hw_tx_disable(struct ble_param *ble_fp)
{
    ble_fp->FORMAT |= BIT(1);                
}


#define __set_scan_rsp_enable(fp)   __set_hw_tx_enable(fp)
#define __set_scan_rsp_disable(fp)  __set_hw_tx_disable(fp)

#define __set_conn_req_enable(fp)   __set_hw_tx_enable(fp)
#define __set_conn_req_disable(fp)  __set_hw_tx_disable(fp)

#define __set_tx_data_enable(fp)   __set_hw_tx_enable(fp)
#define __set_tx_data_disable(fp)  __set_hw_tx_disable(fp)

static void __set_scan_req_enable(struct ble_param *ble_fp)
{
    //active scan
    ble_fp->FORMAT &= ~(1<<8);
}

static void __set_scan_req_disable(struct ble_param *ble_fp)
{
    //passive scan
    ble_fp->FORMAT |= (1<<8);
}

static void __set_privacy_enable(struct ble_hw *hw, u8 enable)
{
    hw->privacy_enable = enable;
    
    //disable auto tx scan_rsp/conn_req/data
    rf_puts("lock scan_rsp\n");
    __set_hw_tx_disable(&hw->ble_fp);

    //disable auto tx scan_req
    rf_puts("lock scan_req\n");
    __set_scan_req_disable(&hw->ble_fp);
}

static void __set_hw_adv_expect_scan_rpa(struct ble_hw *hw, struct ble_rx *rx)
{
	struct ble_param *ble_fp = &hw->ble_fp;
    const u8 *addr;
    rf_puts(__func__);
    rf_buf(addr, 6);

    //ScanA
    addr = rx->data;

	ble_fp->WHITELIST0L = (addr[1] << 8) | addr[0];
	ble_fp->WHITELIST0M = (addr[3] << 8) | addr[2];
	ble_fp->WHITELIST0U = (addr[5] << 8) | addr[4];
    ble_fp->FILTERCNTL |= BIT(8);             //rxadd = random
    ble_fp->FILTERCNTL |= (BIT(3));          //white list enable scan_en

    rf_puts("unlock scan_rsp\n");
    __set_scan_rsp_enable(ble_fp);
}


static void __adv_state_handler(struct ble_hw *hw, struct ble_rx *rx)
{
    switch(rx->type)
    {
    case SCAN_REQ:
        __set_hw_adv_expect_scan_rpa(hw, rx);
        break;
    default:
        break;
    }
}

static void __set_hw_scan_expect_adv_rpa(struct ble_hw *hw, struct ble_rx *rx)
{
	struct ble_param *ble_fp = &hw->ble_fp;
    const u8 *addr;

    //passive scan
    if (hw->backoff.active == 0)
        return;

    rf_puts(__func__);
    //AdvA
    addr = rx->data;
    rf_buf(addr, 6);

	ble_fp->WHITELIST0L = (addr[1] << 8) | addr[0];
	ble_fp->WHITELIST0M = (addr[3] << 8) | addr[2];
	ble_fp->WHITELIST0U = (addr[5] << 8) | addr[4];
    ble_fp->FILTERCNTL |= BIT(8);            //rxadd = random
    ble_fp->FILTERCNTL |= BIT(3);            //white list enable scan_en

    __set_scan_req_enable(ble_fp);
}


static void __scan_state_handler(struct ble_hw *hw, struct ble_rx *rx)
{
    switch(rx->type)
    {
    case ADV_IND:
    case ADV_SCAN_IND:
        __set_hw_scan_expect_adv_rpa(hw, rx);
        break;
    default:
        break;
    }
}


static void __set_hw_init_expect_adv_rpa(struct ble_hw *hw, struct ble_rx *rx)
{
	struct ble_param *ble_fp = &hw->ble_fp;
    const u8 *addr;

    rf_puts(__func__);
    //AdvA
    addr = rx->data;
    rf_buf(addr, 6);

	hw->is_init_enter_conn_pass = 1;

	ble_fp->WHITELIST0L = (addr[1] << 8) | addr[0];
	ble_fp->WHITELIST0M = (addr[3] << 8) | addr[2];
	ble_fp->WHITELIST0U = (addr[5] << 8) | addr[4];
    ble_fp->FILTERCNTL |= BIT(8);            //rxadd = random
    ble_fp->FILTERCNTL |= BIT(4);            //white list enable conn_en

    __set_conn_req_enable(ble_fp);
}

static void __init_state_handler(struct ble_hw *hw, struct ble_rx *rx)
{
    switch(rx->type)
    {
    case ADV_DIRECT_IND:
    case ADV_IND:
        __set_hw_init_expect_adv_rpa(hw, rx);
        break;
    default:
        break;
    }
}

static void __set_hw_adv_tx_data(struct ble_hw *hw, struct ble_rx *rx)
{
	struct ble_param *ble_fp = &hw->ble_fp;

    __set_tx_data_enable(ble_fp);
}

static void __slave_conn_state_handler(struct ble_hw *hw, struct ble_rx *rx)
{
    switch(rx->type)
    {
    case CONNECT_REQ:
        __set_hw_adv_tx_data(hw, rx);
        break;
    default:
        break;
    }
}

static void __set_rpa_resolve_result(struct ble_hw *hw, struct ble_rx *rx, u8 res)
{
    if (hw->privacy_enable == 0)
        return;

    //RPA resolve fail
    if (res)
        return;

    switch(hw->state)
    {
    case ADV_ST:
        /* rf_puts("ADV_ST\n"); */
        __adv_state_handler(hw, rx);
        break;
    case SCAN_ST:
        rf_puts("SCAN_ST\n");
        __scan_state_handler(hw, rx);
        break;
    case INIT_ST:
        rf_puts("INIT_ST\n");
        __init_state_handler(hw, rx);
        break;
    case MASTER_CONN_ST:
        rf_puts("MASTER_CONN_ST\n");
        break;
    case SLAVE_CONN_ST:
        /* rf_puts("SLAVE_CONN_ST\n"); */
        __slave_conn_state_handler(hw, rx);
        break;
    }
}

//Match public AdvA/InitA
static void __set_addr_match_enable(struct ble_param *ble_fp)
{
    ble_fp->OPTCNTL &= ~BIT(4);
}

static void __set_addr_match_disable(struct ble_param *ble_fp)
{
    ble_fp->OPTCNTL |= BIT(4);
}

static void __set_rx_length(struct ble_hw *hw, u16 len)
{
	struct ble_param *ble_fp = &hw->ble_fp;

    ble_fp->RXMAXBUF = len + sizeof(struct ble_rx) + ENC_MIC_LEN;

    /* hw->rx_octets = len; */

    /* rf_deg("rx_octets : %x\n", hw->rx_octets); */
}

static void __set_tx_length(struct ble_hw *hw, u16 len)
{
    hw->tx_octets = len;

    rf_deg("tx_octets : %x\n", hw->tx_octets);
}

#ifdef FPGA
//===================================//
//sel: 1    auto_set agc
//sel: 0    set agc = inc (0~15)
//==================================//
static u16 __get_agc(u8 *agc, u8 sel, char inc)
{
	u16 ret;

	if(!sel) {
		*agc= inc;
	} else {
		if(BT_RSSIDAT0 < 700){
			*agc++;
		} else if(BT_RSSIDAT0 > 1200){
			*agc--;
		}
	}

	if(*agc < 0) {
		*agc = 0;
	} else if(*agc > 15) {
		*agc = 15;
	}

	switch(*agc)
	{
		case 0: //-1db
			ret = 0;
			break;
		case 1: //-1db
			ret = 2;
			break; 
		case 2: //-1db
			ret = (1<<2) | 2;
			break;
		case 3:  //-1db
			ret = 3;
			break;
		case 4: //2db
			ret = (1<<2) | 3;
			break;
		case 5: //5db
			ret = (1<<2) | 1;
			break;
		case 6: //8db
			ret = (2<<2) | 1;
			break;
		case 7:  //11db
			ret = (3<<2) | 1;
			break;
		case 8:  //14db
			ret = (1<<5) | (3<<2) | 1;
			break;
		case 9: //17db
			ret = (2<<5) | (3<<2) | 1;
			break;
		case 10: //20db
			ret = (4<<5) | (3<<2) | 1;
			break;
		case 11: //23db
			ret = (8<<5) | (3<<2) | 1;
			break;
		case 12: //26db
			ret = (1<<9) | (8<<5) | (3<<2) | 1;
			break;
		case 13: //29db
			ret = (2<<9) | (8<<5) | (3<<2) | 1;
			break;
		case 14: //32db
			ret = (4<<9) | (8<<5) | (3<<2) | 1;
			break;
		case 15: //35db
			ret = (8<<9) | (8<<5) | (3<<2) | 1;
			break;
	}

	return ret;
}


//===================================//
//sel 1:  inc: 0  auto_sub  inc: 1 auto_add
//sel 1:  set tx_pwr as inc (0 ~15)
//return: 0 set ok  1: min  2: max
//==================================//
static u16 __get_tx_power(u8 *power, u8 sel, char inc)
{
	char dat0, pwr_d;

	if(!sel) {
		*power = inc;
	} else {
		if(inc) {
			*power++;
		} else {
			*power--;
		}
	}

	if(*power < 0) {
		*power = 0;
	} else if(*power > 15) {
		*power = 15;
	}

	return (*power<<5)|0x1f;
}
#endif


static void __set_hw_frame_init(struct ble_hw *hw)
{
	u8 channel[5];
	struct ble_param *ble_fp = &hw->ble_fp;

	ble_fp->ANCHOR = (0<<15);                      // updat event interval
	ble_fp->BITOFF = (0<<15) | 0; // 1:use soft bitoffset  0: use hardware
	ble_fp->ADVIDX = (3<<14) | 5000;//ADV_PACKET_NUM: ADC_PACKET_interval
	ble_fp->WIDEN0 = 0;             // [11:0] slotoffset_trim
	ble_fp->WIDEN1 = (5<<12) | SLAVE_WINDEN_MAX; //[15:12]slave_Rx_first_bitoffset
	//[9:0]bitoffset_trim
	
	ble_fp->BDADDR0 = PUBILC_ACCESS_ADR0;
	ble_fp->BDADDR1 = PUBILC_ACCESS_ADR1;

	ble_fp->CRCWORD0 = 0x5555;
	ble_fp->CRCWORD1 = 0x55;
    ble_fp->FORMAT = BIT(5);
    ble_fp->OPTCNTL = BIT(3);

    /* ble_fp->ANLCNT0 = ((RXLDO_T-RXEN_T-3)<<24) | ((TXLDO_T-TXEN_T)<<16) | ((PLL_T-RXLDO_T-5)<<8) | (PLL_T-TXLDO_T-5);  */
    /* ble_fp->ANLCNT1 = (PLL_RST << 0);  */
    /* ble_fp->ANLCNT0 = (10<<24) | (10<<16) | (10<<8) | 10; */
    /* ble_fp->ANLCNT1 = (8 << 0); */
    SFR(BLE_RADIO_CON0, 0 , 10 , TXPWRUP);    //TXPWRUP_OPEN_PLL
    SFR(BLE_RADIO_CON0, 12 , 4 , 8);            //TXPWRPD_CLOSE_LOD
    SFR(BLE_RADIO_CON1, 0 , 10 , RXPWRUP);    //RXPWRUP_OPEN_PLL
	SFR(BLE_RADIO_CON2, 0 , 8 ,  T_TXEN);     //TXCNT_OPEN_LDO
	SFR(BLE_RADIO_CON2, 8 , 8 ,  T_RXEN);     //RXCNT_OPEN_LDO
	SFR(BLE_RADIO_CON3, 0 , 8 ,  T_WAIT);     //wait read data
#ifdef FPGA 
	/* ble_fp->RFTXCNTL0 = 0xFFFF;         //[14:0] TXPWR */
	/* ble_fp->RFTXCNTL1 = 0x2a;          //[5:0]  TX_RX */
	/* ble_fp->RFRXCNTL0 = (8<<9) | (8<<5) | (3<<2) | 1;     //[14:0] RXGAIN; */
	/* ble_fp->RFRXCNTL1 = 0x34;                             //[5:0]  TX_RX */

	/* ble_fp->RFCONFIG = (5<<4) | 0XA;                 //BT_OSC_CAP */
    /* SFR(ble_fp->TXPWER, 0, 5, 0X1f); // PA_CSEL  */
    /* SFR(ble_fp->TXPWER, 5, 4, 0Xf); // PA_GSEL  */
    /* SFR(ble_fp->RXGAIN0, 0, 2, 0X3); // LNA_GSEL  */
    /* SFR(ble_fp->RXGAIN0, 2, 3, 0X1); // LNA_ISEL  */
    /* SFR(ble_fp->RXGAIN0, 5, 4, 0X8); // BP_GA  */
    /* SFR(ble_fp->RXGAIN0, 9, 4, 0X8); // BP_GB  */
    /* SFR(ble_fp->RXTXSET, 0, 3, 0X1); // LNA_RSEL  */
    /* SFR(ble_fp->RXTXSET, 12, 3, 0X1); // LNA_RSEL  */
    /* ble_fp->ANLCNT0 = 0;  */
    /* ble_fp->ANLCNT1 = 0;  */
    /* ble_fp->MDM_SET = 0;  */
    /* ble_fp->OSC_SET = 0;  */
#else
    ble_agc_normal_set(hw, 0, 19);
    ble_txpwr_normal_set(hw, 0, 7);
#endif
	ble_fp->RFPRIOCNTL= (2<<8) | 30;                 //PRIO_INC  PRIO_INIT
	ble_fp->RFPRIOSTAT= 30;                         //PRIO_CURR

	ble_fp->TXTOG = (0<<1)|0;                    //nesn / txtog
	ble_fp->RXTOG = (0<<2)|(0<<1)|0;           //rxmd rxseq/rxtog

	ble_fp->TXAHDR0 = (0<<5)|(0<<4)|0;
	ble_fp->TXDHDR0 = (0<<8)|(0<<3)|(0<<2)|1;
	ble_fp->TXAHDR1 = (0<<5)|(0<<4)|0;
	ble_fp->TXDHDR1 = (0<<8)|(0<<3)|(0<<2)|1;

	/*ble_fp->WINCNTL0 = 200; //first_rx:{WINCNTL1[7:0],WINCNTL0[15:0]} N*us
	ble_fp->WINCNTL1 = (50<<8)|0; //packet_interval_rx:WINCNTL1[15:8] N*us*/
    ble_fp->RXMAXBUF = BLE_HW_MIN_RX_OCTETES + sizeof(struct ble_rx);//38;
    /* ble_fp->RXMAXBUF = 38; */
	ble_fp->IFSCNT = T_IFSCNT;                     //txifs / rxifs
	ble_fp->FILTERCNTL = 0;     //white list disable

	//ble_fp->LASTCHMAP = 37;               //unmap_channal set by hardware
	/*ble_fp->WINCNTL0 =  625*2;            //first_rx:{WINCNTL1[7:0],
	ble_fp->WINCNTL1 = (50<<8)|0; //packet_interval_rx:WINCNTL1[15:8]us*/

	/*ble_fp->LOCALADRL = (local_addr[1] << 8) + local_addr[0];
	ble_fp->LOCALADRM = (local_addr[3] << 8) + local_addr[2];
	ble_fp->LOCALADRU = (local_addr[5] << 8) + local_addr[4];*/

	ble_fp->TARGETADRL = 0;
	ble_fp->TARGETADRM = 0;
	ble_fp->TARGETADRU = 0;
#ifdef FPGA
    ble_fp->TXPWER = __get_tx_power(&hw->power, 0, 15);
    ble_fp->RXGAIN0 = __get_agc(&hw->agc, 0, 12);
#endif
	memset(channel, 0xff, 5);
	__set_channel_map(hw, channel);
}


static void __set_adv_device_filter_param(struct ble_hw *hw, u8 filter_policy)
{
	struct ble_param *ble_fp = &hw->ble_fp;

    u8 scan_en, conn_en;

    switch(filter_policy)
    {
    case 0:
        scan_en = 0;
        conn_en = 0;
        break;
    case 1:
        scan_en = 1;
        conn_en = 0;
        break;
    case 2:
        scan_en = 0;
        conn_en = 1;
        break;
    case 3:
        scan_en = 1;
        conn_en = 1;
        break;
    default:
        break;
    }
    //[3] scan en
    ble_fp->FILTERCNTL = ble_fp->FILTERCNTL & 0xf7 | (scan_en&0x1)<<3;

    //[4] conn en
    ble_fp->FILTERCNTL = ble_fp->FILTERCNTL & 0xef | (conn_en&0x1)<<4;

    /* rf_deg("FILTERCNTL %x\n", ble_fp->FILTERCNTL); */
}

static void __set_scan_device_filter_param(struct ble_hw *hw, u8 filter_policy)
{
	struct ble_param *ble_fp = &hw->ble_fp;

    u8 scan_en;

    switch(filter_policy)
    {
    case 0:
        scan_en = 0;
        break;
    case 1:
        scan_en = 1;
        break;
    case 2:
        scan_en = 0;
        break;
    case 3:
        scan_en = 1;
        break;
    default:
        break;
    }
    //[3] scan en
    ble_fp->FILTERCNTL = ble_fp->FILTERCNTL & 0xf7 | (scan_en&0x1)<<3;

    /* rf_deg("FILTERCNTL %x\n", ble_fp->FILTERCNTL); */
}

static void __set_init_device_filter_param(struct ble_hw *hw, u8 filter_policy)
{
	struct ble_param *ble_fp = &hw->ble_fp;

    u8 conn_en;

    switch(filter_policy)
    {
    case 0:
        conn_en = 0;
        break;
    case 1:
        conn_en = 1;
        break;
    default:
        break;
    }
    //[4] conn en
    ble_fp->FILTERCNTL = ble_fp->FILTERCNTL & 0xef | (conn_en&0x1)<<4;

    /* rf_deg("FILTERCNTL %x\n", ble_fp->FILTERCNTL); */
}

static void ble_hw_encrypt_check(struct ble_hw *hw)
{
	int index;
	struct ble_tx *tx;
	struct ble_encrypt *encrypt = &hw->encrypt;

	if (encrypt->rx_enable) {
		return;
	}
    return;
    if (hw->state == SLAVE_CONN_ST){
        if (hw->rx_ack) {
            index = hw->ble_fp.TXTOG & BIT(0);
            tx = hw->tx[index];
            if (tx && tx->llid==3 && tx->data[0] == 0x05){
                encrypt->tx_enable = 1;//LL_START_ENC_REQ:
            }
        }
    }
}

static void ble_hw_encrpty(struct ble_hw *hw, struct ble_tx *tx)
{
    struct ble_encrypt *encrypt = &hw->encrypt;

	u8 tag[4] = {0};
	u8 nonce[16];
	u8 pt[40] = {0};
	u8 head = tx->llid;
	u8 skd[16];

	if (!encrypt->tx_enable){
		return;
	}

	nonce[0] = encrypt->tx_counter_l;
	nonce[1] = encrypt->tx_counter_l>>8;
	nonce[2] = encrypt->tx_counter_l>>16;
	nonce[3] = encrypt->tx_counter_l>>24;
    nonce[4] = encrypt->tx_counter_h;
    if (hw->state == MASTER_CONN_ST)
    {
        //Direction Bit
        nonce[4] |= BIT(7);
    }
	memcpy(nonce+5, encrypt->iv, 8);

	rijndael_setup(encrypt->skd);
	ccm_memory(nonce, &head, 1, tx->data, tx->len, pt, tag, 0);

	encrypt->tx_counter_l++;
	memcpy(tx->data, pt, tx->len);
	memcpy(tx->data+tx->len, tag, 4);
	tx->len += 4;
}

static void ble_hw_txdecrypt(struct ble_hw *hw, struct ble_tx *tx)
{
    struct ble_encrypt *encrypt = &hw->encrypt;

	if (encrypt->tx_enable && tx->len!=0)
	{
		u8 tag[4] = {0};
		u8 nonce[16];
		u8 pt[40] = {0};
		u8 head = tx->llid;
		u8 skd[16];

		nonce[0] = encrypt->rx_counter_l;
		nonce[1] = encrypt->rx_counter_l>>8;
		nonce[2] = encrypt->rx_counter_l>>16;
		nonce[3] = encrypt->rx_counter_l>>24;
		nonce[4] = encrypt->rx_counter_h;
        if (hw->state == SLAVE_CONN_ST)
        {
            nonce[4] |= BIT(7);
        }
		memcpy(nonce+5, encrypt->iv, 8);

		if (tx->llid != 1){
			encrypt->tx_counter_l++;
		}

		tx->len -= 4;
		rijndael_setup(encrypt->skd);
		ccm_memory(nonce, &head, 1, pt, tx->len, tx->data,  tag, 1);
		memcpy(tx->data, pt, tx->len);
		if (memcmp(tag, tx->data+tx->len, 4)){
			rf_deg("no_match: %d\n", tx->llid);
		}
	}
}

static void ble_hw_decrypt(struct ble_hw *hw, struct ble_rx *rx)
{
    struct ble_encrypt *encrypt = &hw->encrypt;

	if (encrypt->rx_enable && rx->len!=0)
	{
		u8 tag[4] = {0};
		u8 nonce[16];
		u8 pt[40] = {0};
		u8 head = rx->llid;
		u8 skd[16];

		nonce[0] = encrypt->rx_counter_l;
		nonce[1] = encrypt->rx_counter_l>>8;
		nonce[2] = encrypt->rx_counter_l>>16;
		nonce[3] = encrypt->rx_counter_l>>24;
		nonce[4] = encrypt->rx_counter_h;
        if (hw->state == SLAVE_CONN_ST)
        {
            nonce[4] |= BIT(7);
        }
		memcpy(nonce+5, encrypt->iv, 8);

		if (rx->llid != 1){
			encrypt->rx_counter_l++;
		}

		rx->len -= 4;
		rijndael_setup(encrypt->skd);
		ccm_memory(nonce, &head, 1, pt, rx->len, rx->data,  tag, 1);
		memcpy(rx->data, pt, rx->len);
		if (memcmp(tag, rx->data+rx->len, 4)){
			rf_deg("no_match: %d\n", rx->llid);
            printf("\n*******NO Match : %d\n", rx->llid);
		}
	}
}

/********************************************************************************/
/*
 *                   HW Interrupt
 */
static bool ble_rx_pdus_filter(struct ble_hw *hw, struct ble_rx *rx)
{
	struct ble_param *ble_fp = &hw->ble_fp;

    switch (hw->state)
    {
        case ADV_ST:
            switch (rx->type)
            {
            case ADV_IND:
            case ADV_SCAN_IND:
            case ADV_NONCONN_IND:
            case ADV_DIRECT_IND:
            case SCAN_RSP:
                return FALSE;

            case CONNECT_REQ:
                //verify InitA
                if ((hw->privacy_enable == 0)
                    && (hw->tx[0]->type == ADV_DIRECT_IND))
                {
                    if ((hw->peer.addr_type != rx->txadd) 
                        || (memcmp(hw->peer.addr, rx->data, 6)))
                    {
                        return FALSE;
                    }
                    //privacy RPA bypass
                }
            case SCAN_REQ:
                //verify AdvA
                if ((hw->local.addr_type != rx->rxadd) 
                    || (memcmp(hw->local.addr, rx->data + 6, 6)))
                {
                    return FALSE;
                }
            }
            break;

        case SCAN_ST:
            switch (rx->type)
            {
            case ADV_IND:
            case ADV_SCAN_IND:
            case ADV_DIRECT_IND:
            case ADV_NONCONN_IND:
            case SCAN_RSP:
                break;

            case SCAN_REQ:
            case CONNECT_REQ:
                return FALSE;
            }
            break;

        case INIT_ST:
            switch (rx->type)
            {
            case ADV_DIRECT_IND:
                //verify InitA
                if (hw->privacy_enable == 0)
                {
                    if ((hw->local.addr_type != rx->rxadd) 
                        || (memcmp(hw->local.addr, rx->data + 6, 6)))
                    {
                        return FALSE;
                    }
                    //privacy RPA bypass
                }
            case ADV_IND:
            case ADV_SCAN_IND:
            case ADV_NONCONN_IND:
            case SCAN_RSP:
                //verify AdvA
                /* if (hw->privacy_enable == 0) */
                /* { */
                    if ((hw->peer.addr_type != rx->txadd) 
                        || (memcmp(hw->peer.addr, rx->data, 6)))
                    {
                        /* rf_u8hex(hw->peer.addr_type); */
                        /* rf_buf(hw->peer.addr, 6); */
                        /* rf_u8hex(rx->txadd); */
                        /* rf_buf(rx->data, 6); */

                        /* rf_u8hex((hw->peer.addr_type != rx->txadd)); */
                        /* rf_u8hex(memcmp(hw->peer.addr, rx->data, 6)); */
                        return FALSE;
                    }
                    //privacy RPA bypass
                /* } */
                break;

            case SCAN_REQ:
            case CONNECT_REQ:
                return FALSE;
            }
            break;

        case SLAVE_CONN_ST:
        case MASTER_CONN_ST:
            break;
    }
    return TRUE;
}


static void ble_rx_adv_process(struct ble_hw *hw, struct ble_rx *rx)
{
	struct ble_param *ble_fp = &hw->ble_fp;

    if (rx->type == SCAN_REQ)
    {
        __set_peer_addr(hw, rx->txadd, rx->data);
        putchar('S');
    }
    else if (rx->type == CONNECT_REQ)
    {
        int	sn =  ble_fp->TXTOG & BIT(0);

        /* DEBUG_IO_1(2); */
        ble_fp->TXAHDR0 = rx->rxadd<<4;
        ble_fp->TXDHDR0 = (0<<8)|(sn<<2)|1;
        ble_fp->TXAHDR1 = rx->rxadd<<4;
        ble_fp->TXDHDR1 = (0<<8)|(!sn<<2)|1;

        hw->tx_seqn = 0;
        ble_tx_init(hw);
    }
    else{
        ASSERT(0, "%s\n", __func__);
    }
}

static void backoff_pseudo(struct ble_hw *hw)
{
    u16 pseudo_random;

    pseudo_random = RAND64L | RAND64H;
    /* After success or failure of receiving the */
    /* SCAN_RSP PDU, the Link Layer shall set backoffCount to a new pseudo-random */
    /* integer between one and upperLimit */
    pseudo_random = pseudo_random % hw->backoff.upper_limit;
    if (!pseudo_random)
        pseudo_random = 1;

    hw->backoff.backoff_pseudo_cnt = pseudo_random;
}

static void ble_rx_scan_process(struct ble_hw *hw, struct ble_rx *rx)
{
    //Backoff Procedure
	struct ble_param *ble_fp = &hw->ble_fp;

    //passive scan
    if (hw->backoff.active == 0)
        return;

    if((rx->type == ADV_IND) || (rx->type == ADV_SCAN_IND))
    {
        rf_putchar('Q');
        if (--hw->backoff.backoff_count == 0) 
        {
            //nack
            if (hw->backoff.send == 1)
            {
                hw->backoff.consecutive_fail++;
                hw->backoff.consecutive_succ = 0;

                /* On every two consecutive failures, the upperLimit shall be doubled until it reaches */
                /* the value of 256 */
                if ((hw->backoff.consecutive_fail == 2)
                    && (hw->backoff.upper_limit != 256))
                {
                    hw->backoff.upper_limit = (hw->backoff.upper_limit == 256) ? 256 : hw->backoff.upper_limit<<1;
                    backoff_pseudo(hw);
                    hw->backoff.consecutive_fail = 0;
                    rf_puts("++");
                }
            }
            hw->backoff.send = 1;
            hw->backoff.backoff_count = hw->backoff.backoff_pseudo_cnt;
        }  
        //hw shoule open on the prev packet
        if (hw->backoff.backoff_count == 1)
        {
            //active scan
            __set_scan_active(hw, 1);
        }
        else
        {
            __set_scan_active(hw, 0);
        }

        //hw auto write back
        /* __set_remote_addr(hw, rx->data); */
        /* __set_scan_req_remote_addr(hw, rx->data); */
        /* __show_remote_addr(hw); */
        /* rf_buf(rx->data, 6); */
    }
    else if(rx->type == SCAN_RSP)
    {
        rf_putchar('P');
        //ack
        hw->backoff.send = 0;
        hw->backoff.consecutive_succ++;
        hw->backoff.consecutive_fail = 0;

        /* On every two consecutive successes, the upperLimit shall be */
        /* halved until it reaches the value of one  */
        if ((hw->backoff.consecutive_succ == 2)
            && (hw->backoff.upper_limit != 1))
        {
            hw->backoff.upper_limit = (hw->backoff.upper_limit == 1) ? 1 : hw->backoff.upper_limit>>1;
            backoff_pseudo(hw);
            hw->backoff.backoff_count = hw->backoff.backoff_pseudo_cnt;
            hw->backoff.consecutive_succ = 0;
            rf_puts("--");
        }
     }
}

static void ble_rx_init_process(struct ble_hw *hw, struct ble_rx *rx)
{
	struct ble_param *ble_fp = &hw->ble_fp;

    if((rx->type == ADV_IND) || (rx->type == ADV_DIRECT_IND)) 
    {
        struct ble_tx *tx;

        ble_tx_init(hw);

        hw->tx_seqn = 0;

        if (!(ble_fp->TXTOG & BIT(0))) {
            ble_fp->TXAHDR0 = rx->rxadd<<4;
            ble_fp->TXDHDR0 = (0<<8)|(0<<2)|1;
            ble_fp->TXAHDR1 = rx->rxadd<<4;
            ble_fp->TXDHDR1 = (0<<8)|(1<<2)|1;
        } else {
            ble_fp->TXAHDR0 = rx->rxadd<<4;
            ble_fp->TXDHDR0 = (0<<8)|(1<<2)|1;
            ble_fp->TXAHDR1 = rx->rxadd<<4;
            ble_fp->TXDHDR1 = (0<<8)|(0<<2)|1;
        }
    }
}

static void ble_rx_pdus_process(struct ble_hw *hw, struct ble_rx *rx)
{
	switch(hw->state)
	{
		case ADV_ST:
            ble_rx_adv_process(hw, rx);
			break;
		case SCAN_ST:
            ble_rx_scan_process(hw, rx);
			break;
		case INIT_ST:
			if(hw->privacy_enable)
			{
				if(!hw->is_init_enter_conn_pass) 	
				{
					return;
				}
			}
            ble_rx_init_process(hw, rx);
			break;
		case SLAVE_CONN_ST:
		case MASTER_CONN_ST:
			ble_hw_decrypt(hw, rx);
			break;
	}
}


static void ble_rx_probe(struct ble_hw *hw, struct ble_rx *rx)
{
    struct ble_rx empty;

    if (rx)
    {
        if (ble_rx_pdus_filter(hw, rx) == FALSE)
        {
            //ignore packet
            rf_putchar('~');
            lbuf_free(rx);
            return; 
        }

        ble_rx_pdus_process(hw, rx);

        //async PDUs upper to Link Layer
        if (RX_PACKET_IS_VALID(rx))
        {
            /* rf_buf(rx, sizeof(*rx)+rx->len); */
            lbuf_push(rx);
        }
    }
    else{
        //clean timeout
        rx = &empty; 
        RX_PACKET_SET_INVALID(rx);
    }

    //sync PDUs upper to Link Layer
    if (hw->handler && hw->handler->rx_probe_handler){
       hw->handler->rx_probe_handler(hw->priv, rx);
    }
}

//async PDUs upper to Link Layer
static void le_hw_upload_data(void (*handler)(void *priv, struct ble_rx *rx))
{
    struct ble_hw *hw;
    struct ble_rx *rx;

    //check available hw rx buf
    list_for_each_entry(hw, &bb.head, entry)
    {
        rx = lbuf_pop(hw->lbuf_rx);
        if (rx)
        {
			/* rf_puts("le_hw_upload_data"); */
            /* rf_buf(rx, sizeof(*rx)+rx->len); */
            handler(hw->priv, rx);
            lbuf_free(rx);
        }
    }
}

static int le_hw_upload_is_empty()
{
    struct ble_hw *hw;

    int empty = 1;

    list_for_each_entry(hw, &bb.head, entry)
    {
        empty &= lbuf_empty(hw->lbuf_rx);
    }
    return empty;
}

static void __hw_tx_process(struct ble_hw *hw)
{
	int i;
	struct ble_tx *tx, *tx_buf;
	struct ble_tx empty;
	struct ble_param *ble_fp = &hw->ble_fp;

	if (hw->state == SLAVE_CONN_ST || hw->state == MASTER_CONN_ST)
	{
		if (!hw->rx_ack){
			rf_putchar('N');
            //Nack no latency
            __set_latency_suspend(hw);
			return;
		}
        __set_latency_resume(hw);

		i = !(ble_fp->TXTOG & BIT(0));
        /* rf_putchar('0'+i); */

		if (hw->tx[i]){
            if (hw->handler && hw->handler->tx_probe_handler){

                /* ble_hw_txdecrypt(hw, hw->tx[i]); */
                hw->handler->tx_probe_handler(hw->priv, hw->tx[i]);
            }
			lbuf_free(hw->tx[i]);
		}
		tx = lbuf_pop(hw->lbuf_tx);
		if (!tx){
			memset(&empty, 0, sizeof(empty));
			empty.llid = 1;
			tx = &empty;
			hw->tx[i] = NULL;
            /* rf_buf(&empty, sizeof(empty)); */
            tx_buf = tx;
		} else {
			/* rf_putchar('$'); */
			rf_u8hex(tx->data[0]);
            tx_buf = hw->tx_buf[i];
			hw->tx[i] = tx;
            memcpy(tx_buf, tx, sizeof(*tx)+tx->len+4);
			ble_hw_encrpty(hw, tx_buf);
		}
        //if more data 
        tx_buf->md = lbuf_have_next(hw->lbuf_tx);
		if(!__get_slots_before_next_instant(hw)){
			if((tx_buf->md == 1) && (hw->tx_md_stop == 0)){	
				/* rf_putchar('%'); */
				tx_buf->md = 0;	
				hw->tx_md_stop = 1;
			}
		}
		else{
			/* rf_putchar('='); */
			hw->tx_md_stop = 0;	
		}

        tx_buf->sn = hw->tx_seqn;
		hw->tx_seqn = !hw->tx_seqn;
		ble_hw_tx(hw, tx_buf, i);
	}
}


static struct ble_rx *ble_rx_buf_alloc(struct ble_hw *hw, struct ble_rx *rx, int ind)
{
	struct ble_rx *rx_alloc;

	struct ble_param *ble_fp = &hw->ble_fp;

	u16 *rxptr;
	rxptr = (u16 *)&ble_fp->RXPTR0 + ind;

	if (RX_PACKET_IS_VALID(rx))
    {
		rf_putchar('R');
		if(ble_hw_rx_underload(hw))
        {
			if(NULL != *rxptr)
            {
                /* rf_putchar('A'); */
				rx_alloc = ble_hw_alloc_rx(hw, rx->len);
			}
			else
            {
                rf_puts("\nunderload...\n");
				*rxptr = PHY_TO_BLE(hw->rx_buf[ind] + sizeof(*rx));
				/* rx_alloc = &temp_rx;	 */
                rx_alloc = NULL;
			}
		}
		else{
			if(NULL != *rxptr)
            {	
                rf_puts("\noverload...\n");
				rx_alloc = ble_hw_alloc_rx(hw, rx->len);
				*rxptr = NULL;
			}
			else{
                rf_puts("\nunderloading\n");
				/* rx_alloc = &temp_rx;	 */
                rx_alloc = NULL;
			}
		}
	}
	else{
		/* rx_alloc = &temp_rx;	 */
        rx_alloc = NULL;
	}

	memcpy((u8 *)rx_alloc, (u8 *)rx, sizeof(*rx) + rx->len);

    return rx_alloc;
}

static void __hw_rx_process(struct ble_hw *hw)
{
	int ind;
	struct ble_rx *rx, *rx_alloc;
	u8 rx_check;
	int rxahdr, rxdhdr, rxstat;
	u16 *rxptr;
	struct ble_param *ble_fp = &hw->ble_fp;

    if (BLE_CON0 & BIT(5))
    {
        /* rf_puts("XXXX\n"); */
        /* trig_fun(); */
        /* while(1) */
        /* { */
             /* ADC_CON = 0; */
            /* [> rf_puts("\n---------------------"); <] */
        /* } */
    }

	if (hw->state == SLAVE_CONN_ST || hw->state == MASTER_CONN_ST)
    {
        ble_agc_normal_set(hw, 1, 0);
        /* ble_agc_normal_set(hw, 0, 19); */
    }
    else{
        ble_agc_normal_set(hw, 0, 19);
    }

	ind = !(ble_fp->RXTOG & BIT(0));
	rxahdr = *((u16 *)&ble_fp->RXAHDR0 + ind);
	rxdhdr = *((u16 *)&ble_fp->RXDHDR0 + ind);
	rxstat = *((u16 *)&ble_fp->RXSTAT0 + ind);
	rxptr  =  (u16 *)&ble_fp->RXPTR0   + ind;

	rx = hw->rx_buf[ind];

	rx->type =  rxahdr & 0xf;
    /* rx->len = (rxahdr>>8) & 0x3f;   //shall be 6 to 37 */
	rx->rxadd = (rxahdr>>7) & 0x1;
	rx->txadd = (rxahdr>>6) & 0x1;

	rx->len = (rxdhdr>>8) & 0xff;
	rx->llid = rxdhdr & 0x3;
	rx->sn = (rxdhdr>>3) & 1;
	rx->nesn = (rxdhdr>>2) & 1;
	rx_check = rxstat & 0XF;
	rx->res = rx_check;
	hw->rx_ack = (rxstat>>8) & 0x1;
	rx->event_count = ble_fp->EVTCOUNT;

	if (rx_check != 0x01){
        if (rx_check & BIT(2))
        {
            //bypass CRC error packet LL_DATA_PDU_CRC
            rf_putchar('C');
            rx->llid = 0x9;
            rx->len = 0;
        }
        else if (rx_check & BIT(3))
        {
            //bypass SN error packet LL_DATA_PDU_SN
            rf_putchar('W');
            rx->llid = 0x8;
            rx->len = 0;
        }
        else{
            rf_putchar('E');rf_u8hex(rx_check); rf_putchar('\n');
            //TO*DO
            return;
        }
	}
#ifndef FPGA
	/* fsk_offset(1); */
#endif
	ble_hw_encrypt_check(hw);

	/* rf_putchar('r'); */
    //baseband loop buf switch
    rx_alloc = ble_rx_buf_alloc(hw, rx, ind);
	ble_rx_probe(hw, rx_alloc);

	__hw_tx_process(hw);
}


static void __hw_event_process(struct ble_hw *hw)
{
	int i;
	struct ble_param *ble_fp = &hw->ble_fp;

	switch(hw->state)
	{
		case ADV_ST:
			putchar('0' + HW_ID(hw));
            __set_adv_channel_index(hw, hw->adv_channel);
			i = ble_fp->TXTOG & BIT(0);
			memcpy(hw->tx_buf[0], hw->tx[0], TX_PACKET_SIZE(hw->tx[0]->len));
			memcpy(hw->tx_buf[1], hw->tx[1], TX_PACKET_SIZE(hw->tx[1]->len));
			ble_hw_tx(hw, hw->tx_buf[0], i);
			ble_hw_tx(hw, hw->tx_buf[1], !i);
			break;
		case SCAN_ST:
		case INIT_ST:
            /* rf_putchar('#'); */
            __set_scan_channel_index(hw, hw->adv_channel);
            if (++hw->adv_channel > 39)
            {
				hw->adv_channel = 37;
			}
			break;
		case SLAVE_CONN_ST:
		case MASTER_CONN_ST:
            /* rf_putchar('@'); */
            /* put_u16hex(ble_fp->EVTCOUNT); */
            /* ble_debug_signal[HW_ID(hw)]++; */
            if (ble_debug_signal[HW_ID(hw)] == 10)
            {
                /* trig_fun(); */
                /* PORTA_DIR &= ~BIT(0); */
                /* PORTA_OUT |= BIT(0); */
                rf_puts("----X------\n");
                put_u16hex(ble_fp->EVTCOUNT);
                /* PORTA_OUT &= ~BIT(0); */
            }
			if (hw->handler && hw->handler->event_handler){
				hw->handler->event_handler(hw->priv, ble_fp->EVTCOUNT);
			}
            /* if (__lower()->event_sync) */
                /* __lower()->event_sync(hw->priv, ble_fp->EVTCOUNT); */
			break;
		default: return;
	}
}

static void ble_irq_handler()
{
	int i;
    struct ble_hw *hw;

    /* trig_fun(); */
	for (i=0; i<8; i++)
	{
        hw = &ble_base.hw[i];
        //must first deal
		if(BLE_INT_CON2 & BIT(i+8))//rx int
		{
			BLE_INT_CON1 |= BIT(i+8);     ///  !!! must be clear twice
            /* DEBUG_IO_1(3) */
            /* {PORTA_DIR &= ~BIT(3); PORTA_OUT ^= BIT(3);} */
            /* trig_fun(); */
            /* ADC_CON = 1; */
			__hw_rx_process(hw);
            /* DEBUG_IO_0(0) */

			/* BLE_INT_CON1 |= BIT(i+8); */
		}

        //must deal after rx
		if(BLE_INT_CON2 & BIT(i))//event_end int
		{
			BLE_INT_CON1 = BIT(i);     ///  !!! must be clear twice
            /* DEBUG_IOB_1(1) */
			__hw_event_process(hw);

            /* DEBUG_IOB_0(1) */
			/* BLE_INT_CON1 = BIT(i); */


            /* ble_power_off(hw); */

		}
	}

    if (BLE_DEEPSL_CON & BIT(7))
	{
		BLE_DEEPSL_CON  |= BIT(6);

		BLE_CLKNCNTCORR = __this->fine_cnt/SLOT_UNIT_US + 1;
		BLE_FINECNTCORR = __this->fine_cnt%SLOT_UNIT_US;
        DEBUG_IO_0(0);

        /* rf_deg("fine_cnt : %04d\n",__this->fine_cnt); */
        BT_LP_CON |= BIT(1);
		BLE_DEEPSL_CON |= BIT(2);

        DEBUG_IO_0(1);
		rf_putchar('W');
        /* while(1); */

		ble_all_power_on();
	}
}

/********************************************************************************/
/*
 *                   HW Abstract Layer
 */
static struct ble_hw *ble_hw_alloc()
{
	int i;
	struct ble_hw *hw = NULL;

	for (i=0; i<8; i++)
	{
		if (ble_base.inst[i] == 0){
			hw = &ble_base.hw[i];
			memset(hw, 0, sizeof(*hw));
			ble_base.inst[i] = BIT(12) | PHY_TO_BLE(&hw->ble_fp);
			__set_hw_frame_init(hw);
            rf_deg("hw: id=%d\n", i);
            list_add_tail(&hw->entry, &bb.head);
			break;
		}
	}
    if (hw)
    {
        hw->regs = bt_malloc(BTMEM_HIGHLY_AVAILABLE, 10*4);
		ASSERT(hw->regs != NULL);
    }

	return hw;
}

static void ble_hw_frame_free(struct ble_hw *hw)
{
    rf_deg("free hw: id=%d\n", HW_ID(hw));

    bt_free(hw->regs);
	ble_base.inst[HW_ID(hw)] = 0;

	list_del(&hw->entry);
}

/* static const u8 addr[6] = {0x10, 0xbf, 0x77, 0x18, 0xeb, 0x84}; */
static void le_hw_standby(struct ble_hw *hw)
{
	hw->state = STANDBY_ST;
	ble_hw_disable(hw);
    /* __set_white_list_addr(hw, 0, addr); */
    /* __set_device_filter_param(hw, 0, 1); */
}


static struct ble_tx *__adv_pdu(struct ble_hw *hw,
		struct ble_adv *adv)
{
	struct ble_tx *tx;

    switch(adv->adv_type)
    {
        case ADV_IND:
        case ADV_NONCONN_IND:
        case ADV_SCAN_IND:
            tx = le_hw_alloc_tx(hw, adv->adv_type, 0, 6+adv->adv_len);
            //AdvA
            tx->txadd = hw->local.addr_type;
            memcpy(tx->data, hw->local.addr, 6);
            //AdvData
            memcpy(tx->data+6, adv->adv_data, adv->adv_len);
            break;
        case ADV_DIRECT_IND:
            tx = le_hw_alloc_tx(hw, ADV_DIRECT_IND, 0, 6+6);
            //AdvA
            tx->txadd = hw->local.addr_type;
            memcpy(tx->data, hw->local.addr, 6);
            //InitA
            tx->rxadd = hw->peer.addr_type;
            memcpy(tx->data+6, hw->peer.addr, 6);
            break;
    }

	return tx;
}

static struct ble_tx *__scan_rsp_pdu(struct ble_hw *hw,
		struct ble_adv *adv)
{
	struct ble_tx *tx;

	tx = le_hw_alloc_tx(hw, SCAN_RSP, 0, 6+adv->scan_rsp_len);

    //AdvA
    tx->txadd = hw->local.addr_type;
    memcpy(tx->data, hw->local.addr, 6);
	memcpy(tx->data+6, adv->scan_rsp_data, adv->scan_rsp_len);


	return tx;
}

static struct ble_tx *__scan_req_pdu(struct ble_hw *hw,
        struct ble_scan *scan)
{
	struct ble_tx *tx;

	tx = le_hw_alloc_tx(hw, SCAN_REQ, 0, 6 + 6);

    //AdvA
    tx->txadd = hw->local.addr_type;
    memcpy(tx->data, hw->local.addr, 6);
	memset(tx->data+6, 0, 6);

    /* tx->rxadd = adv->peer_addr_type; */

	return tx;
}


static struct ble_tx *__connect_req_pdu(struct ble_hw *hw, 
		struct ble_conn *conn)
{
	struct ble_tx *tx;

	tx = le_hw_alloc_tx(hw, CONNECT_REQ, 0, 6 + 6 + 22);

    //InitA
    tx->txadd = hw->local.addr_type;
    memcpy(tx->data, hw->local.addr, 6);
    //AdvA
    tx->rxadd = hw->peer.addr_type;
    memcpy(tx->data+6, hw->peer.addr, 6);

	memcpy(tx->data + 12, conn->ll_data, 16);
	memcpy(tx->data + 12 + 16, ((u8 *)&conn->ll_data) + 18, 6);

	return tx;
}


static void le_hw_advertising(struct ble_hw *hw, struct ble_adv *adv)
{
	rf_puts("hw_advertising\n");

    //from connect state back to adv state
	struct ble_param *ble_fp = &hw->ble_fp;

	ble_fp->BDADDR0 = PUBILC_ACCESS_ADR0;
	ble_fp->BDADDR1 = PUBILC_ACCESS_ADR1;
	ble_fp->CRCWORD0 = 0x5555;
	ble_fp->CRCWORD1 = 0x55;

	__set_winsize(hw, 50, 50);
	__set_interval(hw, adv->interval*SLOT_UNIT_US, 1);
	__set_hw_state(hw, ADV_ST, 0, 0);

    __set_adv_device_filter_param(hw, adv->filter_policy); 

	hw->ble_fp.ADVIDX=(adv->pkt_cnt<<14)|adv->pdu_interval*SLOT_UNIT_US; 	//pkt_cnt:interval N*us
    /* __debug_interval(hw); */

    //The advDelay is a pseudo-random value with a range of 0 ms to 10 ms generated by the Link Layer for each advertising event.
    /* __set_adv_random_us(hw);      */
    __set_adv_random_slot(hw);

    u8 i; 

    for (i = 0, adv->pkt_cnt = 0; i < 3; i++)
    {
        if (adv->channel_map & BIT(i))
        {
            hw->adv_channel = 37 + i;
            break;
        }
    }
    __set_adv_channel_map_patch(hw, adv->channel_map);

    hw->tx[0] = __adv_pdu(hw, adv);
    hw->tx[1] = __scan_rsp_pdu(hw, adv);

	/* __hw_event_process(hw); */
    //new feature 
    __set_addr_match_enable(ble_fp);

	ble_hw_enable(hw, 10);

	hw->ble_fp.FORMAT |= BIT(2);
}


static void le_hw_scanning(struct ble_hw *hw, struct ble_scan *scan)
{
	rf_puts("hw_scanning\n");
	struct ble_tx *tx;

	__set_winsize(hw, 50, scan->window*SLOT_UNIT_US);
	__set_interval(hw, scan->interval*SLOT_UNIT_US, 0);
	__set_hw_state(hw, SCAN_ST, 0, 0);

    __set_scan_device_filter_param(hw, scan->filter_policy);

    hw->backoff.active = scan->type;
    //active
    if (hw->backoff.active)
    {
        hw->backoff.send = 0;
        hw->backoff.upper_limit = 1;
        hw->backoff.backoff_count = 1;
        hw->backoff.backoff_pseudo_cnt = 1;
        hw->backoff.consecutive_fail = 0;
        hw->backoff.consecutive_succ = 0;
    }

    __set_scan_active(hw, scan->type);

	hw->adv_channel = 37;
	/* __hw_event_process(hw); */

	tx = __scan_req_pdu(hw, scan);
    /* hw->tx[0] = tx; */
    /* hw->tx[1] = tx; */
	memcpy(hw->tx_buf[0], tx, TX_PACKET_SIZE(tx->len));
	memcpy(hw->tx_buf[1], tx, TX_PACKET_SIZE(tx->len));
	ble_hw_tx(hw, hw->tx_buf[0], 0);
	ble_hw_tx(hw, hw->tx_buf[1], 1);

	/* ble_hw_tx(hw, tx, 0); */
	/* ble_hw_tx(hw, tx, 1); */

	ble_hw_enable(hw, 1);
}


void le_hw_initiating(struct ble_hw *hw, struct ble_conn *conn)
{
	rf_puts("hw_initiating\n");
	struct ble_tx *tx;
    struct ble_conn_param *conn_param = &conn->ll_data;

	__set_winsize(hw, 50, conn->scan_windows*SLOT_UNIT_US);
	__set_interval(hw, conn->scan_interval*SLOT_UNIT_US, 0);
	__set_hw_state(hw, INIT_ST, 0, 0);

    /* __set_init_device_filter_param(hw, conn->filter_policy); */

	hw->adv_channel = 37;
	/* __hw_event_process(hw); */

	tx = __connect_req_pdu(hw, conn);

    /* rf_puts("conn req : "); */
    /* rf_buf(hw->local.addr, 6); */
    rf_puts("conn req : ");
    rf_buf(hw->peer.addr, 6);
	/* ble_hw_tx(hw, tx, !(hw->ble_fp.TXTOG & BIT(0))); */
	memcpy(hw->tx_buf[0], tx, TX_PACKET_SIZE(tx->len));
	memcpy(hw->tx_buf[1], tx, TX_PACKET_SIZE(tx->len));
	ble_hw_tx(hw, hw->tx_buf[0], 0);
	ble_hw_tx(hw, hw->tx_buf[1], 1);
	/* ble_hw_tx(hw, tx, 0); */
	/* ble_hw_tx(hw, tx, 1); */

    //new feature 
    if (hw->privacy_enable){
        __set_addr_match_disable(&hw->ble_fp);
		hw->is_init_enter_conn_pass = 0; 
    }
    else{
        __set_addr_match_enable(&hw->ble_fp);
    }

	ble_hw_enable(hw, 10);
	__set_init_end(hw);
}



static void *le_hw_open()
{
	struct ble_hw *hw;

	hw = ble_hw_alloc();
    ASSERT(hw != NULL, "hw alloc err");
	if (!hw){
		return NULL;
	}
	/*hw->backoff_state = 0;
	hw->backoff_count = 6;*/

	ble_rx_init(hw);
	ble_tx_init(hw);

    //add to power manager
    ble_power_get(hw);

	le_hw_standby(hw);

	return hw;
}

static void le_hw_close(struct ble_hw *hw)
{
	ble_hw_disable(hw);

    ble_power_put(hw);

	ble_hw_frame_free(hw);
}

static void le_hw_send_packet(struct ble_hw *hw, u8 llid, u8 *packet, int len)
{
    ASSERT(len <= hw->tx_octets, "%s\n", __func__);

	struct ble_tx *tx = le_hw_alloc_tx(hw, 0, llid, len);
	/* struct ble_tx *tx = le_hw_alloc_tx(hw, 0, llid, hw->tx_octets); */

	ASSERT(tx != NULL);

	/* rf_deg("send_packet: %x, %x, %d\n", hw, tx, len); */

	memcpy(tx->data, packet, len);

	lbuf_push(tx);
}


static void le_hw_ioctrl(struct ble_hw *hw, int ctrl, ...)
{
	va_list argptr;
	va_start(argptr, ctrl);

	switch(ctrl)
	{
		case BLE_SET_IV:
			memcpy(hw->encrypt.iv, va_arg(argptr, int), 8);
			break;
		case BLE_SET_SKD:
			memcpy(hw->encrypt.skd, va_arg(argptr, int), 16);
			break;
		case BLE_SET_LOCAL_ADDR:    //public/random/RPA
			__set_local_addr(hw, va_arg(argptr, int), va_arg(argptr, int));
			break;
        case BLE_SET_PEER_ADDR:     //public/random/RPA
			__set_peer_addr(hw, va_arg(argptr, int), va_arg(argptr, int));
            break;
		case BLE_SET_INSTANT:
			__set_event_instant(hw, va_arg(argptr, int));
			break;
		case BLE_SET_WINSIZE:
			__set_winsize(hw, va_arg(argptr, int), va_arg(argptr, int));
			break;
		case BLE_CHANNEL_MAP:
			__set_channel_map(hw, va_arg(argptr, int));
			break;
		case BLE_SET_CONN_PARAM:
			__set_connection_param(hw, va_arg(argptr, int));
			break;
		case BLE_CONN_UPDATE:
			__connection_update(hw, va_arg(argptr, int));
			break;
        case BLE_WHITE_LIST_ADDR:
            __set_white_list_addr(hw, va_arg(argptr, int), va_arg(argptr, int));
            break;
        case BLE_SET_SEND_ENCRYPT:
            __set_send_encrypted(hw, va_arg(argptr, int));
            break;
        case BLE_SET_RECEIVE_ENCRYPT:
            __set_receive_encrypted(hw, va_arg(argptr, int));
            break;
        //privacy
        case BLE_SET_LOCAL_ADDR_RAM:
			__set_local_addr_ram(hw, va_arg(argptr, int), va_arg(argptr, int));
			break;
        case BLE_SET_PRIVACY_ENABLE:
            /* rf_puts("BLE_SET_PRIVACY_ENABLE\n"); */
            __set_privacy_enable(hw, va_arg(argptr, int));
            break;
        case BLE_SET_RPA_RESOLVE_RESULT:
            /* rf_puts("BLE_SET_RPA_RESOLVE_RESULT\n");  */
            __set_rpa_resolve_result(hw, va_arg(argptr, int), va_arg(argptr, int));
            break;
        case BLE_SET_RX_LENGTH:
            /* rf_puts("BLE_SET_RX_LENGTH\n"); */
            /* __set_rx_length(hw, va_arg(argptr, u16)); */
            break;
        case BLE_SET_TX_LENGTH:
            /* rf_puts("BLE_SET_TX_LENGTH\n"); */
            __set_tx_length(hw, va_arg(argptr, u16));
            break;
		default:
			break;
	}

	va_end(argptr);
}



static void le_hw_handler_register(struct ble_hw *hw, void *priv,
	   	const struct ble_handler *handler)
{
	hw->priv = priv;
	hw->handler = handler;
}

void ble_rf_init(void)
{
	rf_puts("ble_RF_init\n");
    BT_BLE_CON |= BIT(0);

	/* SFR(BLE_RADIO_CON0, 0 , 10 , 150);    //TXPWRUP_OPEN_PLL */
	/* SFR(BLE_RADIO_CON0, 12 , 4 , 12);     //TXPWRPD_CLOSE_LOD */
	/* SFR(BLE_RADIO_CON1, 0 , 10 , 145);    //RXPWRUP_OPEN_PLL */
	/* SFR(BLE_RADIO_CON2, 0 , 8 ,  50);     //TXCNT_OPEN_LDO */
	/* SFR(BLE_RADIO_CON2, 8 , 8 ,  60);     //RXCNT_OPEN_LDO */
	/* SFR(BLE_RADIO_CON3, 0 , 8 ,  30);     //wait read data */

	BLE_CON0  = BIT(0);          //ble_en
	/* BLE_CON1  = 3;               //sync_err */
    //debug
    BLE_DBG_CON = BIT(0) | (9 << 8);
}

static void le_hw_init()
{
	rf_puts("le_hw_init\n");

    ble_rf_init();
	BT_BLEEXM_ADR = (u32)&ble_base;
	BT_BLEEXM_LIM = ((u32)&ble_base) + sizeof(ble_base);

    if (!bt_power_is_poweroff_post())
    {
        memset(__this, 0, sizeof(struct ble_handle));
        memset(&ble_base.inst, 0, sizeof(ble_base.inst));

        INIT_LIST_HEAD(&bb.head);
    }

	request_irq(IRQ_BLE, ble_irq_handler, IRQF_SHARED, NULL);
}

static int le_hw_get_id(struct ble_hw *hw)
{
     return BIT(HW_ID(hw));
}

static int le_hw_get_conn_event(struct ble_hw *hw)
{
    return hw->ble_fp.EVTCOUNT;
}

static int le_hw_is_init_enter_conn(struct ble_hw *hw)
{
	return hw->is_init_enter_conn_pass; 
}

static const struct ble_operation ble_ops = {
	.init = le_hw_init,
	.open = le_hw_open,
	.standby = le_hw_standby,
	.advertising = le_hw_advertising,
	.scanning = le_hw_scanning,
	.initiating = le_hw_initiating,
	.handler_register = le_hw_handler_register,
	.send_packet = le_hw_send_packet,
	.ioctrl = le_hw_ioctrl,
	.close = le_hw_close,
    .get_id = le_hw_get_id,
    .upload_data = le_hw_upload_data,
    .upload_is_empty = le_hw_upload_is_empty,
    .get_conn_event = le_hw_get_conn_event,
	.is_init_enter_conn = le_hw_is_init_enter_conn,
};
REGISTER_BLE_OPERATION(ble_ops);



