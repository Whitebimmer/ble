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
#include "br17.h"
#include "hwi.h"

#define ISR_ENTRY   0x1e000L

void hwi_init(void)
{
    unsigned int i;
    u32 *israddr=(u32 *)ISR_ENTRY;//   no dcatch init enter = 0xff80;
//  vm_mutex_init();
    int tmp;
    tmp = 0 ;
    __asm__ volatile("mov icfg,%0" : : "r"(tmp));

    for (i=1;i<32;i++)
    {
        israddr[i] = (u32)default_isr_handler;
    }
}

void HWI_Install(unsigned int index, unsigned int isr, unsigned int Priority)
{
    unsigned int tmp;
    unsigned int a, b;
    unsigned int *israddr = (unsigned int *)ISR_ENTRY;
    a =  index / 16;
    b = (index % 16) * 2;
    printf("HWI Install : %x-%x\n", index, Priority);
    israddr[index] = isr;
    if (index == 64)
        return;
    switch(a) {
        case 0 :
            IPCON0 &= ~(0x3 << (b));
            IPCON0 |= ((Priority & 0x3) << (b));
            break ;
        case 1:
            IPCON1 &= ~(0x3 << (b));
            IPCON1 |= ((Priority & 0x3) << (b));
            break ;
        case 2:
            IPCON2 &= ~(0x3 << (b));
            IPCON2 |= ((Priority & 0x3) << (b));
            break ;
        default:
            IPCON3 &= ~(0x3 << (b));
            IPCON3 |= ((Priority & 0x3) << (b));
    }
    if(index < 32) {
        ILAT0_CLR = BIT(index);
        __asm__ volatile ("mov %0,ie0" : "=r"(tmp));
        tmp |= (BIT(index) & 0xfffffffe);
        __asm__ volatile ("mov ie0,%0" :: "r"(tmp));
    } else
    {
        index = index-32;
        ILAT1_CLR = BIT(index);
        __asm__ volatile ("mov %0,ie1" : "=r"(tmp));
        tmp |= BIT(index);
        __asm__ volatile ("mov ie1,%0" :: "r"(tmp));
    }
}

void ENABLE_INT(void)
{
    int tmp;
    __asm__ volatile("mov %0,icfg" : "=r"(tmp));
    tmp |= BIT(8) ;
    __asm__ volatile("mov icfg,%0" : : "r"(tmp));
}

void DISABLE_INT(void)
{
    int tmp;
    __asm__ volatile("mov %0,icfg" : "=r"(tmp));
    tmp &= ~BIT(8) ;
    __asm__ volatile("mov icfg,%0" : : "r"(tmp));
}

void clear_ie(u32 index)
{
    int tmp;
    if(index < 32)
    {
        __asm__ volatile ("mov %0,ie0" : "=r"(tmp));
        tmp &= ~(BIT(index) & 0xfffffffe);
        __asm__ volatile ("mov ie0,%0" :: "r"(tmp));
    }
    else
    {
        index = index-32;
        __asm__ volatile ("mov %0,ie1" : "=r"(tmp));
        tmp &= ~BIT(index);
        __asm__ volatile ("mov ie1,%0" :: "r"(tmp));
    }

}

void set_ie(u32 index)
{
    int tmp;
    if(index < 32)
    {
        __asm__ volatile ("mov %0,ie0" : "=r"(tmp));
        tmp |= (BIT(index) & 0xfffffffc);
        __asm__ volatile ("mov ie0,%0" :: "r"(tmp));
    }
    else
    {
        index = index-32;
        __asm__ volatile ("mov %0,ie1" : "=r"(tmp));
        tmp |= BIT(index);
        __asm__ volatile ("mov ie1,%0" :: "r"(tmp));
    }
}

void DisableOSInt()
{
    int tmp;

    __asm__ volatile ("mov %0,ie0" : "=r"(tmp));
    tmp &= ~(BIT(0));
    __asm__ volatile ("mov ie0,%0" :: "r"(tmp));
}

void EnableOSInt()
{
    int tmp;
    __asm__ volatile ("mov %0,ie0" : "=r"(tmp));
    tmp |= (BIT(0));
    __asm__ volatile ("mov ie0,%0" :: "r"(tmp));
}

void trig_fun()
{
    __asm__ volatile ("trigger");
}

void default_isr_handler()
{
    printf("\ndefault_isr_handler\n");
}

/*
#include "os_api.h"
static OS_MUTEX crc_mutex;

static inline int vm_mutex_init(void)
{
    //return os_mutex_create(&crc_mutex);
}

static inline int vm_mutex_enter(void)
{
    //return os_mutex_pend(&crc_mutex,0);
}
static inline int vm_mutex_exit(void)
{
    //return os_mutex_post(&crc_mutex);
}*/


u16 CRC16(void *ptr, u32  len)
{
    u8 *p = (u8 *)ptr;
    u16 ret;

//  vm_mutex_enter();

    CRC_REG = 0 ;

    while(len--)
    {
        CRC_FIFO = *p++;
    }
    __asm__ volatile ("csync");
    ret = CRC_REG;

//  vm_mutex_exit();
    return CRC_REG;
}

#endif
