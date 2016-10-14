#include "typedef.h"
/* #include "RF_bredr.h" */
/* #include "interrupt_isr.h" */
#include "stdarg.h"
#include "RF.h"
#include "bt_power_driver.h"
#include "bt_power.h"

#define PD_DEBUG_EN

#ifdef PD_DEBUG_EN
#define pd_putchar(x)        putchar(x)
#define pd_puts(x)           puts(x)
#define pd_u32hex(x)         put_u32hex(x)
#define pd_u8hex(x)          put_u8hex(x)
#define pd_buf(x,y)          printf_buf(x,y)
#define pd_printf            printf
#else
#define pd_putchar(...)
#define pd_puts(...)
#define pd_u32hex(...)
#define pd_u8hex(...)
#define pd_buf(...)
#define pd_printf(...)
#endif


#define Tst0 0x1
#define Tst1 0x1
#define Tst2 0x1
#define Tst4 0x1
#define Tst5 0x1
#define Tst6 0x1

/* struct power_para{ */
    /* u32 Tall; */
    /* u32 local_clock_slot; */
    /* u32 finetimecnt; */
    /* u32 phcom_cnt; */
    /* u8 mode; */
    /* u8 poweroff; */
    /* u8 osc; */
    /* u8 use_PR1; */
    /* u32 osc_hz; */
    /* u32 Tosc; */
    /* u8 kstb0; */
    /* u8 kstb1; */
    /* u8 kstb2; */
    /* u8 kstb3; */
    /* u8 kstb4; */
    /* u8 kstb5; */
    /* u8 kstb6; */
/* }; */

/* extern struct power_para power_handle; */

#define REGS_NUM       40

#define BT_REGS_NUM    1

struct power_driver_hdl{
	u8 mode;
	u32 total;
	void (*callback)();
    u8 delay_usec;
    u8 is_poweroff;

    u32 regs[REGS_NUM];
    u32 bt_regs[BT_REGS_NUM];
    u8 osc_type;
    u8 is_use_PR;
    u32 osc_hz;
    u32 Tosc;
    u8 kstb0;
    u8 kstb1;
    u8 kstb2;
    u8 kstb3;
    u8 kstb4;
    u8 kstb5;
    u8 kstb6;
};

static struct power_driver_hdl  driver_hdl SEC(.btmem_highly_available);

#define __this (&driver_hdl)
/********************************************************************************/
/*
 *                   HW Sfr Layer
 */

////////////////////////////////////////////////////////////
//  function:   delay_us
//  description: delay n uS
////////////////////////////////////////////////////////////
static void delay_us(u32 n)
{
   // while(n--);
    delay(n*__this->delay_usec);
}
////////////////////////////////////////////////////////////
//  function:   pwr_buf
//  description: power control write/read 1 byte
////////////////////////////////////////////////////////////
/* static u8 pwr_buf (u8 buf) SEC(.poweroff_text); */
static u8 pwr_buf (u8 buf)
{
    PD_DAT = buf;

    pmu_stran();

    while(pmu_swait());
    return PD_DAT;
}
////////////////////////////////////////////////////////////
//  function:    dcdc15_on
//  description: dcdc15 5v -> 1.5V on (bluetooth need this)
////////////////////////////////////////////////////////////
static void dcdc15_on(void){
    LDO_CON |= BIT(4);
    delay_us(200);   // ~200us
}



////////////////////////////////////////////////////////////
//  function:    dcdc15_off
//  description: dcdc15 5v -> 1.5v off
////////////////////////////////////////////////////////////
static void dcdc15_off(void){
    LDO_CON &= ~BIT(4);
}
////////////////////////////////////////////////////////////
//  function:    ldo15_on
//  description: ldo 5v -> 1.5V on
////////////////////////////////////////////////////////////
static void ldo15_on(void){
    LDO_CON |= BIT(5);
    delay_us(10);    // ~10uS
}



////////////////////////////////////////////////////////////
//  function:    ldo15_off
//  description: ldo 5v -> 1.5v off
////////////////////////////////////////////////////////////
void ldo15_off (void){
    LDO_CON &= ~BIT(5);
}

