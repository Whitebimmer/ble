/*******************************************************************************************
 File Name: HWI.c

 Version: 1.00

 Discription:  Ó²¼þÖÐ¶Ï¹ÜÀíº¯Êý


 Author:yulin deng

 Email :flowingfeeze@163.com

 Date:2013-09-05 11:33:18

 Copyright:(c)JIELI  2011  @ , All Rights Reserved.
*******************************************************************************************/
#ifndef HWI_c
#define HWI_c

#include "typedef.h"
#include "bt16.h"
#include "hwi.h"


inline void hwi_init(void)
{
    unsigned long tmp = 0 ;
    asm("EMUDAT = %0 ;" : : "r"(tmp));
}

//void clear_ie(u32 index) AT(.rom_api);
void clear_ie(unsigned long index)
{
    unsigned long tmp ;
    asm("%0 = EMUDAT ;" : "=r"(tmp));
    tmp &= ~BIT(index) ;
    asm("EMUDAT = %0 ;" : : "r"(tmp));
}
//void set_ie(u32 index) AT(.rom_api);
void set_ie(unsigned long index)
{
    unsigned long tmp ;
    asm("%0 = EMUDAT ;" : "=r"(tmp));
    tmp |= BIT(index) ;
    asm("EMUDAT = %0 ;" : : "r"(tmp));
}

//void HWI_Install(u32 index, u32 isr, u32 Priority) AT(.rom_api);
void HWI_Install(unsigned long index, unsigned long isr, unsigned long Priority)
{
    unsigned long tmp ;
    unsigned long *israddr = (unsigned long *)0x1ff80;
    unsigned long a, b ;

    a = (index - 2) / 10;
    b = ((index - 2) % 10) * 3 ;
    israddr[index] = isr ;

    switch(a)
    {
        case 0 :
            IPCON0 &= ~(0x07 << (b));
            IPCON0 |= ((Priority & 0x07) << (b));
            break ;

        case 1:
            IPCON1 &= ~(0x07 << (b));
            IPCON1 |= ((Priority & 0x07) << (b));
            break ;

        default:
            IPCON2 &= ~(0x07 << (b));
            IPCON2 |= ((Priority & 0x07) << (b));

    }

    DSP_CLR_ILAT |= BIT(index);

    if(index > 1)
    {
        asm("%0 = EMUDAT ;" : "=r"(tmp));
        tmp |= BIT(index) ;
        asm("EMUDAT = %0 ;" : : "r"(tmp));
    }
}

#endif
