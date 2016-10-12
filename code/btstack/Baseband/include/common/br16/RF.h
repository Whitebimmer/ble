#ifndef _RF_H
#define _RF_H

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
#define    BT_BLEEXM_LIM           (*(volatile unsigned long *)(BT_SFR_ADR + 0x2f*4))
#define    BTA_CON0                (*(volatile unsigned long *)(BT_SFR_ADR + 0x30*4))
#define    BTA_CON1                (*(volatile unsigned long *)(BT_SFR_ADR + 0x31*4))
#define    BTA_CON2                (*(volatile unsigned long *)(BT_SFR_ADR + 0x32*4))
#define    BTA_CON3                (*(volatile unsigned long *)(BT_SFR_ADR + 0x33*4))
#define    BTA_CON4                (*(volatile unsigned long *)(BT_SFR_ADR + 0x34*4))
#define    BTA_CON5                (*(volatile unsigned long *)(BT_SFR_ADR + 0x35*4))
#define    BTA_CON6                (*(volatile unsigned long *)(BT_SFR_ADR + 0x36*4))
#define    BTA_CON7                (*(volatile unsigned long *)(BT_SFR_ADR + 0x37*4))
#define    BTA_CON8                (*(volatile unsigned long *)(BT_SFR_ADR + 0x38*4))
#define    BTA_CON9                (*(volatile unsigned long *)(BT_SFR_ADR + 0x39*4))
#define    BTA_CON10               (*(volatile unsigned long *)(BT_SFR_ADR + 0x3a*4))
#define    BTA_CON11               (*(volatile unsigned long *)(BT_SFR_ADR + 0x3b*4))
#define    BTA_CON12               (*(volatile unsigned long *)(BT_SFR_ADR + 0x3c*4))
#define    BTA_CON13               (*(volatile unsigned long *)(BT_SFR_ADR + 0x3d*4))
#define    BTA_CON14               (*(volatile unsigned long *)(BT_SFR_ADR + 0x3e*4))
#define    BTA_CON15               (*(volatile unsigned long *)(BT_SFR_ADR + 0x3f*4))
#define    BTA_CON16               (*(volatile unsigned long *)(BT_SFR_ADR + 0x40*4))
#define    BTA_CON17               (*(volatile unsigned long *)(BT_SFR_ADR + 0x41*4))
#define    BTA_CON18               (*(volatile unsigned long *)(BT_SFR_ADR + 0x42*4))
#define    BTA_CON19               (*(volatile unsigned long *)(BT_SFR_ADR + 0x43*4))
#define    BTA_CON20               (*(volatile unsigned long *)(BT_SFR_ADR + 0x44*4))
#define    BTA_CON21               (*(volatile unsigned long *)(BT_SFR_ADR + 0x45*4))
#define    BTA_CON22               (*(volatile unsigned long *)(BT_SFR_ADR + 0x46*4))
#define    BTA_CON23               (*(volatile unsigned long *)(BT_SFR_ADR + 0x47*4))
#define    BTA_CON24               (*(volatile unsigned long *)(BT_SFR_ADR + 0x48*4))

// For FPGA SPI  
#define BTACON0       0
#define BTACON1       1
#define BTACON2       2
#define BTACON3       3
#define BTACON4       4
#define BTACON5       5
#define BTACON6       6
#define BTACON7       7
#define BTACON8       8
#define BTACON9       9
#define BTACON10      10
#define BTACON11      11
#define BTACON12      12
#define BTACON13      13
#define RFICON        14
#define LDOCON        15

#endif

