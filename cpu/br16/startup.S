/***********************************Jieli tech************************************************
  File : startup.S
  By   : Juntham
  date : 2015-08-21 09:06
********************************************************************************************/
#include "br16.h"
#include "startup.h"

.extern _main;
.extern _htc_init;
.extern bss_end
.extern bss_begin

.extern data_addr
.extern data_begin
.extern data1_size

.macro INTTERRUPT_ENTRY int_name, int_number

.org (\int_number)*0x10 + 0
[--sp] = p0 ;
[--sp] =rets ;
p0.l= \int_name ;
p0.h= \int_name ;
call (p0) ;

rets = [sp++]
p0=[sp++] ;
RTI  ;
.endm
.section .stack, "a"
         .space 0x800
         .global _stack
_stack:
         .space 0x1000
         .global _ustack
_ustack:
#define hi(x) x>>16
#define lo(x) x&0xffff
.section .text
    .align 2
    .global _start
    .extern memset
    .extern pll_init
    .extern bt_power_is_poweroff_probe
    .type _start,@function
    .org 0

.extern main
_start:
    mov  r7,r0
    mov  r5,r1
    mov  r6,r2
    movh ssp, _stack
    movl ssp, _stack
    movh sp, _ustack
    movl sp, _ustack
    /* call pll_init */
/**/
    call bt_power_is_poweroff_probe
    cmp  r0 ,0
    jne .L7

    movh  r0, ram1_addr
    movl  r0, ram1_addr
    movh  r1, ram1_begin
    movl  r1, ram1_begin
    movh  r2, ram1_size
    movl  r2, ram1_size

    jz    r2, .L7
.L5:
    cmp r2, 32
    ifcs 2, 2
    movs r4, 0
    sub r2, r2, 32
    mov r4, r2
    movs r2, 0
    rep .L6, r4
    lbzadd r3, r1, 1
    sbadd r3, r0, 1
.L6:
    jnz  r2, .L5
.L7:

    /* clean bss */  
    movh  r0, bss_begin
    movl  r0, bss_begin
    mov   r1,0
    movh  r2, bss_size
    movl  r2, bss_size

    jz    r2, .L_2
.L_1:
    cmp r2, 32
    ifcs 2, 2
    movs r3, 0
    sub r2, r2, 32
    mov r3, r2
    movs r2, 0
    rep .L_01, r3
    sbadd r1, r0, 1
.L_01:
    jnz   r2, .L_1
.L_2:

    /* move data  */  
    movh  r0, data_addr
    movl  r0, data_addr
    movh  r1, data_begin
    movl  r1, data_begin
    movh  r2, data1_size
    movl  r2, data1_size

    jz    r2, .L4
.L2:
    cmp r2, 32
    ifcs 2, 2
    movs r4, 0
    sub r2, r2, 32
    mov r4, r2
    movs r2, 0
    rep .L3, r4
    lbzadd r3, r1, 1
    sbadd r3, r0, 1
.L3:
    jnz  r2, .L2

.L4:
    mov  r0,r7
    mov  r1,r5
    mov  r2,r6
//loop:
//    j  loop
    j  main

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