////////////////////////////////////////////////////////////
//  function:    sys_use_ldo33
//  description: digital system use ldo33
////////////////////////////////////////////////////////////
static void sys_use_ldo33(void){
    if(LDO_CON & BIT(2))            // VDD12_EN
    {
        SFR(LDO_CON, 12, 3, 0b010);     // 3.3 -> 1.2 DVDD set to 1.3v
        SFR(LDO_CON, 21, 3, 0b010);     // 3.3 -> 1.2 DVDDA set to 1.3v
        pmu_csen();
        pwr_buf(WR_IVS_SET1);
        pwr_buf(BIT(6));                // MLDO12_EN
        pmu_csdis();
        delay_us(10);     // ~10uS
        LDO_CON &= ~BIT(2);
        SFR(LDO_CON, 12, 3, 0b011);     // 3.3 -> 1.2 DVDD set to 1.2v
        SFR(LDO_CON, 21, 3, 0b011);     // 3.3 -> 1.2 DVDDA set to 1.2v
    }
    else
        return;
}
////////////////////////////////////////////////////////////
//  function:    sys_use_ldo15
//  description: digital system use ldo15
////////////////////////////////////////////////////////////
static void sys_use_ldo15(void){
    if(LDO_CON & BIT(2))            // VDD12_EN
    {
        if(LDO_CON & BIT(4))        // BTDCDC15_EN
        {
            LDO_CON |= BIT(5);
            delay_us(10);     // ~10uS
            LDO_CON &= ~BIT(4);
            return;
        }
        else if(LDO_CON & BIT(5))   // BTLDO15_EN
            return;
    }
    else
    {
        SFR(LDO_CON, 15, 3, 0b010);     // 1.5 -> 1.2 DVDD set to 1.3v
        SFR(LDO_CON, 24, 3, 0b010);     // 1.5 -> 1.2 DVDDA set to 1.3v
        LDO_CON |= BIT(2);
        delay_us(10);     // ~10uS
        SFR(LDO_CON, 12, 3, 0b111);     // 3.3 -> 1.2 DVDD set to min
        SFR(LDO_CON, 21, 3, 0b111);     // 3.3 -> 1.2 DVDDA set to min
        SFR(LDO_CON, 15, 3, 0b011);     // 1.5 -> 1.2 DVDD set to 1.2v
        SFR(LDO_CON, 24, 3, 0b011);     // 1.5 -> 1.2 DVDDA set to 1.2v
        return;
    }
}
////////////////////////////////////////////////////////////
//  function:    sys_use_dcdc15
//  description: digital system use dcdc15
////////////////////////////////////////////////////////////
static void sys_use_dcdc15(void){
    if(LDO_CON & BIT(2))            // VDD12_EN
    {
        if(LDO_CON & BIT(4))        // BTDCDC15_EN
            return;
        else if(LDO_CON & BIT(5))   // BTLDO15_EN
        {
            LDO_CON |= BIT(4);
            delay_us(10);     // ~10uS
            LDO_CON &= ~BIT(5);
            return;
        }
    }
    else
    {
        SFR(LDO_CON, 15, 3, 0b010);     // 1.5 -> 1.2 DVDD set to 1.3v
        SFR(LDO_CON, 24, 3, 0b010);     // 1.5 -> 1.2 DVDDA set to 1.3v
        LDO_CON |= BIT(2);
        delay_us(10);     // ~10uS
        SFR(LDO_CON, 12, 3, 0b111);     // 3.3 -> 1.2 DVDD set to min
        SFR(LDO_CON, 21, 3, 0b111);     // 3.3 -> 1.2 DVDDA set to min

        SFR(LDO_CON, 15, 3, 0b011);     // 1.5 -> 1.2 DVDD set to 1.2v
        SFR(LDO_CON, 24, 3, 0b011);     // 1.5 -> 1.2 DVDDA set to 1.2v
        /*set to 1.1v can save about 400uA*/
        //SFR(LDO_CON, 15, 3, 0b100);     // 1.5 -> 1.2 DVDD set to 1.1v
        //SFR(LDO_CON, 24, 3, 0b100);     // 1.5 -> 1.2 DVDDA set to 1.1v
        return;
    }
}
////////////////////////////////////////////////////////////
//  function:   get_pwrmd
//  description: get power mode
//
//  return:
//  1:      LDOIN 5v -> VDDIO 3.3v -> DVDD 1.2v
//  2:      LDOIN 5v -> LDO 1.5v -> DVDD 1.2v, support bluetooth, pin compatible with BT15
//  3:      LDOIN 5v -> DCDC 1.5v -> DVDD 1.2v, support bluetooth
//  others: configuration error
////////////////////////////////////////////////////////////
static u8 __ldo_get_pwrmd(void)
{
    if(LDO_CON & BIT(2))                // VDD12_EN
    {
        if(LDO_CON & BIT(4))            // BTDCDC15
            return 3;
        else if(LDO_CON & BIT(5))       // BTLDO15
            return 2;
    }
    else
        return 1;

    return 0xff;
}
////////////////////////////////////////////////////////////
//  function:   set_pwrmd
//  description: set power mode
//
//  option:
//  sm == 0:  no change
//  sm == 1:  LDOIN 5v -> VDDIO 3.3v -> DVDD 1.2v
//  sm == 2:  LDOIN 5v -> LDO 1.5v -> DVDD 1.2v, support bluetooth, pin compatible with BT15
//  sm == 3:  LDOIN 5v -> DCDC 1.5v -> DVDD 1.2v, support bluetooth
//
//  returun:
//  0:        success
//  others:   configuration error
////////////////////////////////////////////////////////////
static u8 __ldo_set_pwrmd(u8 sm)
{
    u8 tmp8;
    tmp8 = __ldo_get_pwrmd();

    if(tmp8 == 1)
    {
        if((sm == 0) || (sm == 1))
            return 0;
        else if(sm == 2)
        {
            ldo15_on();
            sys_use_ldo15();
            return 0;
        }
        else if(sm == 3)
        {
            dcdc15_on();
            sys_use_dcdc15();
            return 0;
        }
    }
    else if(tmp8 == 2)
    {
        if(sm == 1)
        {
            sys_use_ldo33();
            ldo15_off();
            return 0;
        }
        else if((sm == 0) || (sm == 2))
            return 0;
        else if(sm == 3)
        {
            sys_use_ldo33();
            ldo15_off();
            dcdc15_on();
            sys_use_dcdc15();
            return 0;
        }
    }
    else if(tmp8 == 3)
    {
        if(sm == 1)
        {
            sys_use_ldo33();
            dcdc15_off();
            return 0;
        }
        else if(sm == 2)
        {
            sys_use_ldo33();
            dcdc15_off();
            ldo15_on();
            sys_use_ldo15();
            return 0;
        }
        else if((sm == 0) || (sm == 3))
            return 0;
    }

    return 0xff;
}

