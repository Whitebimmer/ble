#include "RF_ble.h"
#include "ble_interface.h"
#include "interrupt.h"
#include "init.h"
/*#include "platform_device.h"*/
#include "stdarg.h"
#include "RF.h"


#define READ_BT16(p, offset)  (((u16)p[(offset)+1]<<8) | p[offset])

/*
struct conn_ll_data {
	u8 aa[4];
	u8 crcinit[3];
	u8 winsize;
	u8 winoffset[2];
	u8 interval[2];
	u8 latency[2];
	u8 timeout[2];
	u8 chm[5];
#ifdef LITTLE_ENDIAN
	u8 hop:5;
	u8 sca:3;
#elif defined BIG_ENDIAN
	u8 sca:3;
	u8 hop:5;
#else
#error "undefine cpu endian"
#endif
};
*/



/*#define PHY_TO_BLE(a) \*/
		/*((u32)(a) - ((u32)&ble_base))*/
#define PHY_TO_BLE(a) \
		({ ASSERT((u32)(a)-(u32)&ble_base < 0xffff); \
		((u32)(a) - ((u32)&ble_base)); \
		})

#define HW_ID(hw) \
	((hw) - ble_base.hw)


static struct ble_hw_base ble_base;

struct baseband{
    struct list_head head;
};

static struct baseband bb;


struct ble_rx * ble_hw_alloc_rx(struct ble_hw *hw, int size)
{
	struct ble_rx *rx;

	rx = lbuf_alloc(hw->lbuf_rx, sizeof(*rx)+size);
	ASSERT(rx != NULL, "%s\n", "rx alloc err\n");
	ASSERT(((u32)rx & 0x03) == 0, "%s\n", "rx not align\n");

	memset(rx, 0, sizeof(*rx));

	return  rx;
}


static struct ble_tx * le_hw_alloc_tx(struct ble_hw *hw,
	   int type, int llid, int len)
{
	struct ble_tx *tx;

	tx = lbuf_alloc(hw->lbuf_tx, sizeof(*tx)+len+4);
	ASSERT(tx != NULL, "%s\n", "tx alloc err\n");
	memset(tx, 0, sizeof(*tx));
	tx->type = type;
	tx->llid = llid;
	tx->len = len;

	return tx;
}

static void ble_rx_init(struct ble_hw *hw)
{
	hw->lbuf_rx = lbuf_init(ble_base.rx[HW_ID(hw)], 512);
	hw->rx[0] = ble_hw_alloc_rx(hw, 40);
	hw->rx[1] = ble_hw_alloc_rx(hw, 40);

	hw->ble_fp.RXPTR0 = PHY_TO_BLE(hw->rx[0]->data);
	hw->ble_fp.RXPTR1 = PHY_TO_BLE(hw->rx[1]->data);
}

static void ble_tx_init(struct ble_hw *hw)
{
	hw->tx[0] = NULL;
	hw->tx[1] = NULL;
	hw->lbuf_tx = lbuf_init(ble_base.tx[HW_ID(hw)], 512);
}

static void ble_hw_enable(struct ble_hw *hw, int slot)
{ 
	int hw_id = HW_ID(hw);

	BLE_INT_CON1 = BIT(hw_id+8)| BIT(hw_id);
	BLE_ANCHOR_CON1=(1<<15)|(slot); /*anchor_en/anchor_cnt N*625us*/ 
	BLE_ANCHOR_CON0=(0<<12)|(hw_id<<8)|(1<<2)|1;

	SFR(BLE_INT_CON0, hw_id, 1, 1);  /*open event int*/ 
	SFR(BLE_INT_CON0, (hw_id+8), 1, 1);  /*open rx int*/ 
}

static void ble_hw_disable(struct ble_hw *hw)
{
	int hw_id = HW_ID(hw);

	BLE_ANCHOR_CON1 = 0;
	BLE_ANCHOR_CON0 = (0<<12)|(hw_id<<8)|(1<<2)|1; 

	SFR(BLE_INT_CON0, hw_id, 1, 0);  		/*open event int*/ 
	SFR(BLE_INT_CON0, (hw_id+8), 1, 0);  	/*open rx int*/   
}

static void __set_interval(struct ble_hw *hw, int interval, int rand)
{
	BLE_ANCHOR_CON1 = (rand<<15)|(interval/625);
	BLE_ANCHOR_CON0 = (1<<12)|(HW_ID(hw)<<8)|(1<<2)|1;
}

