/***********************************Jieli tech************************************************
  File : cpu.S
  By   : Juntham
  date : 2014-07-03 09:10
********************************************************************************************/

.section .bss, "aw"
IntCounter:
        .space 0x4

#define  UPPER_( x )   (((x) >> 16) & 0x0000FFFF)
#define  LOWER_( x )   ((x) & 0x0000FFFF)
#define  LOAD(x, y)    x##.h = UPPER_(y); x##.l = LOWER_(y)
#define  LOADA(x, y)   x##.h = y; x##.l = y
//------------------------------------------------------------------
//    Global variables
//------------------------------------------------------------------
/* #define  CPU_INT_DIS()  __builtin_pi32_cli(); */
/* #define  CPU_INT_EN()  __builtin_pi32_sti(); */


.section .text;
    .type CPU_INT_DIS,@function
.global CPU_INT_DIS
CPU_INT_DIS:
    cli
    movl r0, IntCounter
    movh r0, IntCounter
    lw   r1, r0, 0
    add  r1, r1, 1
    sw   r1, r0, 0
    rts

    .type CPU_INT_EN,@function
.global CPU_INT_EN
CPU_INT_EN:
    movl r0, IntCounter
    movh r0, IntCounter
    lw   r1, r0, 0
    add  r1, r1, -1
    sw   r1, r0, 0
    ifeq 1, 0
    sti
    rts