static void __pmu_powerdown(void)
{
    u8 pd_tmp;

    pmu_csen();
    pwr_buf(RD_PMU_CON0);
    pd_tmp = pwr_buf(0);
    pmu_csdis();

    pmu_csen();
    pwr_buf(WR_PMU_CON0);
    //[6] pend [1] mode
    pwr_buf((pd_tmp &  ~BIT(1)) | BIT(6));
    pmu_csdis();
}

static void __pmu_poweroff(void)
{
    u8 pd_tmp;

    pmu_csen();
    pwr_buf(RD_PMU_CON0);
    pd_tmp = pwr_buf(0);
    pmu_csdis();

    pmu_csen();
    pwr_buf(WR_PMU_CON0);
    //[6] pend [1] mode
    pwr_buf(pd_tmp | BIT(1));
    pmu_csdis();
}

static void __pmu_soft_poweroff(void)
{
    u8 pd_tmp;

    pmu_csen();
    pwr_buf(RD_PMU_CON0);
    pd_tmp = pwr_buf(0);
    pmu_csdis();

    pmu_csen();
    pwr_buf(WR_PMU_CON0);
    //[6] pend [1:0] mode
    pwr_buf(pd_tmp & ~BIT(0));
    pmu_csdis();
}

static void __ldo12_disable(void)
{
    u8 pd_tmp;

    pmu_csen();               //wldo12_en
    pwr_buf(RD_PWR_SCON1);
    pd_tmp = pwr_buf(0);
    pmu_csdis();

    pmu_csen();
    pwr_buf(WR_PWR_SCON1);
    pd_tmp &= ~BIT(1);
    pwr_buf(pd_tmp);
    pmu_csdis();
}

static void __ldo12_enable(void)
{
    u8 pd_tmp;

    pmu_csen();               //WLDO12_EN
    pwr_buf(RD_PWR_SCON1);
    pd_tmp = pwr_buf(0);
    pmu_csdis();

    pmu_csen();
    pwr_buf(WR_PWR_SCON1);
    pwr_buf(pd_tmp | BIT(1));
    pmu_csdis(); 
}

static void __ldo_disable(void)
{
    pmu_csen();
    pwr_buf(WR_IVS_SET0);
    PD_DAT = 0xe0;      // pwvld, 12en, 33en
    //PD_CON |= BIT(5);
    //nop;
    //nop;
    //nop;
    //nop;
    //nop;
    //PWR_CON = BIT(4);           //soft rst_
    __asm__ volatile ("movl r4, 0x1200  ");         //LSB_DIV 0
    __asm__ volatile ("movh r4, 6       ");
    __asm__ volatile ("memor r4, 0, 32  ");
    __asm__ volatile ("nop              ");
    __asm__ volatile ("nop              ");
    __asm__ volatile ("nop              ");
    __asm__ volatile ("nop              ");
    __asm__ volatile ("nop              ");
    __asm__ volatile ("movl r0, 64      ");
    __asm__ volatile ("movh r0, 6       ");
    __asm__ volatile ("movs r1, 16      ");
    __asm__ volatile ("sw r1, r0        ");
}


