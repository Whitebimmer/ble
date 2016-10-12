/***********************************Jieli tech************************************************
  File : cpu.h
  By   : Juntham
  date : 2014-10-30 16:05
********************************************************************************************/
#ifndef _CPU_H_
#define _CPU_H_
#include "bt16.h"

#ifndef __ASSEMBLY__
#include "asm_type.h"
#endif

#define LITTLE_ENDIAN
#define  OS_STK_GROWTH        1                             /* Stack grows from HIGH to LOW memory on 80x86  */

#define OS_CPU_CORE               1
#define OS_CPU_MMU                1
#define SDRAM                     0
#define MEM_MALLOC                1

#if SDRAM > 0
#define PAGE_SHIFT 14
#define SDRAM_BEGIN_ADDR        (0x800000)
#define MAX_SDRAM_SIZE          (4*1024*1024)
#define MMU_MAX_SIZE            (4*1024*1024)  // mmu 管理的最大地址空间
#define MMU_ADDR_BEGIN          (0x400000)
#define MMU_ADDR_END            (0x800000)
#define STACK_TASK_SIZE         (1024*64)
#define STACK_START             MMU_ADDR_BEGIN
#define STACK_END               (MMU_ADDR_BEGIN+STACK_TASK_SIZE)
#else
#define PAGE_SHIFT 9
//#define SDRAM_BEGIN_ADDR        (0x0)
#define MAX_SDRAM_SIZE          (128*1024-PAGE_SIZE)
#define MMU_MAX_SIZE            (128*1024-PAGE_SIZE)  // mmu 管理的最大地址空间
#define MMU_ADDR_BEGIN          (0x80000)
#define MMU_ADDR_END            (MMU_ADDR_BEGIN+MMU_MAX_SIZE)
#define STACK_TASK_SIZE         (1024*4)
#define STACK_START             MMU_ADDR_BEGIN
#define STACK_END               (MMU_ADDR_BEGIN+STACK_TASK_SIZE)
#endif
#define PAGE_SIZE   (1UL << PAGE_SHIFT)
#define PAGE_MASK   (~(PAGE_SIZE-1))
#define PAGE_ALIGN(addr)    (((addr)+PAGE_SIZE-1)&PAGE_MASK)

#define MMU_TAG_ADDR0   0x49800
#define APP_BEGIN_ADDR  MMU_ADDR_BEGIN

#if OS_CPU_CORE > 1
#define MMU_TAG_ADDR1   0x6a000
#endif

#if (MAX_SDRAM_SIZE/PAGE_SIZE  > 256 )
#define ufat_t u16
#else
#define ufat_t u8
#endif

#ifndef __ASSEMBLY__
/*typedef unsigned char   u8, bool, BOOL;
typedef char            s8;
typedef unsigned short  u16;
typedef signed short    s16;
typedef unsigned int    u32 , tu8, tu16 , tbool , tu32;
typedef signed int      s32;*/

inline static int processor_id()
{
    int cpu_index ;
    asm("%0 = SEQSTAT ;" : "=r" (cpu_index));
    return cpu_index ;
}

#if OS_CPU_CORE > 1
#define OS_CPU_ID                 processor_id()
#else
#define OS_CPU_ID                 0
#endif

#endif
/*
*********************************************************************************************************
*                                              Defines
*********************************************************************************************************
*/

#define  CPU_SR_ALLOC()  unsigned int cpu_sr

/*save and Disable interrupts */
#define  CPU_INT_DIS()    \
	asm volatile ("cli %0" : "=d" (cpu_sr))

/* pop interrupts values */
#define  CPU_INT_EN()     \
	asm volatile ("sti %0": : "d" (cpu_sr))


#ifndef __ASSEMBLY__
//#include "irq.h"
#endif

#define  CPU_LOCK_EN()    asm volatile (".word 0x26")
#define  CPU_LOCK_DIS()   asm volatile (".word 0x27")

