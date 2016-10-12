#include "typedef.h"

void delay(unsigned long  t)
{
    while ( t--)
    {
        asm("nop");
    }
}