void __pmu_debug(void)
{
    u8 pd_tmp;

    pd_printf("PDCON : %02x\n", PD_CON);
    pd_printf("WLA_CON13 : %02x\n", WLA_CON13);

    /* pmu_csen(); */
    /* pwr_buf(WR_MD_CON); */
    /* pwr_buf(1); */
    /* pmu_csdis(); */

    pmu_csen();
    pwr_buf(RD_PMU_CON0);
    u8 i;

    while(i <= 0x1d)
    {
        pd_tmp = pwr_buf(0);
        pd_printf("%02x : %02x\n", i, pd_tmp);
        i++;
    }
    pmu_csdis();
}
/********************************************************************************/
/*
 *                   HW API Layer
 */
static void __hw_power_down_enter(void)
{
    __pmu_powerdown();
    __ldo12_disable();
}

static void __hw_power_off_enter(void)
{
    __pmu_poweroff();
    __ldo12_enable();
}


static void __hw_power_down_exit(void)
{

}

static void __hw_power_off_exit(void)
{

}

//void power_init(u8 mode)SEC(.poweroff_flash);
static void __hw_power_init(u8 osc_type)
{
    if(osc_type== BT_OSC)
    {
        PD_CON = pd_con_init_bt;
    }
    else
    {
        PD_CON = pd_con_init_rtc;
    }

    pmu_csen();
    pwr_buf(WR_PMU_CON0);
    pwr_buf(pd_con0_init);
   // pwr_buf(pd_con1_init);
    switch(osc_type)
    {
    case BT_OSC:
        pwr_buf(pd_con1_init_bt);
        break;
    case RTC_OSCL:
        pwr_buf(pd_con1_init_rtcl);
        break;
    case RTC_OSCH:
        pwr_buf(pd_con1_init_rtch);
        break;
    }

    pwr_buf(pd_con2_init);
    pmu_csdis();

    /* u8 tmp; */
    /* pmu_csen(); */
    /* pwr_buf(RD_PWR_SCON0); */
    /* tmp = pwr_buf(0); */
    /* pmu_csdis(); */
    /* printf("RD_PWR_SCON0: %02x \n", tmp); */

    /* pmu_csen(); */
    /* pwr_buf(WR_PWR_SCON0); */
    /* pwr_buf(tmp | BIT(7)); */
    /* pmu_csdis(); */

  // bt_printf("pd_con4_init=%")

    pd_printf("stab : %02x - %02x\n", __this->kstb1, __this->kstb0);
    pd_printf("stab : %02x - %02x\n", __this->kstb3, __this->kstb2);
    pd_printf("stab : %02x - %02x\n", __this->kstb5, __this->kstb4);
    pd_printf("stab : %02x\n", __this->kstb6);

    pmu_csen();
    pwr_buf(WR_STB10_SET);
    pwr_buf(pd_con4_init(__this->kstb1,__this->kstb0));
    pwr_buf(pd_con5_init(__this->kstb3,__this->kstb2));
    pwr_buf(pd_con6_init(__this->kstb5,__this->kstb4));
    pwr_buf(pd_con7_init(__this->kstb6));
    pmu_csdis();


    pmu_csen();
    pwr_buf(RD_PWR_SCON1);
    pwr_buf(pd_con21_init);
    pwr_buf(pd_con22_init);
    pwr_buf(pd_con23_init);
    pmu_csdis();


    LDO_CON = LDO_CON & (~(0X7 << 18) | (0X0 << 18));     //WVDD min
    pd_printf("osc type: %04d\n", osc_type);
    /* __pmu_debug(); */
}

static u8 __hw_power_is_poweroff(void)SEC(.poweroff_flash);
static u8 __hw_power_is_poweroff(void)
{
    u8 pwr_down_wkup;

    pmu_csen();
    pwr_buf(RD_PMU_CON0);
    pwr_down_wkup = pwr_buf(0);
    pmu_csdis();

    if(pwr_down_wkup & BIT(5))
    {
        return 1;
    }
    return 0;
}
/***********************************************************/
/*
 *
 * bb 
 *      _________                         ___________
 *               |_______________________|
 * cpu           Tprp                 Tcke
 *      ____________                   ______________
 *                  |_________________|
 *
 */
/***********************************************************/
//RTC OSC sel 32K
/* #define TICK_PER_SEC 	31250 */
//BT OSC sel 12M / prescale 187500 
#define TICK_PER_SEC        (__this->osc_hz)