static void __set_hw_state(struct ble_hw *hw, int state,
	   	int latency_en, int latency)
{
	hw->state = state;
	BLE_ANCHOR_CON1 = (state<<12)|((latency_en)<<11)|latency;
	BLE_ANCHOR_CON0 = (2<<12)|(HW_ID(hw)<<8)|(1<<2)|1;
}

static void __set_event_count(struct ble_hw *hw, int count)
{
	BLE_ANCHOR_CON1 = count;     //conn_event_count
	BLE_ANCHOR_CON0 = (3<<12)|(HW_ID(hw)<<8)|(1<<2)|1;
}


static void __set_event_instant(struct ble_hw *hw, int instant)
{
	BLE_ANCHOR_CON1 =  instant;
	BLE_ANCHOR_CON0 = (5<<12) | (HW_ID(hw)<<8) | (1<<2)  | 1;
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

static void __set_instant(struct ble_hw *hw, int instant)
{
	BLE_ANCHOR_CON1 = instant;
	BLE_ANCHOR_CON0 = (5<<12)|(HW_ID(hw)<<8)|(1<<2)|1;
}

static void __set_winsize(struct ble_hw *hw, int winsize, int nwinsize)
{
    /* printf("\nwinsize : %x - 1 : %x \n", winsize, nwinsize); */
	hw->ble_fp.WINCNTL0 = nwinsize;//N*us 
    //50us Active Clock Accuracy / Sleep Clock Accuracy
	hw->ble_fp.WINCNTL1 = (winsize<<8)|(nwinsize>>16);    //50 depend on 
}

static void __set_widen(struct ble_hw *hw, int widen)
{
    if ((widen < 625) && (widen > 420))
    {
        widen = 625;
    }
    //windowWidening = ((masterSCA + slaveSCA) / 1000000) * timeSinceLastAnchor
    hw->ble_fp.WIDEN0 = (widen)/625;                // [11:0] slotoffset_trim
    //[12:15] only establish connetion & winoffset
    //[11:0] connetion interval
    hw->ble_fp.WIDEN1 = (5<<12) | (widen%420); //[15:12]slave_Rx_first_bitoffset
    /* printf("\nwiden0 : %x - 1 : %x \n", hw->ble_fp.WIDEN0, hw->ble_fp.WIDEN1); */
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



static void __set_local_addr(struct ble_hw *hw, u8 addr_type, const u8 *addr)
{
    hw->local.addr_type = (addr_type) ? 1 : 0;

	memcpy(hw->local.addr, addr, 6);

	hw->ble_fp.LOCALADRL = (addr[1] << 8) + addr[0];
	hw->ble_fp.LOCALADRM = (addr[3] << 8) + addr[2];
	hw->ble_fp.LOCALADRU = (addr[5] << 8) + addr[4];
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

extern char bt_pll_cap0;
extern char bt_pll_cap1;
extern char bt_pll_cap2;
extern char bt_pll_cap3;

static void __set_channel_map(struct ble_hw *hw, u8 *channel)
{
	int i;
	int num = 0;
	int frq_mid;   // 89 mid 2402  2M MID_FREQ_UP;
	struct ble_param *ble_fp = &hw->ble_fp;
	u8 pll_param = 0;
	u8 m[3]={0, 12, 39};
	//	u8 *channel = hw->conn_param.channel;

	for(i=0; i<=36; i++)
	{
		if(i==0) { //2404
			frq_mid = 81;
		} else if(i==11)  { //2428
			frq_mid =  83;
		}
#ifndef FPGA
		pll_param = get_pll_param_for_frq(frq_mid+i*2);
#endif
		ble_fp->FRQ_TBL0[i] = (i<<16) |(pll_param<<8)|(frq_mid + i*2);
		if (channel[i>>3] & BIT(i&0x7)){
			ble_fp->FRQ_TBL1[num++] = (i<<16) | (pll_param<<8)|(frq_mid + i*2);
		}
	}
	for (i=0; i<3; i++)
	{
	
		int j = m[i];
#ifndef FPGA
		pll_param = get_pll_param_for_frq(81-2+j*2);
#endif
		ble_fp->FRQ_TBL0[37+i] = ((37+i)<<16) |(pll_param<<8)| (81-2 + j*2);
		ble_fp->FRQ_TBL1[37+i] = ((37+i)<<16) |(pll_param<<8)| (81-2 + j*2);
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
	u8 m[3]={0, 12, 39};

    if (channel_map != 0x5)
        return;
    puts("****adv channel map patch***\n");
    for (i=0; i<3; i++)
    {
        int j = m[i];
#ifndef FPGA
        pll_param = get_pll_param_for_frq(81-2+j*2);
#endif
        if (i == 0)
        {
            //ble_fp->FRQ_IDX0[37] = 37;
            ble_fp->FRQ_TBL0[37] = (37<<16) |(pll_param<<8)| (81-2 + m[0]*2);
        }
        else if (i == 2)
        {
            //ble_fp->FRQ_IDX0[38] = 39;
            ble_fp->FRQ_TBL0[38] = (39<<16) |(pll_param<<8)| (81-2 + m[2]*2);
        }
        //ble_fp->FRQ_IDX1[37+i] = 37+i;
        ble_fp->FRQ_TBL1[37+i] = (37+i<<16) |(pll_param<<8)| (81-2 + j*2);
    }
}

static void __set_scan_active(struct ble_hw *hw, u8 active)
{
	struct ble_param *ble_fp = &hw->ble_fp;

    if (active)
    {
        //active scan
        ble_fp->FORMAT &= ~(1<<8);
    }
    else{
        //passive scan
        ble_fp->FORMAT |= (1<<8);
    }
}

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

static void ble_hw_frame_init(struct ble_hw *hw)
{
	u8 channel[5];
	struct ble_param *ble_fp = &hw->ble_fp;

	ble_fp->ANCHOR = (0<<15);                      // updat event interval
	ble_fp->BITOFF = (0<<15) | 0; // 1:use soft bitoffset  0: use hardware
	ble_fp->ADVIDX = (3<<14) | 5000;//ADV_PACKET_NUM: ADC_PACKET_interval
	ble_fp->WIDEN0 = 0;             // [11:0] slotoffset_trim
	ble_fp->WIDEN1 = (5<<12) | 400; //[15:12]slave_Rx_first_bitoffset
	//[9:0]bitoffset_trim
	
	ble_fp->BDADDR0 = PUBILC_ACCESS_ADR0;
	ble_fp->BDADDR1 = PUBILC_ACCESS_ADR1;

	ble_fp->CRCWORD0 = 0x5555;
	ble_fp->CRCWORD1 = 0x55;
#ifdef FPGA 
	ble_fp->RFTXCNTL0 = 0xFFFF;         //[14:0] TXPWR
	ble_fp->RFTXCNTL1 = 0x2a;          //[5:0]  TX_RX
	ble_fp->RFRXCNTL0 = (8<<9) | (8<<5) | (3<<2) | 1;     //[14:0] RXGAIN;
	ble_fp->RFRXCNTL1 = 0x34;                             //[5:0]  TX_RX

	ble_fp->RFCONFIG = (5<<4) | 0XA;                 //BT_OSC_CAP
#else
	ble_fp->RFTXCNTL0 = 0;
	ble_fp->RFTXCNTL1 = 0;
	ble_fp->RFTXCNTL0 |= 0XF;     //[3:0] PA_GSEL
	ble_fp->RFTXCNTL0 |= (0Xd<<4);//[7:4] MIXSR_LCSEL
	ble_fp->RFTXCNTL0 |= (1<<8);  //[9:8] MIXER_GISEL
	ble_fp->RFTXCNTL0 |= (0<<10); //[12:10] DAC_BIAS_SET
	ble_fp->RFTXCNTL0 |= (4<<13); //[15:13] DAC_GAIN_SET
	ble_fp->RFTXCNTL1 |= 0X16;    //[4:0] PA_CSEL
	ble_fp->RFTXCNTL1 |= (0<<5);  //[5]   LNA_GSEL1

	ble_fp->RFRXCNTL0 = 0;
	ble_fp->RFRXCNTL1 = 0;
	ble_fp->RFRXCNTL0 |= 0Xf;      //[3:0]BP_GA
	ble_fp->RFRXCNTL0 |= (1<<8);   //LNA_GSEL0
	ble_fp->RFRXCNTL0 |= (3<<9);   //[11:9]LNA_ISEL
	ble_fp->RFRXCNTL0 |= (6<<12);  //[15:12]DMIX_R_S
	ble_fp->RFRXCNTL1 |= 0X10;    //[4:0]PA_CSEL
	ble_fp->RFRXCNTL1 |= (0<<5);  //[5]   LNA_GSEL1;

	ble_fp->RFCONFIG = 0;
	ble_fp->RFCONFIG |= 1;      //[3:0]XOSC_CLSEL
	ble_fp->RFCONFIG |= (1<<5); //[7:4]XOSC_CRSEL
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
	ble_fp->RXMAXBUF = 38;
	ble_fp->IFSCNT = (130<<8) | (150);                     //txifs / rxifs
	ble_fp->FILTERCNTL = 0;

	//ble_fp->LASTCHMAP = 37;               //unmap_channal set by hardware
	/*ble_fp->WINCNTL0 =  625*2;            //first_rx:{WINCNTL1[7:0],
	ble_fp->WINCNTL1 = (50<<8)|0; //packet_interval_rx:WINCNTL1[15:8]us*/

	/*ble_fp->LOCALADRL = (local_addr[1] << 8) + local_addr[0];
	ble_fp->LOCALADRM = (local_addr[3] << 8) + local_addr[2];
	ble_fp->LOCALADRU = (local_addr[5] << 8) + local_addr[4];*/

	ble_fp->TARGETADRL = 0;
	ble_fp->TARGETADRM = 0;
	ble_fp->TARGETADRU = 0;
/*#ifdef FPGA*/
	ble_fp->RFRXCNTL0 = __get_agc(&hw->agc, 0, 12);
	ble_fp->RFTXCNTL0 = __get_tx_power(&hw->power, 0, 15);
/*#endif*/
	memset(channel, 0xff, 5);
	__set_channel_map(hw, channel);
}



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
			ble_hw_frame_init(hw);
            printf("hw: id=%d\n", i);
            list_add_tail(&hw->entry, &bb.head);
			break;
		}
	}

	return hw;
}

static void ble_hw_frame_free(struct ble_hw *hw)
{
    printf("free hw: id=%d\n", HW_ID(hw));

	ble_base.inst[HW_ID(hw)] = 0;

	list_del(&hw->entry);
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

static void ble_hw_encrpty(struct ble_encrypt *encrypt, struct ble_tx *tx)
{
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
	memcpy(nonce+5, encrypt->iv, 8);

	rijndael_setup(encrypt->skd);
	ccm_memory(nonce, &head, 1, tx->data, tx->len, pt, tag, 0);

	encrypt->tx_counter_l++;
	memcpy(tx->data, pt, tx->len);
	memcpy(tx->data+tx->len, tag, 4);
	tx->len += 4;
}


static void ble_hw_decrypt(struct ble_encrypt *encrypt, struct ble_rx *rx)
{
	struct ble_tx *tx;

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
		nonce[4] = encrypt->rx_counter_h | 0x80;
		memcpy(nonce+5, encrypt->iv, 8);

		if (rx->llid != 1){
			encrypt->rx_counter_l++;
		}

		rx->len -= 4;
		rijndael_setup(encrypt->skd);
		ccm_memory(nonce, &head, 1, pt, rx->len, rx->data,  tag, 1);
		memcpy(rx->data, pt, rx->len);
		if (memcmp(tag, rx->data+rx->len, 4)){
			printf("no_match: %d\n", rx->llid);
		}
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
		BLE_ANCHOR_CON1 = 0;
		BLE_ANCHOR_CON0 = (0<<12) | (HW_ID(hw)<<8) | (1<<2)  | 1;

		BLE_ANCHOR_CON1 = (1<<15) | (conn_param->winoffset*2+2+2);
		BLE_ANCHOR_CON0 = (0<<12) | (HW_ID(hw)<<8) | (1<<2)  | 1;
        //----Winsize
		/* __set_winsize(hw, 50, conn_param->winsize*1250); */

        //----Interval
		/* __set_interval(hw, conn_param->interval*1250, 0); */
        //----Channel map
		/* __set_channel_map(hw, conn_param->channel); */
        //----HW state 
		__set_hw_state(hw, MASTER_CONN_ST, 0, 0);
	}
	else
	{
        __disable_adv_random(hw);
        //----Winsize
		/* __set_winsize(hw, 50, conn_param->winsize*1250); */
        //----Winoffset
        __set_winoffset(hw, (conn_param->winoffset) ? conn_param->winoffset*2-1 : 0);
        /* puts("winoffset : ");put_u8hex(winoffset); */
        //----Interval
		/* __set_interval(hw, conn_param->interval*1250, 0); */
        //----HW state 
		__set_hw_state(hw, SLAVE_CONN_ST, !!conn_param->latency, conn_param->latency);
        //----Channel map
		/* __set_channel_map(hw, conn_param->channel); */
        __set_widen(hw, conn_param->widening);
	}
    //----Winsize
    __set_winsize(hw, 50, conn_param->winsize*1250);
    //----Interval
    __set_interval(hw, conn_param->interval*1250, 0);
    //----Channel map
    __set_channel_map(hw, conn_param->channel);
    //----Hop
    __set_conn_channel_index(hw, (conn_param->hop<<8)|conn_param->hop);
    //BLE_ANCHOR_CON0 = (7<<12)|(HW_ID(hw)<<8)|(1<<1)|0;

}


static void __hw_tx(struct ble_hw *hw, struct ble_tx *tx, int index)
{
	if (index == 0){
		if (tx->len){
			hw->ble_fp.TXPTR0 = PHY_TO_BLE(tx->data);
		}
		hw->ble_fp.TXAHDR0 = (tx->rxadd<<5)|(tx->txadd<<4)|tx->type;
		hw->ble_fp.TXDHDR0 = (tx->len<<8)|(tx->md<<3)|(tx->sn<<2)|tx->llid;
	} else {
		if (tx->len){
			hw->ble_fp.TXPTR1 = PHY_TO_BLE(tx->data);
		}
		hw->ble_fp.TXAHDR1 = (tx->rxadd<<5)|(tx->txadd<<4)|tx->type;
		hw->ble_fp.TXDHDR1 = (tx->len<<8)|(tx->md<<3)|(tx->sn<<2)|tx->llid;
	}
}


/* static void __set_scan_req_remote_addr(struct ble_hw *hw, const u8 *remote_addr) */
/* { */
    /* memcpy(hw->tx[0]->data + 6, remote_addr, 6); */
    /* memcpy(hw->tx[1]->data + 6, remote_addr, 6); */
/* } */

static void __connection_update(struct ble_hw *hw, struct ble_conn_param *param)
{
	int winoffset;

	if (hw->state == MASTER_CONN_ST){
		winoffset = param->winoffset*2;
	} else {
		winoffset = (param->winoffset) ? param->winoffset*2-1 : 0;
		__set_winsize(hw, 50, (param->winsize)*1250+625);
		__set_hw_state(hw, SLAVE_CONN_ST, !!param->latency, param->latency);
	}

    /* winoffset = 30;  */
	__set_winoffset(hw, winoffset);
	__set_interval(hw, param->interval*1250, 0);
    __set_widen(hw, param->widening);
}

static void le_hw_handler_register(struct ble_hw *hw, void *priv,
	   	const struct ble_handler *handler)
{
	hw->priv = priv;
	hw->handler = handler;
}


static void ble_tx_handler(struct ble_hw *hw)
{
	int i;
	struct ble_tx *tx;
	struct ble_tx empty;
	struct ble_param *ble_fp = &hw->ble_fp;

	if (hw->state == SLAVE_CONN_ST || hw->state == MASTER_CONN_ST)
	{
		if (!hw->rx_ack){
			putchar('N');
			return;
		}

		i = !(ble_fp->TXTOG & BIT(0));
		/* putchar('0'+i); */

		if (hw->tx[i]){
            if (hw->handler && hw->handler->tx_probe_handler){
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
            /* printf_buf(&empty, sizeof(empty)); */
		} else {
			putchar('G');
			put_u8hex(tx->data[0]);
			hw->tx[i] = tx;
			ble_hw_encrpty(&hw->encrypt, tx);
		}
		tx->sn = hw->tx_seqn;
		hw->tx_seqn = !hw->tx_seqn;
		__hw_tx(hw, tx, i);
	}
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
			__hw_tx(hw, hw->tx[0], i);
			__hw_tx(hw, hw->tx[1], !i);
			break;
		case SCAN_ST:
		case INIT_ST:
            /* putchar('#'); */
            __set_scan_channel_index(hw, hw->adv_channel);
            if (++hw->adv_channel > 39)
            {
				hw->adv_channel = 37;
			}
			break;
		case SLAVE_CONN_ST:
		case MASTER_CONN_ST:
            /* putchar('@'); */
            /* put_u8hex(ble_fp->LASTCHMAP); */
			if (hw->handler && hw->handler->event_handler){
				hw->handler->event_handler(hw->priv, ble_fp->EVTCOUNT);
			}
			break;
		default: return;
	}
}

static void backoff_pseudo(struct ble_hw *hw)
{
    u16 pseudo_random;
    /* After success or failure of receiving the */
    /* SCAN_RSP PDU, the Link Layer shall set backoffCount to a new pseudo-random */
    /* integer between one and upperLimit */
    pseudo_random = pseudo_random % hw->backoff.upper_limit;
    if (!pseudo_random)
        pseudo_random = 1;

    hw->backoff.backoff_pseudo_cnt = pseudo_random;
}

/********************************************************************************/
/*
 *                  Device Filtering(White List)
 */
static void __set_white_list_addr(struct ble_hw *hw, u8 addr_type, const u8 *addr)
{
	struct ble_param *ble_fp = &hw->ble_fp;

    addr_type = (addr_type) ? 1 : 0;
    ble_fp->FILTERCNTL = ble_fp->FILTERCNTL & 0x7f | (addr_type&0x1)<<8 | 1;

	ble_fp->WHITELIST0L = (addr[1] << 8) | addr[0];
	ble_fp->WHITELIST0M = (addr[3] << 8) | addr[2];
	ble_fp->WHITELIST0U = (addr[5] << 8) | addr[4];

    /* printf("addr type %x\n", addr_type); */

    /* printf("addr L %x\n", ble_fp->WHITELIST0L); */
    /* printf("addr M %x\n", ble_fp->WHITELIST0M); */
    /* printf("addr U %x\n", ble_fp->WHITELIST0U); */
}


static void __set_receive_encrypted(struct ble_hw *hw, u8 rx_enable)
{
    hw->encrypt.rx_enable = rx_enable;
}

static void __set_send_encrypted(struct ble_hw *hw, u8 tx_enable)
{
    hw->encrypt.tx_enable = tx_enable;
}

static void __set_privacy_enable(struct ble_hw *hw, u8 enable)
{
    hw->privacy_enable = enable;
}
/********************************************************************************/
static void ble_rx_adv_process(struct ble_hw *hw, struct ble_rx *rx)
{
	struct ble_param *ble_fp = &hw->ble_fp;

    if(rx->type == SCAN_REQ)
    {
        __set_peer_addr(hw, rx->txadd, rx->data);
        putchar('S');
    }
    else if(rx->type == CONNECT_REQ)
    {
        int	sn =  ble_fp->TXTOG & BIT(0);

        DEBUG_IO_1(2);
        ble_fp->TXAHDR0 = rx->rxadd<<4;
        ble_fp->TXDHDR0 = (0<<8)|(sn<<2)|1;
        ble_fp->TXAHDR1 = rx->rxadd<<4;
        ble_fp->TXDHDR1 = (0<<8)|(!sn<<2)|1;

        hw->tx_seqn = 0;
        ble_tx_init(hw);
    }
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
                    puts("++");
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
        /* printf_buf(rx->data, 6); */
    }
    else if(rx->type == SCAN_RSP)
    {
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
            puts("--");
        }
     }
}



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
            case ADV_NONCONN_IND:
            case SCAN_RSP:
            case ADV_DIRECT_IND:
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
                if (hw->privacy_enable == 0)
                {
                    if ((hw->peer.addr_type != rx->txadd) 
                        || (memcmp(hw->peer.addr, rx->data, 6)))
                    {
                        return FALSE;
                    }
                    //privacy RPA bypass
                }
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

