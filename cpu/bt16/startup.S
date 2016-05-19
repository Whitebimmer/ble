/*****************************************************************************************************************

(C) Copyright 2003 - Analog Devices, Inc.  All rights reserved.

File Name:		startup.asm

Date Modified:	2/26/03		LB		Rev 0.2

Software:       GNU DCC 4.1+

Hardware:		CM-BF533

Purpose:		Generic Startup file

************************************************************************************************/

#include "bt16.h"
#include "startup.h"
.extern _main;
.extern _htc_init;
.extern _bss_end
.extern _bss_begin

.extern _data_addr
.extern _data_begin
.extern _data_size

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




#define STACK_SIZE          0x800
#define INTR_STACK_SIZE     0x800

.section .stack, "aw", @nobits

	    .space  STACK_SIZE

_stack:

.section .intr_stack, "aw", @nobits

	    .space  INTR_STACK_SIZE

_intr_stack:

#define hi(A) (A>>16)
#define lo(A) (A&0xFFFF)
.section .text;
.org 0
_START:

// Initialize System Configuration Register
	r1 = SYSCFG_VALUE;
	SYSCFG = r1;

    B0 = 0;
    B1 = 0;
    B2 = 0;
    B3 = 0;

    L0 = 0;
    L1 = 0;
    L2 = 0;
    L3 = 0;

    I0 = 0 ;
    I1 = 0 ;
    I2 = 0 ;
    I3 = 0 ;


    SP.l = _stack;
    SP.h = _stack;
    r0.l = _intr_stack ;
    r0.h = _intr_stack;
    syscfg = r0 ;
    FP = SP;
	R1 = 0;


    P2 = 0;
    P3 = 0;
    P4 = 0;
    P5 = 0;

    M0 = 4;

/*
_clean_bss :

    P0.l= _bss_begin;
    P0.h= _bss_begin ;
    p1.l= _bss_end ;
    p1.h= _bss_end ;
    r0 = 0;

    _clean_bss1:
    cc = p0 < p1(IU);
    if !cc jump _jump_to_cmain ;

    [p0]=r0 ;
    P0 += 0x4;
    jump _clean_bss1 ;
*/

//////////////////////////////////////////////////
/**/
_clean_bss :
    P0.l= bss_begin;
    P0.h= bss_begin ;
    p1.l= bss_size + 4;
    p1.h= bss_size + 4;
    p1 = p1>>2;
    r0 = 0;

    LSETUP(.10, .10) LC0 = P1;
    .10:
    [p0++]=r0 ;
////////////////////////////////////////////////
/**/
_movedata :
    P0.l= data_begin;
    P0.h= data_begin ;
    p1.l= data_size + 4;
    p1.h= data_size + 4;
    p1 = p1>>2;
    p2.l= data_addr ;
    p2.h= data_addr ;

    CC = P1 == 0;
    IF CC JUMP _jump_to_cmain;

    LSETUP(.16, .17) LC0 = P1;
.16:
    r0 = [p0++] ;
.17:
    [p2++]= r0 ;


_jump_to_cmain:
    p1 = 0 ;
    p2 = 0 ;
    r0 = 0 ;
    p0.l = _main;
    p0.h = _main;
    call (p0);
    csync ;




.global _ISR_PUSH_REG

_ISR_PUSH_REG:
[--SP] = ASTAT;
[--SP] = reti;
[--SP] = (R7:0,P5:0);
[--sp] = FP;
[--SP] = I0;
[--SP] = I1;
[--SP] = I2;
[--SP] = I3;
[--SP] = B0;
[--SP] = B1;
[--SP] = B2;
[--SP] = B3;
[--SP] = L0;
[--SP] = L1;
[--SP] = L2;
[--SP] = L3;
[--SP] = M0;
[--SP] = M1;
[--SP] = M2;
[--SP] = M3;
R1.L = A0.X;
[--SP] = R1;
//R1 = A0.W;
[--SP] = A0.W;
R1.L = A1.X;
[--SP] = R1;
//R1 = A1.W;
[--SP] = A1.W;
[--SP] = LC0;
R3 = 0x0 (X);		/*		R3=0x0(  0) */
LC0 = R3;
[--SP] = LC1;
LC1 = R3;
[--SP] = LT0;
[--SP] = LT1;
[--SP] = LB0;
[--SP] = LB1;
L0 = 0x0 (X);		/*		L0=0x0 <_cnt.1320>(  0) */
L1 = 0x0 (X);		/*		L1=0x0 <_cnt.1320>(  0) */
L2 = 0x0 (X);		/*		L2=0x0 <_cnt.1320>(  0) */
L3 = 0x0 (X);		/*		L3=0x0 <_cnt.1320>(  0) */
RTS;


.global _ISR_POP_REG

_ISR_POP_REG:
LB1 = [SP++];
LB0 = [SP++];
LT1 = [SP++];
LT0 = [SP++];
LC1 = [SP++];
LC0 = [SP++];
R0 = [SP++];
A1 = R0;
R0 = [SP++];
A1.X = R0.L;
R0 = [SP++];
A0 = R0;
R0 = [SP++];
A0.X = R0.L;
M3 = [SP++];
M2 = [SP++];
M1 = [SP++];
M0 = [SP++];
L3 = [SP++];
L2 = [SP++];
L1 = [SP++];
L0 = [SP++];
B3 = [SP++];
B2 = [SP++];
B1 = [SP++];
B0 = [SP++];
I3 = [SP++];
I2 = [SP++];
I1 = [SP++];
I0 = [SP++];
FP = [SP++];
(R7:0,P5:0) = [SP++];
RETI = [SP++];
ASTAT = [SP++];
UNLINK ;
rti ;

.global ___udivsi3
___udivsi3:
divq(r0,r1) ;
RTS ;

.global ___divsi3
___divsi3:
divs(r0,r1) ;
RTS ;