/* #define T_DEBUG_EN */

#ifdef T_DEBUG_EN
#define t_putchar(x)        putchar(x)
#define t_puts(x)           puts(x)
#define t_put_u8hex(x)      put_u8hex(x)
#define t_put_u32hex(x)     put_u32hex(x)
#define t_put_u64hex(x)     put_u64hex(x)
#define t_buf(x,y)          printf_buf(x,y)
#define t_printf            printf
#else
#define t_putchar(...)
#define t_puts(...)
#define t_put_u8hex(...)
#define t_put_u32hex(...)
#define t_put_u64hex(...)
#define t_buf(...)
#define t_printf(...)
#endif
/* static  */
/* __attribute__((noinline))  */
u32 __tcnt_us(u32 x)
{
    u64 y;

    t_puts("\n");
    t_puts(__func__);
    t_put_u32hex(x);
    t_put_u32hex(__this->osc_hz);
    y = (u64)x*(__this->osc_hz);
    t_put_u64hex(y);
    y = y/1000000L;
    t_put_u32hex(y);
    return (u32)y;
}

/* static  */
u32 __tcnt_ms(u32 x)
{
    u64 y;

    t_puts("\n");
    t_puts(__func__);
    t_put_u32hex(x);
    t_put_u32hex(__this->osc_hz);
    y = ((u64)x)*(__this->osc_hz);
    t_put_u64hex(y);
    y = y/1000L;
    t_put_u32hex(y);
    return (u32)y;
}

/* static  */
u32 __tus_cnt(u32 x)
{
    t_puts("\n");
    t_puts(__func__);
    u64 y;
    t_put_u32hex(x);
    t_put_u32hex(__this->osc_hz);
    y = ((u64)__this->total)*1000000L;
    t_put_u64hex(y);
    y = y/(__this->osc_hz);
    t_put_u32hex(y);

    return (u32)y;
}

/* static  */
u32 __hw_poweroff_time(u32 usec, u8 mode)
{
    u32 tmp32;
    u32 Tcke, Tprp;

    if(mode)
    {
        Tprp = __tcnt_ms(30);     ///保存时间
        Tcke = __tcnt_ms(200);    ///恢复时间6000L;// ms
    }
    else
    {
        Tprp = __tcnt_ms(3);      ///保存时间
        Tcke = __tcnt_ms(3);      ///恢复时间6000L;// ms
        /* pd_puts("Tprp : "); */
        /* put_u32hex(Tprp); */
        /* pd_puts("Tcke : "); */
        /* put_u32hex(Tcke); */
    }

    /* pd_printf("osc hz: %08d\n", TICK_PER_SEC); */
    //total cnt = usec / (1/period(Hz)/1000000)(us)
    __this->total = __tcnt_us(usec);

    /* put_u32hex(usec); */
    /* pd_puts("Total : "); */
    /* put_u32hex(__this->total); */
	/**/ 
	if (__this->total < (Tprp + Tcke))
	{
		putchar('X');
		return 0;
	}


    pmu_csen();
    pwr_buf(WR_LVC3_SET);
    tmp32 = __this->total;
    pwr_buf((u8)(tmp32 >> 24));
    pwr_buf((u8)(tmp32 >> 16));
    pwr_buf((u8)(tmp32 >> 8));
    pwr_buf((u8)(tmp32 >> 0));
    tmp32 = __this->total - Tcke  - (1<<__this->kstb4) -
                                    (1<<__this->kstb5) -
                                    (1<<__this->kstb6);
    /* put_u32hex(tmp32); */
    pwr_buf((u8)(tmp32 >> 24));
    pwr_buf((u8)(tmp32 >> 16));
    pwr_buf((u8)(tmp32 >> 8));
    pwr_buf((u8)(tmp32 >> 0));

    pwr_buf((u8)(Tprp >> 8));
    pwr_buf((u8)(Tprp >> 0));
    pmu_csdis();


	usec = __tus_cnt(__this->total);
    /* put_u32hex(usec); */

	return usec;
}





void bt_poweroff_recover()
{
    u64 m,Tosc,K1,K2,Tdr;
    unsigned int K, R;
    u32 aa;

	__this->callback();
//    put_u8hex(power_handle.mode);
//    put_buf(test_buf,100);
//    my_memcpy(test_buf,0,100);

///    bdedr_poweroff_recover();
}


#define rtc_cs_h            IRTC_CON  |= BIT(8)
#define rtc_cs_l            IRTC_CON  &= ~BIT(8)

#define rtc_ck_h            IRTC_CON  |= BIT(9)
#define rtc_ck_l            IRTC_CON  &= ~BIT(9)