static void ble_rx_pdus_process(struct ble_hw *hw, struct ble_rx *rx)
{
	struct ble_param *ble_fp = &hw->ble_fp;

	switch(hw->state)
	{
		case ADV_ST:
            ble_rx_adv_process(hw, rx);
			break;
		case SCAN_ST:
            ble_rx_scan_process(hw, rx);
			break;
		case INIT_ST:
			if((rx->type==ADV_IND) || (rx->type==ADV_SCAN_IND)) 
			{
				struct ble_tx *tx;

				ble_tx_init(hw);

				hw->tx_seqn = 0;
				/*ble_rx_post(hw, rx);
				tx = lbuf_pop(hw->lbuf_tx);
				ASSERT(tx != NULL, "%s\n", "Get version faild");
				tx->txadd = rx->rxadd;*/
				/*if (!(ble_fp->TXTOG & BIT(0))) {
					__hw_tx(ble_fp, tx, 0);
					ble_fp->TXAHDR1 = tx->txadd<<4;
					ble_fp->TXDHDR1 = (0<<8)|(1<<2)|1;
					hw->tx[0] = tx;
				} else {
					__hw_tx(ble_fp, tx, 1);
					ble_fp->TXAHDR0 = tx->txadd<<4;
					ble_fp->TXDHDR0 = (0<<8)|(1<<2)|1;
					hw->tx[1] = tx;
				}*/

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
			break;
		case SLAVE_CONN_ST:
		case MASTER_CONN_ST:
			ble_hw_decrypt(&hw->encrypt, rx);
			break;
	}
}


static void ble_rx_probe(struct ble_hw *hw, struct ble_rx *rx)
{
    if (ble_rx_pdus_filter(hw, rx) == FALSE)
    {
        //ignore packet
        putchar('~');
        lbuf_free(rx);
        return; 
    }

    ble_rx_pdus_process(hw, rx);

    //async PDUs upper to Link Layer
	/* if (rx->llid != 1 || rx->len != 0) */
    {
        /* printf_buf(rx, sizeof(*rx)+rx->len); */
        lbuf_push(rx);
    }

    //sync PDUs upper to Link Layer
    if (hw->handler && hw->handler->rx_probe_handler){
       hw->handler->rx_probe_handler(hw->priv, rx);
    }
}

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
            /* printf_buf(rx, sizeof(*rx)+rx->len); */
            handler(hw->priv, rx);
            lbuf_free(rx);
        }
    }
}

