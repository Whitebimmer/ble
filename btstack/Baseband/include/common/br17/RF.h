#ifndef _RF_H_
#define _RF_H_


//FPGA
#define BTACON0   0
#define BTACON1   1
#define BTACON2   2
#define BTACON3   3
#define BTACON4   4
#define BTACON5   5
#define BTACON6   6
#define BTACON7   7
#define BTACON8   8
#define BTACON9   9
#define BTACON10   10
#define BTACON11   11
#define BTACON12   12
#define BTACON13   13
#define RFICON    14
#define LDOCON    15



//*********************************************************************
//
// RF common register control
//
//*********************************************************************
#define    BT_SFR_ADR              0XEFC00

#define    BT_MDM_CON0             (*(volatile unsigned long *)(BT_SFR_ADR + 0x00*4))
#define    BT_MDM_CON1             (*(volatile unsigned long *)(BT_SFR_ADR + 0x01*4))
#define    BT_MDM_CON2             (*(volatile unsigned long *)(BT_SFR_ADR + 0x02*4))
#define    BT_MDM_CON3             (*(volatile unsigned long *)(BT_SFR_ADR + 0x03*4))
#define    BT_MDM_CON4             (*(volatile unsigned long *)(BT_SFR_ADR + 0x04*4))
#define    BT_MDM_CON5             (*(volatile unsigned long *)(BT_SFR_ADR + 0x05*4))
#define    BT_MDM_CON6             (*(volatile unsigned long *)(BT_SFR_ADR + 0x06*4))
#define    BT_MDM_CON7             (*(volatile unsigned long *)(BT_SFR_ADR + 0x07*4))
#define    BT_RSSIDAT0             (*(volatile unsigned long *)(BT_SFR_ADR + 0x08*4))
#define    BT_RSSIDAT1             (*(volatile unsigned long *)(BT_SFR_ADR + 0x09*4))
#define    BT_RSSIDAT2             (*(volatile unsigned long *)(BT_SFR_ADR + 0x0a*4))
#define    BT_RSSIDAT3             (*(volatile unsigned long *)(BT_SFR_ADR + 0x0b*4))
#define    BT_FC_CNT               (*(volatile unsigned long *)(BT_SFR_ADR + 0x0c*4))
#define    BT_MDM_CON8             (*(volatile unsigned long *)(BT_SFR_ADR + 0x0d*4))

#define    BT_BSB_CON              (*(volatile unsigned long *)(BT_SFR_ADR + 0x10*4))
#define    BT_BREDREXM_ADR         (*(volatile unsigned long *)(BT_SFR_ADR + 0x11*4))
#define    BT_BSB_EDR              (*(volatile unsigned long *)(BT_SFR_ADR + 0x12*4))
#define    BT_BSB_INF              (*(volatile unsigned long *)(BT_SFR_ADR + 0x13*4))
#define    BT_EXT_CON              (*(volatile unsigned long *)(BT_SFR_ADR + 0x14*4))
#define    BT_PCM_CON              (*(volatile unsigned long *)(BT_SFR_ADR + 0x15*4))
#define    BT_PCM_WADR             (*(volatile unsigned long *)(BT_SFR_ADR + 0x16*4))
#define    BT_PCM_RADR             (*(volatile unsigned long *)(BT_SFR_ADR + 0x17*4))
#define    BT_LOFC_CON             (*(volatile unsigned long *)(BT_SFR_ADR + 0x18*4))
#define    BT_LOFC_RES             (*(volatile unsigned long *)(BT_SFR_ADR + 0x19*4))
#define    BT_ANL_RXPRD            (*(volatile unsigned long *)(BT_SFR_ADR + 0x1a*4))
#define    BT_PHCOM_CNT            (*(volatile unsigned long *)(BT_SFR_ADR + 0x1b*4))
#define    BT_LP_CON               (*(volatile unsigned long *)(BT_SFR_ADR + 0x1c*4))

#define    BT_BLE_CON              (*(volatile unsigned long *)(BT_SFR_ADR + 0x20*4))
#define    BT_BLEEXM_ADR           (*(volatile unsigned long *)(BT_SFR_ADR + 0x21*4))
#define    BT_PLLCONFIG_ADR        (*(volatile unsigned long *)(BT_SFR_ADR + 0x22*4))
#define    BT_BLEEXM_LIM           (*(volatile unsigned long *)(BT_SFR_ADR + 0x2f*4))


#define    BT_CLKN_CON             (*(volatile unsigned long *)(BT_SFR_ADR + 0x50*4))
#define    BT_CLKN0_PRD            (*(volatile unsigned long *)(BT_SFR_ADR + 0x51*4))
#define    BT_CLKN1_PRD            (*(volatile unsigned long *)(BT_SFR_ADR + 0x52*4))
#define    BT_CLKN2_PRD            (*(volatile unsigned long *)(BT_SFR_ADR + 0x53*4))
#define    BT_CLKN3_PRD            (*(volatile unsigned long *)(BT_SFR_ADR + 0x54*4))
#define    BT_CLKN4_PRD            (*(volatile unsigned long *)(BT_SFR_ADR + 0x55*4))
#define    BT_CLKN5_PRD            (*(volatile unsigned long *)(BT_SFR_ADR + 0x56*4))
#define    BT_CLKN6_PRD            (*(volatile unsigned long *)(BT_SFR_ADR + 0x57*4))
#define    BT_CLKN7_PRD            (*(volatile unsigned long *)(BT_SFR_ADR + 0x58*4))

#define    BT_CLKN_PRD          ((volatile unsigned long *)(BT_SFR_ADR + 0x51*4))



#include "typedef.h"


struct common_rf_param{
    u8 delay_sys;
    u8 pll_para;
    u8 pll_hen;
    u8 low_powr_en;
    u8 pll_bank_drop;
    u8 pll_bank_rise;
    u8 bta_pll_bank_limit[15];
};

extern struct common_rf_param rf;

struct rf_analog_trim
{
    u16 crc;
    s16 i_dc;
	s16 q_dc;
	u8 bw_set;
}__attribute__((packed));

extern struct rf_analog_trim trim_info;

#endif