#define rtc_do_h            IRTC_CON  |= BIT(10)
#define rtc_do_l            IRTC_CON  &= ~BIT(10)

#define rtc_di              IRTC_CON & BIT(11)

/* void rtc_tbuf (u8 buf) SEC(.poweroff_text); */
void rtc_tbuf (u8 buf)
{
    u8 aaa, cnt;

    cnt = 8;
    while(cnt--){
        aaa = buf & BIT(7);
        buf = buf << 1;

        if(aaa)
            rtc_do_h;
        else
            rtc_do_l;
        rtc_ck_h;
        rtc_ck_l;
    }
}
/* u8 rtc_rbuf (void )SEC(.poweroff_text); */
u8 rtc_rbuf (void )
{
    u8 bbb, cnt;

    cnt = 8;
    while(cnt--){

        rtc_ck_h;
        rtc_ck_l;

        bbb = bbb << 1;
        if(rtc_di)
            bbb = bbb | BIT(0);
    }
    return bbb;
}

static void __rtc_port_set(void)
{
    u8 rtc_tmp;

    rtc_cs_h;
    rtc_tbuf(0x28 | 0X80);        //rd
    rtc_tmp = rtc_rbuf();
    rtc_cs_l;

    rtc_cs_h;
    rtc_tbuf(0x28);               //PR1 DIE wr
    rtc_tbuf(rtc_tmp & ~0x02);    //DIE disable
    rtc_cs_l;
}

static void __rtc_port_set1(void)
{
    u8 rtc_tmp;

    rtc_cs_h;
    rtc_tbuf(0x28 | 0X80);        //rd
    rtc_tmp = rtc_rbuf();
    rtc_cs_l;

    rtc_cs_h;
    rtc_tbuf(0x28);               //PR1 DIE wr
    rtc_tbuf(rtc_tmp & BIT(1));    //DIE disable
    rtc_cs_l;
}

static void __regs_push(u32 *ptr, u8 num)
{
    u32 *ptr_begin;

    ptr_begin = ptr;
    /////////////////io///////////////////
    *ptr++ = PORTA_DIR;
    *ptr++ = PORTA_DIE;
    *ptr++ = PORTA_PU;
    *ptr++ = PORTA_PD;

    *ptr++ = PORTB_DIR;
    *ptr++ = PORTB_DIE;
    *ptr++ = PORTB_PU;
    *ptr++ = PORTB_PD;

    *ptr++ = PORTC_DIR;
    *ptr++ = PORTC_DIE;
    *ptr++ = PORTC_PU;
    *ptr++ = PORTC_PD;

    *ptr++ = PORTD_DIR;
    *ptr++ = PORTD_DIE;
    *ptr++ = PORTD_PU;
    *ptr++ = PORTD_PD;

    *ptr++ = USB_IO_CON;
    *ptr++ = USB_CON0;
    *ptr++ = USB_CON1;


    *ptr++ = ADC_CON;
    /////////////////adc///////////////////
    ADC_CON = 0;

    *ptr++ =DAA_CON2;
    /////////////////vcm///////////////////
    DAA_CON2 = 0;

    *ptr++ =DAA_CON0;
    DAA_CON0 = 0;

    *ptr++ = LVD_CON;

    /* pd_printf("ptr : %08d - %08d\n", (ptr - ptr_begin), num); */
    LVD_CON = 0;

    ASSERT(((ptr - ptr_begin) <= (num)), "%s\n", __func__);
}



static void __regs_pop(u32 *ptr, u8 num)
{
    u32 *ptr_begin;

    ptr_begin = ptr;

    PORTA_DIR = *ptr++ ;
    PORTA_DIE = *ptr++ ;
    PORTA_PU  = *ptr++ ;
    PORTA_PD = *ptr++ ;

    PORTB_DIR=*ptr++ ;
    PORTB_DIE=*ptr++ ;
    PORTB_PU=*ptr++ ;
    PORTB_PD=*ptr++ ;

    PORTC_DIR=*ptr++ ;
    PORTC_DIE=*ptr++ ;
    PORTC_PU=*ptr++ ;
    PORTC_PD=*ptr++ ;

    PORTD_DIR =*ptr++ ;
    PORTD_DIE = *ptr++ ;
    PORTD_PU = *ptr++ ;
    PORTD_PD = *ptr++ ;

    USB_IO_CON =*ptr++ ;
    USB_CON0 =*ptr++ ;
    USB_CON1 =*ptr++ ;

    ADC_CON = *ptr++ ;
    DAA_CON2 = *ptr++ ;
    DAA_CON0 = *ptr++ ;
    LVD_CON = *ptr++ ; 

    ASSERT(((ptr - ptr_begin) <= (num)), "%s\n", __func__);
}