static void __hw_rx_process(struct ble_hw *hw)
{
	int ind;
	struct ble_rx *rx;
	u8 rx_check;
	int rxahdr, rxdhdr, rxstat;
	u16 *rxptr;
	struct ble_param *ble_fp = &hw->ble_fp;


	ind = !(ble_fp->RXTOG & BIT(0));
	rxahdr = *((u16 *)&ble_fp->RXAHDR0 + ind);
	rxdhdr = *((u16 *)&ble_fp->RXDHDR0 + ind);
	rxstat = *((u16 *)&ble_fp->RXSTAT0 + ind);
	rxptr  =  (u16 *)&ble_fp->RXPTR0   + ind;

	rx = hw->rx[ind];
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
		putchar('E');put_u8hex(rx_check); putchar('\n');
        //TO*DO
        return;
	}
#ifndef FPGA
	fsk_offset(1);
#endif
	ble_hw_encrypt_check(hw);
    //data buf loop
	if (rx->llid!=1 || rx->len!=0){
		putchar('R');
        //baseband loop buf switch
        //TO*DO
	}
    hw->rx[ind] = ble_hw_alloc_rx(hw, 40);
    *rxptr = PHY_TO_BLE(hw->rx[ind]->data);

    ble_rx_probe(hw, rx);


	ble_tx_handler(hw);
}

