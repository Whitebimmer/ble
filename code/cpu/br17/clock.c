#include "cpu.h"
#include "RF.h"

#define PLLNR 0
#define PLLNF 19

#define BTOSC_CLK   24 

void osc_clk_init(void)
{
    /* CLK_CON0 |= BIT(0); */

    /* CLK_CON0 &= ~BIT(8); */
    //bt osc en
    SFR(WLA_CON17,10 ,4 ,0x8);    //osc HCS
    SFR(WLA_CON17,0  ,5 ,0x10);   //osc CLS
    SFR(WLA_CON17,5  ,5 ,0x10);   //osc CRS
    SFR(WLA_CON14,13 ,1 ,0x0);    //osc bt oe
    SFR(WLA_CON14,14 ,1 ,0x1);    //osc fm oe
    SFR(WLA_CON17,14 ,2 ,0x2);    //osc LDO level
    SFR(WLA_CON14,11 ,1 ,0x1);    //osc ldo en
    SFR(WLA_CON14,12 ,1 ,0x0);    //osc test
    SFR(WLA_CON18, 2 ,2 ,0x2);    //osc xhd current
    SFR(WLA_CON14,10 ,1 ,0x1);    //osc en ; no connect should close    

    SFR(CLK_CON0, 6, 2, 0x0);

    /* CLK_CON0 |= BIT(8);             //rc_clk switch to osc_clk */
}

void clk_out(void)
{
   // SFR(PORTA_DIR,  4,  1,  0);
   // SFR(CLK_CON0, 10,  3,  2); // 0:PA4; 1:LSB_CLK; 2:BTOSC; 3:RTOSC; 4:IRC_CLK; 5:HSB_CLK; 6:HTC_CLK; 7:PLL_CLK

    SFR(PORTA_DIR,  5,  1,  0);
    SFR(CLK_CON0, 13,  3,  4); // 0:PA5; 1:FM_LO/2; 2:PLL_RFI; 3:192M; 4:BT_LO_32; 5:PLL_MDM; 6:apc_clk; 7:BT_RCCL_CLK
}


void pll_init(void)
{
    SFR(CLK_CON0,  2,  2,  0); // clk_mux0  sel rc
    SFR(CLK_CON0,  0,  1,  1); //  rcen
    delay(100);
    SFR(CLK_CON0,  8,  1,  0); // SYS_CLK: 0:RC_CLK; 1:MUX_CLK
    /* osc_clk_init(); */

    SFR(PLL_CON,  8,  2,  0);  // PLL_ref_sel: 0:PLL_OSC_CLK  ; 1: BTOSC_CLK
    SFR(PLL_CON,  2,  5,  (BTOSC_CLK/2-2));  // PLL_REFDS
    SFR(PLL_CON,  7,  1,  1);  // PLL_REFDSEN: PLL_SEL12M = 0: PLL_ref/1 ; 1: PLL_ref/(2+PLL_REFDS)
    SFR(PLL_CON, 16,  1,  0);  // PLL_SEL12M:  0: 2M ; 1: 12M
    SFR(PLL_CON, 10,  1,  0);  // PLL_SEL12M:  0: 2M ; 1: 12M
    SFR(PLL_CON,  1,  1,  0);  // PLL_RN
    SFR(PLL_CON,  0,  1,  1);  // PLL_EN;
    delay(100);
    SFR(PLL_CON,  1,  1,  1);  // PLL_RN
    delay(500);

    SFR(CLK_CON2, 31,  1,  0); // PLL_192_SEL: 0:PLL_192  1:PLL_480/2.5

    SFR(CLK_CON2,  0,  2,  0); // PLL_SYS_CLK:  0:PLL_192M; 1:PLL_480M; 2:FM_PLL; 3:0
    SFR(CLK_CON2,  2,  2,  0); // PLL_SYS_DIV1: 0:1/1; 1:1/3; 2:1/5; 3:/7
    SFR(CLK_CON2,  4,  2,  2); // PLL_SYS_DIV2: 0:1/1; 1:1/2; 2:1/4; 3:/8

    SFR(CLK_CON2, 12,  2,  1); // PLL_FM_RFI_SEL:  0:PLL_192M; 1:PLL_480M; 2:FM_PLL; 3:0
    SFR(CLK_CON2, 14,  2,  1); // PLL_FM_RFI_DIV1: 0:1/1; 1:1/3; 2:1/5; 3:/7
    SFR(CLK_CON2, 16,  2,  1); // PLL_FM_RFI_DIV2: 0:1/1; 1:1/2; 2:1/4; 3:/8

    SFR(CLK_CON2, 18,  2,  0); // PLL_APC_SEL:   0:PLL_192M; 1:PLL_480M; 2:FM_PLL; 3:0
    SFR(CLK_CON2, 20,  2,  0); // PLL_APC_DIV1:  0:1/1; 1:1/3; 2:1/5; 3:/7
    SFR(CLK_CON2, 22,  2,  1); // PLL_APC_DIV2:  0:1/1; 1:1/2; 2:1/4; 3:/8
    SFR(CLK_CON2, 30,  2,  0); // PLL_ALNK_SEL: 0:12.288M; 1:11.2896M

    SFR(CLK_CON0,  6,  2,  3); // SRC_MUX_CLK1:  0:PLL_SYS_CLK; 1:htc; 2:btosc; 3:rtOSC_CLK
    delay(100);
    SFR(CLK_CON0,  8,  1,  1); // SYS_CLK: 0:MUX_CLK0; 1:MUX_CLK1
    SFR(SYS_DIV,  0,  8,  0);  // SYS_CLK = SYS_CLK/(1+n);
    SFR(SYS_DIV,  8,  3,  0);  // LSB_CLK = SYS_CLK/(1+n);
    SFR(SYS_DIV,  11, 1,  0);  // FM_CLK = SYS_CLK/(1+n);

    /* clk_out(); */
   // while(1);
}
