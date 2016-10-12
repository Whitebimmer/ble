/*******************************************************************************************
 File Name: HWI.h

 Version: 1.00

 Discription:


 Author:Bingquan Cai

 Email :change.tsai@gmail.com

 Date:2013-09-14 16:47:39

 Copyright:(c)JIELI  2011  @ , All Rights Reserved.
*******************************************************************************************/
#ifndef HWI_h
#define HWI_h
#include "typedef.h"
#include "br16.h"

// #define TIME0_INIT   2
// #define TIME1_INIT   3
// #define TIME2_INIT   4
// #define TIME3_INIT   5

// #define IRQ_BT  		25
// #define IRQ_BLE      	26
// #define IRQ_BLE_DBG   	27

#define EXCEPTION_INIT  64

#define TIME0_INIT      2
#define TIME1_INIT      3
#define TIME2_INIT      4
#define TIME3_INIT      5
#define USB_SOF         6
#define USB_CTRL        7
#define RTC_INIT        8
#define ALINK_INIT      9
#define DAC_INIT        10
#define PORT_INIT       11
#define SPI0_INIT       12
#define SPI1_INIT       13
#define SD0_INIT        14
#define SD1_INIT        15
#define UART0_INIT      16
#define UART1_INIT      17
#define UART2_INIT      18
#define PAP_INIT        19
#define IIC_INIT        20
#define SARADC_INIT     21
#define FM_HWFE_INIT    22
#define FM_INIT         23
#define FM_LOFC_INIT    24
#define IRQ_BT  		25
#define IRQ_BREDR       26
#define IRQ_BLE_DBG     27
#define BT_PCM_INIT     28
#define NFC_INIT        29
#define IRQ_BLE         36
#define SOFT0_INIT      62
#define SOFT_INIT       63

#define REG_INIT_HANDLE(a)\
    extern void _##a();\
    void handle__##a()\
    {\
        __asm__ volatile ("pushs {psr, rets, reti}"); \
        __asm__ volatile ("push  {r3,r2,r1,r0}"); \
        __asm__ volatile ("call %0" : : "i" (a)); \
        __asm__ volatile ("pop  {r3,r2,r1,r0}"); \
        __asm__ volatile ("pops {psr, rets, reti}"); \
        __asm__ volatile ("rti"); \
    }

#define INTALL_HWI(a,b,c)\
    extern void handle__##b();\
    HWI_Install(a,handle__##b,c)

void hwi_init(void);

void HWI_Install(unsigned int index, unsigned int isr, unsigned int Priority);

void ENABLE_INT(void);

void DISABLE_INT(void);

void clear_ie(u32 index);

void set_ie(u32 index);

void DisableOSInt();

void EnableOSInt();

void trig_fun();

u16 CRC16(void *ptr, u32  len);

void default_isr_handler();
#endif