static void ble_irq_handler()
{
	int i;

   /* printf("BLE_INT_CON0 %0x\n",BLE_INT_CON0); */
   /* printf("BLE_INT_CON1 %0x\n",BLE_INT_CON1); */
	for (i=0; i<8; i++)
	{
        //must first deal
		if(BLE_INT_CON2 & BIT(i+8))//rx int
		{
            DEBUG_IO_1(0)
            /* trig_fun(); */
            /* ADC_CON = 1; */
			__hw_rx_process(&ble_base.hw[i]);
            DEBUG_IO_0(0)

			BLE_INT_CON1 |= BIT(i+8);
			BLE_INT_CON1 |= BIT(i+8);     ///  !!! must be clear twice
		}

        //must deal after rx
		if(BLE_INT_CON2 & BIT(i))//event_end int
		{
            DEBUG_IO_1(1)
			__hw_event_process(&ble_base.hw[i]);

            DEBUG_IO_0(1)
			BLE_INT_CON1 = BIT(i);
			BLE_INT_CON1 = BIT(i);     ///  !!! must be clear twice
		}
	}
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

	memcpy(tx->data + 12, (u8 *)&conn->ll_data, 16);
	memcpy(tx->data + 12 + 16, ((u8 *)&conn->ll_data) + 18, 6);

	return tx;
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

    /* printf("FILTERCNTL %x\n", ble_fp->FILTERCNTL); */
}

