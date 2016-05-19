/*************************************************************
File: typedef.h
Author:Juntham
Discriptor:
    数据类型重定义
Version:
Date：
*************************************************************/
#ifndef _typedef_h_
#define _typedef_h_


#include "asm_type.h"

#ifdef __BIG_ENDIAN
#define __BYTE_ORDER  __BIG_ENDIAN
#elif defined __LITTLE_ENDIAN
#define __BYTE_ORDER  __LITTLE_ENDIAN
#else
#error "cpu endian is undefined"
#endif


#define GCC_VERSION (__GNUC__ * 10000 \
                               + __GNUC_MINOR__ * 100 \
                               + __GNUC_PATCHLEVEL__)


/*#define	LD_WORD(ptr)		(u16)(*(u16*)(u8*)(ptr))
#define	LD_DWORD(ptr)		(u32)(*(u32*)(u8*)(ptr))
#define	ST_WORD(ptr,val)	*(u16*)(u8*)(ptr)=(u16)(val)
#define	ST_DWORD(ptr,val)	*(u32*)(u8*)(ptr)=(u32)(val)*/

#ifdef __CPU_BIG_ENDIAN
#define ntohl(x) x
#define ntoh(x) x
#endif //Big Endian

#ifdef __CPU_LITTLE_ENDIAN
#define ntohl(x) (u32)((x>>24)|((x>>8)&0xff00)|(x<<24)|((x&0xff00)<<8))
#define ntoh(x) (u16)((x>>8&0x00ff)|x<<8&0xff00)
#endif  //Little Endian



#define     BIT(n)	            (1L << (n))

#define     clr_bit(x,y)         x &= ~(1L << y)
#define     set_bit(x,y)         x |= (1L << y)

#define      TRUE        1
#define      FALSE       0

#define      true        1
#define      false       0

#ifdef __GNUC__
#define sec(x) __attribute__((section(#x)))
#define AT(x) __attribute__((section(#x)))
#define SET(x) __attribute__((x))
#define _GNU_PACKED_	__attribute__((packed))
#else
#define sec(x)
#define AT(x)
#define SET(x)
#define _GNU_PACKED_
#endif

#ifndef NULL
#define NULL    ((void *)0)
#endif


#define ASSERT(a,...)   \
		do { \
			if(!(a)){ \
				printf("ASSERT-FAILD: "#a"\n"__VA_ARGS__); \
				while(1); \
			} \
		}while(0);


#define _static_  static

#define LITTLE_ENDIAN
#define _banked_func
#define _xdata
#define _data
#define _root
#define _no_init
#define my_memset memset
#define my_memcpy memcpy

#define readb(addr)   *((unsigned char*)(addr))
#define readw(addr)   *((unsigned short *)(addr))
#define readl(addr)   *((unsigned long*)(addr))

#define writeb(addr, val)  *((unsigned char*)(addr)) = (val)
#define writew(addr, val)  *((unsigned short *)(addr)) = (u16)(val)
#define writel(addr, val)  *((unsigned long*)(addr)) = (val)

#define ARRAY_SIZE(array)  (sizeof(array)/sizeof(array[0]))
#define ARRAY_AND_SIZE(array)  array, (sizeof(array)/sizeof(array[0]))

#define CPU_BIG_EADIN

#if defined(CPU_BIG_EADIN)
#define __cpu_u16(lo, hi)  ((lo)|((hi)<<8))

#elif defined(CPU_LITTLE_EADIN)
#define __cpu_u16(lo, hi)  ((hi)|((lo)<<8))
#else
#error "undefine cpu eadin"
#endif



#include "cpu.h"




#endif
