#ifndef BLE_REGS_H 
#define BLE_REGS_H  

//*********************************************************************
//
// BLE register control
//
//*********************************************************************
#define BLE_AHB_BASE_ADR    0xE8000L

#define    BLE_CON0        (*(volatile unsigned long *)(BLE_AHB_BASE_ADR + 0x0 *4))
#define    BLE_CON1        (*(volatile unsigned long *)(BLE_AHB_BASE_ADR + 0x1 *4))
#define    BLE_RADIO_CON0  (*(volatile unsigned long *)(BLE_AHB_BASE_ADR + 0x2 *4))
#define    BLE_RADIO_CON1  (*(volatile unsigned long *)(BLE_AHB_BASE_ADR + 0x3 *4))
#define    BLE_RADIO_CON2  (*(volatile unsigned long *)(BLE_AHB_BASE_ADR + 0x4 *4))
#define    BLE_RADIO_CON3  (*(volatile unsigned long *)(BLE_AHB_BASE_ADR + 0x5 *4))
#define    BLE_ANCHOR_CON0 (*(volatile unsigned long *)(BLE_AHB_BASE_ADR + 0x6 *4))
#define    BLE_ANCHOR_CON1 (*(volatile unsigned long *)(BLE_AHB_BASE_ADR + 0x7 *4))
#define    BLE_INT_CON0     (*(volatile unsigned long *)(BLE_AHB_BASE_ADR + 0x8 *4))
#define    BLE_INT_CON1     (*(volatile unsigned long *)(BLE_AHB_BASE_ADR + 0x9 *4))
#define    BLE_INT_CON2     (*(volatile unsigned long *)(BLE_AHB_BASE_ADR + 0xa *4))
#define    BLE_DBG_CON     (*(volatile unsigned long *)(BLE_AHB_BASE_ADR + 0xb *4))




#endif