static void le_hw_advertising(struct ble_hw *hw, struct ble_adv *adv)
{
	puts("hw_advertising\n");

	__set_winsize(hw, 50, 50);
	__set_interval(hw, adv->interval*625, 1);
	__set_hw_state(hw, ADV_ST, 0, 3);

    __set_adv_device_filter_param(hw, adv->filter_policy); 

	hw->ble_fp.ADVIDX=(adv->pkt_cnt<<14)|adv->pdu_interval*625; 	//pkt_cnt:interval N*us

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

	__hw_event_process(hw);

	ble_hw_enable(hw, 10);
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

    /* printf("FILTERCNTL %x\n", ble_fp->FILTERCNTL); */
}

static void le_hw_scanning(struct ble_hw *hw, struct ble_scan *scan)
{
	puts("hw_scanning\n");
	struct ble_tx *tx;

	__set_winsize(hw, 50, scan->window*625);
	__set_interval(hw, scan->interval*625, 0);
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
	__hw_event_process(hw);

	tx = __scan_req_pdu(hw, scan);
    /* hw->tx[0] = tx; */
    /* hw->tx[1] = tx; */

	__hw_tx(hw, tx, 0);
	__hw_tx(hw, tx, 1);

	ble_hw_enable(hw, 1);
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

    /* printf("FILTERCNTL %x\n", ble_fp->FILTERCNTL); */
}

