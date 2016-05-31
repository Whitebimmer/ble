#include "typedef.h"


void pll_init(void)
{
    CLK_CON0 &= ~BIT(8);
    delay(100);
    /* PLL_CON &= ~(0x1f << 2); */
    /* PLL_CON |= (24000/2000-2)<<2; */
    PLL_CON |= BIT(0);//EN
    //wait 100 us
    delay(10);
    CLK_CON2 &= ~(0x3f);    //pll_192M
    CLK_CON2 |= BIT(5);     //pll div4 sys_clk = 192/2=96M

    CLK_CON0 &= ~(3 << 6);  //clk mux1 pll
    CLK_CON0 |= (3 << 6);  //clk mux1 pll
    CLK_CON0 |= BIT(8);     //sys switch pll clk 48M

    /* debug_init(48000000 / 115200); */
}