#define  OS_TASK_CLR(a)   DSP_CLR_ILAT = (1L << (31-a))
#define  OS_TASK_SW(a)    DSP_SET_ILAT = (1L << (31-a))       /* 任务级任务切换函数*/

#if OS_CPU_CORE > 1
#define OS_ENTER_CRITICAL()  CPU_INT_DIS();CPU_LOCK_EN()            /* 关中断*/
#define OS_EXIT_CRITICAL()   CPU_LOCK_DIS();CPU_INT_EN()             /* 开中断*/
#else
#define OS_ENTER_CRITICAL()  \
	CPU_INT_DIS()
#if 0
	do { \
		int t; \
		__global_irq_disable(); \
		CPU_INT_DIS(); \
		t = cpu_sr & BIT(25);  \
		asm volatile ("sti %0": : "d" (t)); \
	}while(0)
#endif

#define OS_EXIT_CRITICAL()  \
		CPU_INT_EN(); \

#if 0
	do{ \
		CPU_INT_EN(); \
		__global_irq_enable(); \
	}while(0)
#endif

#endif




#define MULU(Rm,Rn) asm volatile (".word 0xc512;divq(%0, %1);" :: "d" (Rm), "d" (Rn) );
#define MUL(Rm,Rn)  asm volatile (".word 0xc512;divs(%0, %1);" :: "d" (Rm), "d" (Rn) );
#define MAC(Rm,Rn)  asm volatile (".word 0xc512;%0*=%1;" ::"d" (Rm), "d" (Rn) );
#define MAS(Rm,Rn)  asm volatile (".word 0xc512;%0=~%1;" :: "d"(Rm), "d" (Rn) );
#define MRSIS(Rm,Rn) asm volatile (".word 0xc512;%0+=%1;" : "=&d" (Rm): "i" (Rn) );  //mac read shift imm&sat
#define MRSI(Rm,Rn) asm volatile (".word 0xc512;%0=%1;" : "=&d" (Rm): "i" (Rn) );    //mac read shift imm
#define MRSRS(Rm,Rn) asm volatile (".word 0xc512;%0>>>=%1;" : "=&d" (Rm): "d" (Rn) );  //mac read shift reg&sat
#define MRSR(Rm,Rn) asm volatile (".word 0xc512;%0>>=%1;" : "=&d" (Rm): "d" (Rn) );    //mac read shift register
#define MUL32(Ro,Rm,Rn) asm volatile (".word 0xc512;%0=%1&%2;": "=&d" (Ro):"d" (Rm), "d" (Rn) );

#define MACCLR()  asm volatile (".word 0xc512;divs(r0, r0);.word 0xc512;r0=~r0");
#define MACSET(h,l) asm volatile ("SEQSTAT = %0; USP = %1"::"d" (l), "d" (h));
#define MACRL(l) asm volatile ("%0 = SEQSTAT":"=d" (l));
#define MACRH(h) asm volatile ("%0 = USP":"=d" (h));

#define MULSI(Ro,Rm,Rn,Rp)  asm volatile (".word 0xc512;divs(%1, %2);.word 0xc512;%0=%3;" : "=&d" (Ro) : "d"(Rm), "d" (Rn), "i" (Rp) );
#define MULSIS(Ro,Rm,Rn,Rp) asm volatile (".word 0xc512;divs(%1, %2);.word 0xc512;%0+=%3;" : "=&d" (Ro) : "d"(Rm), "d" (Rn), "i" (Rp) );
#define MULSR(Ro,Rm,Rn,Rp)  asm volatile (".word 0xc512;divs(%1, %2);.word 0xc512;%0>>=%3;" : "=&d" (Ro) : "d"(Rm), "d" (Rn), "d" (Rp) );
#define MULSRS(Ro,Rm,Rn,Rp) asm volatile (".word 0xc512;divs(%1, %2);.word 0xc512;%0>>>=%3;" : "=&d" (Ro) : "d"(Rm), "d" (Rn), "d" (Rp) );



/* void EnableOtherCpu(void); */
/* #define enable_other_cpu EnableOtherCpu */


#endif