void le_hw_initiating(struct ble_hw *hw, struct ble_conn *conn)
{
	puts("hw_initiating\n");
	struct ble_tx *tx;
    struct ble_conn_param *conn_param = &conn->ll_data;

	__set_winsize(hw, 50, conn->scan_windows*625);
	__set_interval(hw, conn->scan_interval*625, 0);
	__set_hw_state(hw, INIT_ST, 0, 0);

    __set_init_device_filter_param(hw, conn->filter_policy);

	hw->adv_channel = 37;
	__hw_event_process(hw);

	tx = __connect_req_pdu(hw, conn);

    puts("conn req : ");
    printf_buf(hw->peer.addr, 6);
	__hw_tx(hw, tx, !(hw->ble_fp.TXTOG & BIT(0)));
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

	le_hw_standby(hw);

	return hw;
}

static void le_hw_close(struct ble_hw *hw)
{
	ble_hw_disable(hw);
	ble_hw_frame_free(hw);
}
int event_cnt =0;
static void le_hw_send_packet(struct ble_hw *hw, u8 llid, u8 *packet, int len)
{
	struct ble_tx *tx = le_hw_alloc_tx(hw, 0, llid, len);

	ASSERT(tx != NULL);

	/* printf("send_packet: %x, %x, %d\n", hw, tx, len); */

	memcpy(tx->data, packet, len);

	lbuf_push(tx);
	/* puts("exit\n"); */
	/*if (++event_cnt >= 3){*/
		/*puts("stop\n");*/
		/*while(1);*/
	/*}*/
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
			__set_instant(hw, va_arg(argptr, int));
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
        case BLE_SET_PRIVACY_ENABLE:
            __set_privacy_enable(hw, va_arg(argptr, int));
		default:
			break;
	}

	va_end(argptr);
}



