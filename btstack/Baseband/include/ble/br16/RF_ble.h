#ifndef RF_BLE_H
#define RF_BLE_H


#include "typedef.h"
#include "ble_regs.h"
#include "list.h"



struct ble_param
{
    volatile unsigned short ANCHOR; 
    volatile unsigned short BITOFF; 
    volatile unsigned short ADVIDX; 
    volatile unsigned short FORMAT; 
    volatile unsigned short OPTCNTL; 
    volatile unsigned short BDADDR0; 
    volatile unsigned short BDADDR1; 
    volatile unsigned short TXTOG; 
    volatile unsigned short RXTOG; 
    volatile unsigned short TXPTR0; 
    volatile unsigned short TXPTR1; 
    volatile unsigned short TXAHDR0; 
    volatile unsigned short TXAHDR1; 
    volatile unsigned short TXDHDR0; 
    volatile unsigned short TXDHDR1; 
    volatile unsigned short RXPTR0; 
    volatile unsigned short RXPTR1; 
    volatile unsigned short WINCNTL0; 
    volatile unsigned short WINCNTL1; 
    volatile unsigned short RXAHDR0; 
    volatile unsigned short RXAHDR1; 
    volatile unsigned short RXDHDR0; 
    volatile unsigned short RXDHDR1; 
    volatile unsigned short CHMAP0; 
    volatile unsigned short CHMAP1; 
    volatile unsigned short CHMAP2; 
    volatile unsigned short LASTCHMAP; 
    volatile unsigned short RXMAXBUF; 
    volatile unsigned short RXSTAT0; 
    volatile unsigned short RXSTAT1; 
    volatile unsigned short FILTERCNTL;
    volatile unsigned short WHITELIST0L;
    volatile unsigned short WHITELIST0M;
    volatile unsigned short WHITELIST0U;
    volatile unsigned short CRCWORD0;
    volatile unsigned short CRCWORD1;
    volatile unsigned short WIDEN0;
    volatile unsigned short WIDEN1;
    volatile unsigned short TARGETADRL;
    volatile unsigned short TARGETADRM;
    volatile unsigned short TARGETADRU;
    volatile unsigned short LOCALADRL;
    volatile unsigned short LOCALADRM;
    volatile unsigned short LOCALADRU;
    volatile unsigned short EVTCOUNT;
    volatile unsigned short RXBIT;
    volatile unsigned short IFSCNT;
    volatile unsigned short RFPRIOSTAT; // 
    volatile unsigned short RFPRIOCNTL; //31
    volatile unsigned short UNDEF1; 

    volatile unsigned char  FRQ_IDX0[40];
    volatile unsigned char  FRQ_IDX1[40];

    volatile unsigned short RSSI0        ; //16'd176     //len:104 [31:0] read only RSSI 
    volatile unsigned short RSSI1        ; //16'd176     //len:104 [31:0] read only RSSI 
    volatile unsigned long  TXPWER     ; //16'd180     //len:108 [8:0]  
    volatile unsigned long  RXGAIN0    ; //16'd184     //len:112 [12:0]        
    volatile unsigned long  RXTXSET    ; //16'd188     //len:116 RX:[20:12]  TX[8:0] 
    volatile unsigned long  OSC_SET     ; //16'd192     //len:120 SET OSC CAP [8:0] 
    volatile unsigned long  ANLCNT0     ; //16'd196     //len:124 ldo: rx[15:8] tx[7:0] en: rx[31:24] tx[23:16]    
    volatile unsigned long  ANLCNT1     ; //16'd200     //len:128 powd[12:8]] pll_rst[3:0]         
    volatile unsigned long  MDM_SET     ; //16'd204     //len:132 [31:0]
    volatile unsigned short RSSI2        ; //16'd176     //len:104 [31:0] read only RSSI 
    volatile unsigned short RSSI3        ; //16'd176     //len:104 [31:0] read only RSSI 
    volatile unsigned long  UNDEF[3];     // 208 212 216  
    volatile unsigned short FRQ_TBL0[40]; //16'd220
    volatile unsigned short FRQ_TBL1[40];
};



//format
#define STANDBY_ST         0
#define SCAN_ST            1
#define ADV_ST             2
#define INIT_ST            3
#define M_CONN_SETUP_ST    4
#define S_CONN_SETUP_ST    5
#define MASTER_CONN_ST     6
#define SLAVE_CONN_ST      7


#define PUBILC_ACCESS_ADR0      0xBED6
#define PUBILC_ACCESS_ADR1      0x8E89

//rx tx type ADV_PDU
#define ADV_IND            0       //37
#define ADV_DIRECT_IND     1       //12
#define ADV_NONCONN_IND    2       //37
#define ADV_SCAN_IND       6       //37
#define SCAN_REQ           3       //12
#define SCAN_RSP           4       //37
#define CONNECT_REQ        5       //34



struct ble_encrypt{
	u8 rx_enable;
	u8 tx_enable;
	u8 tx_counter_h;
	u8 rx_counter_h;
	u32 tx_counter_l;
	u32 rx_counter_l;
	u8 iv[8];
	u8 skd[16];
};

struct ble_backoff{
    u8 active;
    u8  send;
    u16 upper_limit;
    u16 backoff_count;
    u16 backoff_pseudo_cnt;
    u8  consecutive_succ;
    u8  consecutive_fail;
};

struct ble_addr{
	u8 addr_type;
	u8 addr[6];
};

struct ble_hw{
	u8 rx_ack;
	u8 tx_seqn;
	u8 adv_channel;
	u8 state;
	u8 agc;
	u8 power;

    u32 clkn_cnt;
    u32 interval;

    struct ble_addr local;
    struct ble_addr peer;

	struct ble_rx 			*rx[2];
	struct ble_tx 			*tx[2];
	struct lbuff_head 		*lbuf_rx;
	struct lbuff_head 		*lbuf_tx;
	struct ble_param 		ble_fp;
	struct ble_encrypt 		encrypt;
    // struct ble_conn_param   conn_param;
	struct list_head        rx_oneshot_head;
	struct list_head        event_oneshot_head;
    struct ble_backoff      backoff;

    //LL layer 
	void *priv;
	const struct ble_handler *handler;
	struct list_head entry;

    u8 privacy_enable;

    void *power_ctrl;
    u32 *regs;

    u16 rx_octets;
    u16 tx_octets;
};

#define BLE_HW_NUM    1

#define BLE_HW_RX_SIZE  (512*1)
#define BLE_HW_TX_SIZE  512

struct ble_hw_base {
	u16 inst[8];
	struct ble_hw hw[BLE_HW_NUM];
	u8 rx[BLE_HW_NUM][BLE_HW_RX_SIZE];
	u8 tx[BLE_HW_NUM][BLE_HW_TX_SIZE];
};





















#endif

