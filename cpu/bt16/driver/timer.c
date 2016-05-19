#include "typedef.h"
#include "HWI.h"
#include "init.h"

extern volatile u32 g_rand_seed ;

static void timer0_isr() 
{
    static u32 cnt1;

    TMR0_CON |= BIT(14);
    DSP_CLR_ILAT = BIT(TIME0_INIT);

    cnt1++;
	g_rand_seed++;

    if((cnt1 % 5) == 0)
    {
		sys_timer_schedule();
    }

    if (cnt1 == 250)
    {
       cnt1 = 0;
       puts(".");
    }
}
REG_INIT_HANDLE(timer0_isr);


static void timer0_irq_hannel_init(void)
{
	puts("timer0_irq_hannel_init\n");
    INTALL_HWI(TIME0_INIT, timer0_isr, 1) ; //timer0_isr
}
early_initcall(timer0_irq_hannel_init);


void timer0_start()
{
	puts("timer0_start\n");
#if 1     //LSB 60M   64分频
    TMR0_CNT = 0;

    TMR0_PRD = 937*2; //2ms

    TMR0_CON = BIT(0) | BIT(4)| BIT(5)| BIT(6);  // LSB 30M  64分频
#endif

#if 0     //LSB 30M   64分频
    TMR0_CNT = 0;

    //TMR0_PRD = 937/4; //500us
    //TMR0_PRD = 937/2; //1ms
    TMR0_PRD = 937; //2ms
    //TMR0_PRD = 937*2; //4ms
    //TMR0_PRD = 937*25; //50ms
    //TMR0_PRD = 65535; //139.8ms
    TMR0_CON = BIT(0) | BIT(4)| BIT(5);  // LSB 30M  64分频
#endif

#if 0     //LSB 30M   16分频
    TMR0_CNT = 0;

    //TMR0_PRD = 3750/5; //400us
    //TMR0_PRD = 3750/2; //1ms
    TMR0_PRD = 3750; //2ms
    //TMR0_PRD = 3750*10; //20ms
    //TMR0_PRD = 65535; //34.95ms

    TMR0_CON = BIT(0) |  BIT(5);  //
#endif

#if 0     //LSB 30M   4分频
    TMR0_CNT = 0;

    //TMR0_PRD = 15000/4; //500us
    TMR0_PRD = 15000; //2ms
    //TMR0_PRD = 65535; //8.7ms

    TMR0_CON = BIT(0) |  BIT(4);  //
#endif

#if 0     //LSB 30M   0分频
    TMR0_CNT = 0;

    //TMR0_PRD = 60000/4; //500us
    TMR0_PRD = 60000; //2ms
    //TMR0_PRD = 65535; //2.18ms

    TMR0_CON = BIT(0);  //
#endif



#if 0     //OSC 12M   64分频
    TMR0_CNT = 0;

    //TMR0_PRD = 375/4; //500us
    TMR0_PRD = 375; //2ms
    //TMR0_PRD = 65535; //349.5ms

    TMR0_CON = BIT(0) | BIT(3)| BIT(4)| BIT(5);
#endif

#if 0     //OSC 12M   16分频
    TMR0_CNT = 0;

    //TMR0_PRD = 1500/4; //500us
    TMR0_PRD = 1500; //2ms
    //TMR0_PRD = 1500*25; //50ms

    TMR0_CON = BIT(0) | BIT(3)| BIT(5);  //
#endif

#if 0     //OSC 12M   4分频
    TMR0_CNT = 0;

    //TMR0_PRD = 6000/4; //500us
    TMR0_PRD = 6000; //2ms
    //TMR0_PRD = 6000*10; //20ms

    TMR0_CON = BIT(0) | BIT(3)| BIT(4);  //
#endif

#if 0     //OSC 12M   0分频
    TMR0_CNT = 0;

    //TMR0_PRD = 24000/4; //500us
    TMR0_PRD = 24000; //2ms

    TMR0_CON = BIT(0) | BIT(3);  //
#endif

}

void timer1_init()
{
//    puts("timer1_init\r\n");

    TMR1_CNT = 0XFF<<8;
    TMR1_CON = BIT(0);// | BIT(8);
    TMR1_CON |= (7<<4);
}


void timer1_isr()
{
    TMR1_CON |= BIT(6);                    // timer2  interurrpt
    TMR1_CON  = 0;
    DSP_CLR_ILAT |= BIT(TIME1_INIT);        // cpu 中断号
    //printf("\nttttttt1");
    //clear_ie(TIME1_INIT);
}
REG_INIT_HANDLE(timer1_isr);

static void timer1_handler_init(void)
{
	puts("timer1_handler_init\n");
    INTALL_HWI(TIME1_INIT, timer1_isr, 0) ; //timer0_isr
}
/* early_initcall(timer1_handler_init); */
