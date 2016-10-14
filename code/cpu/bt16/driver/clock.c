#include "typedef.h"

#define PLLNR 0
#define PLLNF 19

void pll_init(void)
{

    PLL_CON |= BIT(0);        //PLL enable
    delay(10);                                  //wait PLL stable
    CLK_CON0 |= BIT(7) | BIT(6);
    delay(100);
    CLK_CON0 |= BIT(8);                        //system clock select to PLL clock

}