/*void ble_hw_probe (struct platform_device *dev)
{
	dev->priv = &ble_ops;

	int irq = platform_get_irq(dev, 0);

	if (irq >= 0){
		request_irq(irq, ble_irq_handler, IRQF_SHARED, NULL);
	}
}

void ble_hw_remove(struct platform_device *dev)
{
}

const struct platform_driver ble_driver = {
	.name = "ble",
	.probe = ble_hw_probe,
	.remove = ble_hw_remove,
};*/


static void ble_hw_init()
{
	puts("ble_hw_init\n");


	BT_BLEEXM_ADR = (u32)&ble_base;
	BT_BLEEXM_LIM = ((u32)&ble_base) + sizeof(ble_base);

	memset(&ble_base.inst, 0, sizeof(ble_base.inst));

    INIT_LIST_HEAD(&bb.head);

	request_irq(IRQ_BLE, ble_irq_handler, IRQF_SHARED, NULL);
}

void ble_rf_init(void)
{
	SFR(BLE_RADIO_CON0, 0 , 10 , 150);    //TXPWRUP_OPEN_PLL
	SFR(BLE_RADIO_CON0, 12 , 4 , 12);     //TXPWRPD_CLOSE_LOD
	SFR(BLE_RADIO_CON1, 0 , 10 , 145);    //RXPWRUP_OPEN_PLL
	SFR(BLE_RADIO_CON2, 0 , 8 ,  50);     //TXCNT_OPEN_LDO
	SFR(BLE_RADIO_CON2, 8 , 8 ,  60);     //RXCNT_OPEN_LDO
	SFR(BLE_RADIO_CON3, 0 , 8 ,  30);     //wait read data

	BLE_CON0  = BIT(0);          //ble_en
	BLE_CON1  = 3;               //sync_err
    //-------debug sfr
    BLE_DBG_CON = BIT(0) | (10<<8);//EN NUM
    //BT_EXT_CON |=BIT(1);  //DBG EN
    //BT_EXT_CON |=BIT(5); //1:BLE 0:BREDR
    //PORTB_DIR &= 0xf00f;
    //-------debug sfr
}

static int le_hw_get_id(struct ble_hw *hw)
{
     return BIT(HW_ID(hw));
}

static int le_hw_get_conn_event(struct ble_hw *hw)
{
    return hw->ble_fp.EVTCOUNT;
}

static const struct ble_operation ble_ops = {
	.init = ble_hw_init,
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
    .get_conn_event = le_hw_get_conn_event,
};
REGISTER_BLE_OPERATION(ble_ops);




