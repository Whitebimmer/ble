#ifndef ASM_TYPE_H
#define ASM_TYPE_H


#define __LITTLE_ENDIAN   0x0123

typedef char			s8;
typedef unsigned char	u8;
typedef short			s16;
typedef unsigned short	u16;
typedef unsigned int	u32, tu8, tu16, tu32, tbool, bool;
typedef int				s32;
typedef unsigned long long u64;
typedef unsigned char   bit1;

static inline void __delay(u32 t)
{
    while (t--) {
        asm("nop");
    }
}
















#endif

