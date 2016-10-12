#include "cpu.h"
#include "RF.h"

#define PLLNR 0
#define PLLNF 19

#define BTOSC_CLK   24 

void osc_clk_init(void)
{
    SFR(CLK_CON1,  14, 2,  2); //BT_CLK SEL SYS
    SFR(RFI_CON,   1,  2,  3); // OPEN BT_EN
    SFR(BTA_CON13, 1,  4,  0); // BTOSC_CLSEL
    SFR(BTA_CON13, 5,  4,  0); // BTOSC_CRSEL
    if(BTOSC_CLK> 12)
    {
      SFR(BTA_CON13, 10, 1,  1); // BTOSC_HEN
    }
    else
    {
      SFR(BTA_CON13, 10, 1,  0); // BTOSC_HEN
    }

    SFR(BTA_CON13, 0,  1,  0); // BTOSC_AACEN

    SFR(BTA_CON14, 11, 1,  0); // BTOSC_EN_SEL_BASEBAND
    SFR(BTA_CON13, 9,  1,  1); // BTOSC_EN
    SFR(BTA_CON13, 11, 1,  1); // BTOSC_BT_OE
    SFR(BTA_CON13, 12, 1,  1); // BTOSC_FM_OE
    SFR(BTA_CON13, 13, 1,  1); // BTOSC_SYS_OE

    SFR(CLK_CON1,  14, 2,  3); //BT_CLK SEL SYS
//    SFR(CLK_CON0,  2,  1,  0); // XOSC_HEN
//    SFR(CLK_CON0,  3,  1,  0); // XOSC_32K
//    SFR(CLK_CON0,  1,  1,  1); // XOSC_EN

    SFR(CLK_CON0,  4,  2,  1); // OSC_SEL: 0 XOSC_CLK ; 1BTOSC_CLK

	SFR(CLK_CON1, 14, 2, 0);
}

void clk_out(void)
{
    SFR(PORTA_DIR,  4,  1,  0);
    SFR(CLK_CON0, 10,  3,  7); // 0:PA4; 1:XOSC_CLK; 2:BT_OSC; 3:RTC_32K; 4:RC_CLK; 5:RC_CLK; 6:PLL_CLK; 7:SYS_CLK

    SFR(PORTA_DIR,  5,  1,  0);
    SFR(CLK_CON0, 13,  3,  5); // 0:PA5; 1:FM_LO/2; 2:PLL_RFI; 3:PLL_FM; 4:BT_LO_32; 5:PLL_MDM; 6:FM_TEST; 7:PLL_24
}


void bt16_pll_init(void)
{
    SFR(CLK_CON0,  0,  1,  1); // RC_EN
    delay(100);
    SFR(CLK_CON0,  8,  1,  0); // SYS_CLK: 0:RC_CLK; 1:MUX_CLK

    osc_clk_init();
    SFR(PLL_CON, 17,  2,  2);  // PLL_osc_sel: 0 XOSC_CLK ; 1 BTOSC_CLK ; 2,3 RC_CLK
    SFR(PLL_CON,  8,  1,  1);  // PLL_ref_sel: 0:PLL_OSC_CLK  ; 1: BTOSC_CLK
    SFR(PLL_CON,  2,  5,  (BTOSC_CLK/2-2));  // PLL_REFDS
    SFR(PLL_CON,  7,  1,  1);  // PLL_REFDSEN: PLL_SEL12M = 0: PLL_ref/1 ; 1: PLL_ref/(2+PLL_REFDS)
    SFR(PLL_CON, 16,  1,  0);  // PLL_SEL12M:  0: 2M ; 1: 12M
    SFR(PLL_CON,  1,  1,  0);  // PLL_RN
    SFR(PLL_CON,  0,  1,  1);  // PLL_EN;
    delay(100);
    SFR(PLL_CON,  1,  1,  1);  // PLL_RN
    delay(500);

    SFR(CLK_CON2,  0,  2,  0); // PLL_SYS_CLK:  0:PLL_192M; 1:PLL_480M; 2:FM_PLL; 3:0
    SFR(CLK_CON2,  2,  2,  0); // PLL_SYS_DIV1: 0:1/1; 1:1/3; 2:1/5; 3:/7
    SFR(CLK_CON2,  4,  2,  1); // PLL_SYS_DIV2: 0:1/1; 1:1/2; 2:1/4; 3:/8

    SFR(CLK_CON2,  6,  2,  0); // PLL_FM_DSP_SEL:  0:PLL_192M; 1:PLL_480M; 2:FM_PLL; 3:0
    SFR(CLK_CON2,  8,  2,  0); // PLL_FM_DSP_DIV1: 0:1/1; 1:1/3; 2:1/5; 3:/7
    SFR(CLK_CON2, 10,  2,  1); // PLL_FM_DSP_DIV2: 0:1/1; 1:1/2; 2:1/4; 3:/8
    SFR(CLK_CON2, 12,  2,  1); // PLL_FM_RFI_SEL:  0:PLL_192M; 1:PLL_480M; 2:FM_PLL; 3:0
    SFR(CLK_CON2, 14,  2,  1); // PLL_FM_RFI_DIV1: 0:1/1; 1:1/3; 2:1/5; 3:/7
    SFR(CLK_CON2, 16,  2,  1); // PLL_FM_RFI_DIV2: 0:1/1; 1:1/2; 2:1/4; 3:/8

    SFR(CLK_CON2, 18,  2,  0); // PLL_APC_SEL:   0:PLL_192M; 1:PLL_480M; 2:FM_PLL; 3:0
    SFR(CLK_CON2, 20,  2,  0); // PLL_APC_DIV1:  0:1/1; 1:1/3; 2:1/5; 3:/7
    SFR(CLK_CON2, 22,  2,  1); // PLL_APC_DIV2:  0:1/1; 1:1/2; 2:1/4; 3:/8
    SFR(CLK_CON2, 24,  2,  0); // PLL_ALNK_SEL: 0:12.288M; 1:11.2896M

    SFR(CLK_CON0,  6,  2,  3); // SRC_MUX_CLK:  0:OSC_CLK; 1:RTC_32K; 2:RC_CLK; 3:PLL_SYS_CLK
    delay(100);
    SFR(CLK_CON0,  8,  1,  1); // SYS_CLK: 0:RC_CLK; 1:MUX_CLK
    SFR(SYS_DIV,  8,  2,  0);  // LSB_CLK = SYS_CLK/(1+n); /4


//    clk_out();
//
//    while(1);
}