static void __hw_cache_idle(void) SEC(.poweroff_text);
static void __hw_cache_idle()
{
    //等flash 操作完毕
    while(!(DSPCON&BIT(5)));
    SFC_CON &=~BIT(0);
}

static void __hw_cache_run(void) SEC(.poweroff_text);
static void __hw_cache_run()
{
    SFC_CON |= BIT(0);
}

static void __hw_ldo_sw30(void) SEC(.poweroff_text);
static void __hw_ldo_sw30(void)
{
    SFR(LDO_CON, 8, 2, 0x3);    // VDDIO set to 3.0v 
}

static void __hw_ldo_sw33(void) SEC(.poweroff_text);
static void __hw_ldo_sw33(void)
{
    SFR(LDO_CON, 8, 2, 0x1);    // VDDIO set to 3.3v
}

/* static void __hw_bt_analog_off(void) SEC(.poweroff_text); */
static void __hw_bt_analog_off(void)
{
    WLA_CON0 &= ~BIT(0);
}

/* static void __hw_bt_analog_on(void) SEC(.poweroff_text); */
static void __hw_bt_analog_on(void)
{
    WLA_CON0 |= BIT(0);
}

static void __bt_regs_push(u32 *ptr, u8 num)
{
    u32 *ptr_begin;

    //push regs
    ptr_begin = ptr;

    *ptr++ = WLA_CON14;

    ASSERT(((ptr - ptr_begin) <= (num)), "%s\n", __func__);

    //changer regs
    //bt osc en
    //HCS[3:0] = 1000, CLS[4:0] = 10000, CRS[4:0] = 10000,   XCK_SYS_OE = 1
    //WLA_CON14[4:1], WLA_CON13[9:5],   WLA_CON13[14:10],   WLA_CON14[7]
    SFR(WLA_CON14,1 ,4,0x0);
}

static void __bt_regs_pop(u32 *ptr, u8 num)
{
    u32 *ptr_begin;

    ptr_begin = ptr;

    WLA_CON14 = *ptr++;

    ASSERT(((ptr - ptr_begin) <= (num)), "%s\n", __func__);
}

static __hw_ldo_vbg_off(void)
{
    LDO_CON &= ~BIT(0);    //VBG
}

static __hw_ldo_vbg_on(void)
{
    LDO_CON |= BIT(0);    //VBG
}

static u8 __hw_power_is_wakeup(void) SEC(.poweroff_text);
static u8 __hw_power_is_wakeup(void)
{
    u8 pd_tmp;

    pmu_csen();
    pwr_buf(RD_STB6_SET);
    pd_tmp = pwr_buf(0);
    pmu_csdis();

    return (pd_tmp & BIT(7)) ? 1 : 0;
}

static void __hw_pmu_reset_mask(void)
{
    pmu_csen();
    pwr_buf(WR_IVS_SET0);
    pwr_buf(0x20);
    pmu_csdis();
}
/********************************************************************************/
/*
 *                   HW Abstract Layer
 */
#if 1
typedef enum
{
    RUN_RAM = 0,
	MAX_INS,
	RUN_NORMAL = 0xff,
}SKIP_INS;


const u8 maskrom_ins[MAX_INS][8]  = {
    0xBF,0XEC,0XCB,0XD9,0XC6,0XF4,0XB6,0XAF,    //快速启动
};

#define RAM1_START      0x40000L
#define RAM1_SIZE       (24*1024L)
#define RAM1_END        (RAM1_START + RAM1_SIZE)


#define MAGIC_ADDR		(RAM1_END-8)

#define FUNC_ADDR 		(RAM1_END-12)
/*
 *	--设置启动状态
 */
void maskrom_call(SKIP_INS cmd)
{
	u8 j;

	u8 *p_magic_addr;

	p_magic_addr = MAGIC_ADDR;

    for(j = 0; j < 8; j++)
    {
        p_magic_addr[j] = maskrom_ins[cmd][j];
    }
///	*((u32*)FUNC_ADDR) = _btpower_up;//__reset_from_power_off;
}


/*
 *	--清空启动状态
 */
void maskrom_clear(void)
{
    u8 j;

	for(j = 0; j < 8; j++)
    {
        ((u8 *)MAGIC_ADDR)[j] = 0;
    }
}
#endif

