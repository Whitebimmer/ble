#include "cpu.h"
#include "RF.h"

#define PLLNR 0
#define PLLNF 19

#define BTOSC_CLK   12 

void osc_clk_init(void)
{
    SFR(CLK_CON1, 14, 2, 2);         // OPEN BT_CLK
    delay(10);
    RFI_CON |= (1<<1);               // BT_EN
    delay(10);
    SFR(BTA_CON13, 5,  5,  0); // BTOSC_CLSEL
    SFR(BTA_CON13, 10, 5,  0); // BTOSC_CRSEL

    if(BTOSC_CLK> 12)
    {
      SFR(BTA_CON14, 1, 4,  4); // BTOSC_Hcs
    }
    else
    {
      SFR(BTA_CON14, 1, 4,  2); // BTOSC_Hcs
    }

    SFR(BTA_CON15, 11, 1,  0); // BTOSC_EN_SEL_BASEBAND
    SFR(BTA_CON13, 15, 1,  1); // BTOSC_EN
    SFR(BTA_CON14, 5, 1,  1); // BTOSC_BT_OE
    SFR(BTA_CON14, 6, 1,  1); // BTOSC_FM_OE
    SFR(BTA_CON14, 7, 1,  1); // BTOSC_SYS_OE

    SFR(CLK_CON1,  14, 2,  3); //BT_CLK SEL close
    SFR(CLK_CON0,  4,  2,  0); // OSC_SEL: 0 btOSC_CLK ; 1rtOSC_CLK
}

void clk_out(void)
{
   // SFR(PORTA_DIR,  4,  1,  0);
   // SFR(CLK_CON0, 10,  3,  2); // 0:PA4; 1:LSB_CLK; 2:BTOSC; 3:RTOSC; 4:IRC_CLK; 5:HSB_CLK; 6:HTC_CLK; 7:PLL_CLK

    SFR(PORTA_DIR,  5,  1,  0);
    SFR(CLK_CON0, 13,  3,  4); // 0:PA5; 1:FM_LO/2; 2:PLL_RFI; 3:192M; 4:BT_LO_32; 5:PLL_MDM; 6:apc_clk; 7:BT_RCCL_CLK
}


void br16_pll_init(void)
{
    CLK_CON1 &= ~(1<<26);       // CLOSE PIN RESET
    CRC_REG = 0X6EA5;
    WDT_CON = 0;                // CLOSE WDT
    CRC_REG = 0X0;

    SFR(CLK_CON0,  2,  2,  0); // clk_mux0  sel rc
    SFR(CLK_CON0,  0,  1,  1); //  rcen
    delay(100);
    SFR(CLK_CON0,  8,  1,  0); // SYS_CLK: 0:RC_CLK; 1:MUX_CLK
    osc_clk_init();

    SFR(PLL_CON, 17,  2,  0);  // PLL_osc_sel: 0 BTOSC ; 1 RTOSCC ; 2,HTC_cLK 3 PAT_CLK
    SFR(PLL_CON,  8,  1,  1);  // PLL_ref_sel: 0:PLL_OSC_CLK  ; 1: BTOSC_CLK
    SFR(PLL_CON,  2,  5,  (BTOSC_CLK/2-2));  // PLL_REFDS
    SFR(PLL_CON,  7,  1,  1);  // PLL_REFDSEN: PLL_SEL12M = 0: PLL_ref/1 ; 1: PLL_ref/(2+PLL_REFDS)
    SFR(PLL_CON, 16,  1,  0);  // PLL_SEL12M:  0: 2M ; 1: 12M
    SFR(PLL_CON,  1,  1,  0);  // PLL_RN
    SFR(PLL_CON,  0,  1,  1);  // PLL_EN;
    delay(100);
    SFR(PLL_CON,  1,  1,  1);  // PLL_RN
    delay(500);

    SFR(CLK_CON2, 31,  1,  0); // PLL_192_SEL: 0:PLL_192  1:PLL_480/2.5

    SFR(CLK_CON2,  0,  2,  0); // PLL_SYS_CLK:  0:PLL_192M; 1:PLL_480M; 2:FM_PLL; 3:0
    SFR(CLK_CON2,  2,  2,  1); // PLL_SYS_DIV1: 0:1/1; 1:1/3; 2:1/5; 3:/7
    SFR(CLK_CON2,  4,  2,  0); // PLL_SYS_DIV2: 0:1/1; 1:1/2; 2:1/4; 3:/8

    SFR(CLK_CON2, 12,  2,  1); // PLL_FM_RFI_SEL:  0:PLL_192M; 1:PLL_480M; 2:FM_PLL; 3:0
    SFR(CLK_CON2, 14,  2,  1); // PLL_FM_RFI_DIV1: 0:1/1; 1:1/3; 2:1/5; 3:/7
    SFR(CLK_CON2, 16,  2,  1); // PLL_FM_RFI_DIV2: 0:1/1; 1:1/2; 2:1/4; 3:/8

    SFR(CLK_CON2, 18,  2,  0); // PLL_APC_SEL:   0:PLL_192M; 1:PLL_480M; 2:FM_PLL; 3:0
    SFR(CLK_CON2, 20,  2,  0); // PLL_APC_DIV1:  0:1/1; 1:1/3; 2:1/5; 3:/7
    SFR(CLK_CON2, 22,  2,  1); // PLL_APC_DIV2:  0:1/1; 1:1/2; 2:1/4; 3:/8
    SFR(CLK_CON2, 30,  2,  0); // PLL_ALNK_SEL: 0:12.288M; 1:11.2896M

    SFR(CLK_CON0,  6,  2,  0); // SRC_MUX_CLK1:  0:PLL_SYS_CLK; 1:htc; 2:btosc; 3:rtOSC_CLK
    delay(100);
    SFR(CLK_CON0,  8,  1,  1); // SYS_CLK: 0:MUX_CLK0; 1:MUX_CLK1
    SFR(SYS_DIV,  0,  8,  0);  // SYS_CLK = SYS_CLK/(1+n);
    SFR(SYS_DIV,  8,  3,  0);  // LSB_CLK = SYS_CLK/(1+n);
    SFR(SYS_DIV,  11, 1,  0);  // FM_CLK = SYS_CLK/(1+n);

    /* clk_out(); */
   // while(1);
}