static u32 __do_power_off(u32 usec, int mode)
{
    maskrom_clear();
    /* maskrom_call(RUN_RAM); */
    /* u8 tmp; */
    /* pmu_csen(); */
    /* pwr_buf(RD_PMU_CON1); */
    /* tmp = pwr_buf(0); */
    /* pmu_csdis(); */
    /* printf("PMU CON1 %x\n", tmp); */

    /* pmu_csen(); */
    /* pwr_buf(RD_PWR_SCON0); */
    /* tmp = pwr_buf(0); */
    /* pmu_csdis(); */
    /* printf("PMU CON3 %x\n", tmp); */

    if (mode){
        __hw_power_off_enter();    
    }
    else{
        __hw_power_down_enter();    
    }

    /* usec = 0x7A120; */
    /* usec = 0x8A120; */
    /* usec = 0x5A120; */
    /* usec = 0x9A120; */
	return __hw_poweroff_time(usec, mode);
}

static void __power_init()
{
	__hw_power_init(__this->osc_type);
}

static u32 __power_suspend(u32 usec, void (*resume)())
{
	__this->mode = 0;
	__this->callback = resume;

	//putchar('@'); 
	return __do_power_off(usec, __this->mode);
}


static void __power_down_enter(void) SEC(.poweroff_text);
static void __power_down_enter(void)
{
    __hw_ldo_vbg_off();

    if(__this->is_use_PR)
    {
        __rtc_port_set();
    }

    /* __this->user_power_down_enter_callback(); */

    __regs_push(__this->regs, REGS_NUM);

    if(__this->osc_type == BT_OSC)
    {
        __bt_regs_push(__this->bt_regs, BT_REGS_NUM);
    }
    __hw_bt_analog_off();
    __hw_cache_idle();
    __hw_ldo_sw30();
}


static void __power_down_exit(void) SEC(.poweroff_text);
static void __power_down_exit(void)
{
    while(__hw_power_is_wakeup() == 0)
    {
        /* pd_puts(__func__); */
    }
    __hw_ldo_sw33();
    __hw_cache_run();
    __hw_bt_analog_on();

    if(__this->osc_type == BT_OSC)
    {
        __bt_regs_pop(__this->bt_regs, BT_REGS_NUM);
    }

    __regs_pop(__this->regs, REGS_NUM);

    if(__this->is_use_PR)
    {
        __rtc_port_set1();
    }
    __hw_ldo_vbg_on();
}

static u32 __power_off(u32 usec, void (*on)())
{
	__this->mode = 1;
	__this->callback = on;

	//putchar('#'); 
	return __do_power_off(usec, __this->mode);
}

static void __power_off_enter(void)
{
}

static void __power_off_exit(void)
{
    
}

static void __power_ioctrl(int ctrl, ...)
{
	va_list argptr;
	va_start(argptr, ctrl);

	switch(ctrl)
	{
		case POWER_OSC_INFO:
            __this->osc_type = va_arg(argptr, int);
            __this->osc_hz = va_arg(argptr, int);
            __this->kstb0 = Kstb0(__this->osc_hz);
            __this->kstb1 = Kstb0(__this->osc_hz);
            __this->kstb2 = Kstb0(__this->osc_hz);
            __this->kstb3 = Kstb0(__this->osc_hz);
            __this->kstb4 = Kstb0(__this->osc_hz);
            __this->kstb5 = Kstb0(__this->osc_hz);
            __this->kstb6 = Kstb0(__this->osc_hz);
            if ((__this->osc_type == BT_OSC) || \
                (__this->osc_type == RTC_OSCH))
            {
                __this->osc_hz >>= 6;
            }
            pd_printf("POWER_OSC_INFO: %04d - %08d\n", __this->osc_type, __this->osc_hz);
            break;
		case POWER_RTC_PORT:
            __this->is_use_PR = va_arg(argptr, int);
            pd_printf("POWER_RTC_PORT: %x\n", __this->is_use_PR);
            break;
        case POWER_DELAY_US:
            __this->delay_usec = va_arg(argptr, int);
            pd_printf("POWER_DELAY_US: %x\n", __this->delay_usec);
            break;

        default:
            break;
    }

	va_end(argptr);
}


u8 __power_is_poweroff()SEC(.poweroff_flash);
u8 __power_is_poweroff()
{
    /* __hw_pmu_reset_mask(); */
    return __hw_power_is_poweroff();
}


static const struct bt_power_driver bt_power_ins  = {
	.init               = __power_init,
	.set_suspend_timer  = __power_suspend,
    .suspend_enter      = __power_down_enter,
    .suspend_exit       = __power_down_exit,
	.set_off_timer      = __power_off,
    .off_enter          = __power_off_enter,
    .off_exit           = __power_off_exit,
    .ioctrl             = __power_ioctrl,
};
REGISTER_POWER_OPERATION(bt_power_ins);

