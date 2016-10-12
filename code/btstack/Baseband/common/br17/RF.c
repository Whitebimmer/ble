#include "RF.h"
#include "typedef.h"



unsigned const char bt_lo_sel[83] =
{
/*2400*/  0x4,
/*2401*/  0x2,
/*2402*/  0x2,
/*2403*/  0x2,
/*2404*/  0x4,
/*2405*/  0x2,
/*2406*/  0x2,
/*2407*/  0x2,
/*2408*/  0x4,
/*2409*/  0x2,
/*2410*/  0x2,
/*2411*/  0x2,
/*2412*/  0x4,
/*2413*/  0x2,
/*2414*/  0x2,
/*2415*/  0x2,
/*2416*/  0x4,
/*2417*/  0x2,
/*2418*/  0x2,
/*2419*/  0x2,
/*2420*/  0x4,
/*2421*/  0x2,
/*2422*/  0x2,
/*2423*/  0x2,
/*2424*/  0x4,
/*2425*/  0x2,
/*2426*/  0x2,
/*2427*/  0x2,
/*2428*/  0x2,     ///
/*2429*/  0x2,
/*2430*/  0x2,
/*2431*/  0x2,
/*2432*/  0x4,
/*2433*/  0x2,
/*2434*/  0x2,
/*2435*/  0x2,
/*2436*/  0x4,
/*2437*/  0x2,
/*2438*/  0x2,
/*2439*/  0x2,
/*2440*/  0x4,
/*2441*/  0x2,
/*2442*/  0x2,
/*2443*/  0x2,
/*2444*/  0x4,
/*2445*/  0x2,
/*2446*/  0x2,
/*2447*/  0x2,
/*2448*/  0x4,
/*2449*/  0x2,
/*2450*/  0x2,
/*2451*/  0x2,
/*2452*/  0x4,
/*2453*/  0x2,
/*2454*/  0x2,
/*2455*/  0x2,
/*2456*/  0x4,
/*2457*/  0x2,
/*2458*/  0x2,
/*2459*/  0x2,
/*2460*/  0x4,
/*2461*/  0x2,
/*2462*/  0x2,
/*2463*/  0x2,
/*2464*/  0x4,
/*2465*/  0x2,
/*2466*/  0x2,
/*2467*/  0x2,
/*2468*/  0x4,
/*2469*/  0x2,
/*2470*/  0x2,
/*2471*/  0x2,
/*2472*/  0x4,
/*2473*/  0x2,
/*2474*/  0x2,
/*2475*/  0x2,
/*2476*/  0x4,
/*2477*/  0x2,
/*2478*/  0x2,
/*2479*/  0x2,
/*2480*/  0x4,
/*2481*/  0x2,
/*2482*/  0x2
};

static unsigned const short bt_frac_pll_int[83] =
{
/*2400*/    200,
/*2401*/    200,
/*2402*/    200,
/*2403*/    200,
/*2404*/    200,
/*2405*/    200,
/*2406*/    200,
/*2407*/    200,
/*2408*/    200,
/*2409*/    200,
/*2410*/    200,
/*2411*/    200,
/*2412*/    201,
/*2413*/    201,
/*2414*/    201,
/*2415*/    201,
/*2416*/    201,
/*2417*/    201,
/*2418*/    201,
/*2419*/    201,
/*2420*/    201,
/*2421*/    201,
/*2422*/    201,
/*2423*/    201,
/*2424*/    202,
/*2425*/    202,
/*2426*/    202,
/*2427*/    202,
/*2428*/    202,
/*2429*/    202,
/*2430*/    202,
/*2431*/    202,
/*2432*/    202,
/*2433*/    202,
/*2434*/    202,
/*2435*/    202,
/*2436*/    203,
/*2437*/    203,
/*2438*/    203,
/*2439*/    203,
/*2440*/    203,
/*2441*/    203,
/*2442*/    203,
/*2443*/    203,
/*2444*/    203,
/*2445*/    203,
/*2446*/    203,
/*2447*/    203,
/*2448*/    204,
/*2449*/    204,
/*2450*/    204,
/*2451*/    204,
/*2452*/    204,
/*2453*/    204,
/*2454*/    204,
/*2455*/    204,
/*2456*/    204,
/*2457*/    204,
/*2458*/    204,
/*2459*/    204,
/*2460*/    205,
/*2461*/    205,
/*2462*/    205,
/*2463*/    205,
/*2464*/    205,
/*2465*/    205,
/*2466*/    205,
/*2467*/    205,
/*2468*/    205,
/*2469*/    205,
/*2470*/    205,
/*2471*/    205,
/*2472*/    206,
/*2473*/    206,
/*2474*/    206,
/*2475*/    206,
/*2476*/    206,
/*2477*/    206,
/*2478*/    206,
/*2479*/    206,
/*2480*/    206,
/*2481*/    206,
/*2482*/    206
};

static unsigned const long bt_frac_pll_frac[83] =
{
/*2400*/    0        ,
/*2401*/    1398101  ,
/*2402*/    2796203  ,
/*2403*/    4194304  ,
/*2404*/    5592405  ,
/*2405*/    6990507  ,
/*2406*/    8388608  ,
/*2407*/    9786709  ,
/*2408*/    11184811 ,
/*2409*/    12582912 ,
/*2410*/    13981013 ,
/*2411*/    15379115 ,
/*2412*/    0        ,
/*2413*/    1398101  ,
/*2414*/    2796203  ,
/*2415*/    4194304  ,
/*2416*/    5592405  ,
/*2417*/    6990507  ,
/*2418*/    8388608  ,
/*2419*/    9786709  ,
/*2420*/    11184811 ,
/*2421*/    12582912 ,
/*2422*/    13981013 ,
/*2423*/    15379115 ,
/*2424*/    0        ,
/*2425*/    1398101  ,
/*2426*/    2796203  ,
/*2427*/    4194304  ,
/*2428*/    5592405  ,
/*2429*/    6990507  ,
/*2430*/    8388608  ,
/*2431*/    9786709  ,
/*2432*/    11184811 ,
/*2433*/    12582912 ,
/*2434*/    13981013 ,
/*2435*/    15379115 ,
/*2436*/    0        ,
/*2437*/    1398101  ,
/*2438*/    2796203  ,
/*2439*/    4194304  ,
/*2440*/    5592405  ,
/*2441*/    6990507  ,
/*2442*/    8388608  ,
/*2443*/    9786709  ,
/*2444*/    11184811 ,
/*2445*/    12582912 ,
/*2446*/    13981013 ,
/*2447*/    15379115 ,
/*2448*/    0        ,
/*2449*/    1398101  ,
/*2450*/    2796203  ,
/*2451*/    4194304  ,
/*2452*/    5592405  ,
/*2453*/    6990507  ,
/*2454*/    8388608  ,
/*2455*/    9786709  ,
/*2456*/    11184811 ,
/*2457*/    12582912 ,
/*2458*/    13981013 ,
/*2459*/    15379115 ,
/*2460*/    0        ,
/*2461*/    1398101  ,
/*2462*/    2796203  ,
/*2463*/    4194304  ,
/*2464*/    5592405  ,
/*2465*/    6990507  ,
/*2466*/    8388608  ,
/*2467*/    9786709  ,
/*2468*/    11184811 ,
/*2469*/    12582912 ,
/*2470*/    13981013 ,
/*2471*/    15379115 ,
/*2472*/    0        ,
/*2473*/    1398101  ,
/*2474*/    2796203  ,
/*2475*/    4194304  ,
/*2476*/    5592405  ,
/*2477*/    6990507  ,
/*2478*/    8388608  ,
/*2479*/    9786709  ,
/*2480*/    11184811 ,
/*2481*/    12582912 ,
/*2482*/    13981013
};

struct common_rf_param rf;

static void bta_pll_config_init()
{
    unsigned char i, buf8;
	BT_PLLCONFIG_ADR = (volatile unsigned long)rf.pll_config_tab; 
	for(i=0; i<80; i++)
	{
        buf8 = get_bta_pll_bank(i+79);
        if(bt_lo_sel[i] == 2)
        {                              ///refdiven refs   mode1   mode0   caps     fbds
            rf.pll_config_tab[i*2] = (1<<28)|(10<<18)|(0<<16)|(0<<17)|(buf8<<8)|(79+i);
        }
        else if(bt_lo_sel[i] == 4)
        {
                                       ///refdiven refs   mode1   mode0   caps     fbds
            rf.pll_config_tab[i*2] = (1<<28)|(4<<18)|(0<<16)|(1<<17)|(buf8<<8)|(39+i/2);
        }
        else if(bt_lo_sel[i] == 0xf)
        {
                                       ///refdiven refs   mode1   mode0   caps     fbds
            rf.pll_config_tab[i*2] = (0<<28)|(0<<18)|(1<<16)|(0<<17)|(buf8<<8)|(79+i);
            rf.pll_config_tab[i*2+1] = (bt_frac_pll_int[i]<<20)|(bt_frac_pll_frac[i]>>4);
        }
        else
        {
            printf("\n\n\n.... bt_pll_hop_sel error !!! %d....\n\n\n", i);
        }
	}
}

static void bt_pll_para(u32 osc,u32 sys,u8 low_power,u8 xosc)
{
    //osc/(n+2)==2M
	/* bt_printf("&&&&&&&&&&&& %d,%d,%d,%d\n",osc,sys,low_power,xosc); */
	/* rf.tone_pwr=0;//bt_tone_power; */
	rf.xosc = xosc;
    if(rf.xosc > 0x0f)
    {
        rf.xosc = 0;
    }
	 /* rf.low_powr_en = low_power; */
    rf.pll_hen=0;
    if(osc>12000000L)
    {
        rf.pll_hen =1;
    }
    rf.delay_sys = sys/1000000L;
    rf.pll_para= (osc/1000000L/2)-2;
    /* bt_printf("low pwoer %d \n",bredr_rf.low_powr_en); */
}

/* AT_POWER */
static void RF_mdm_set_timer() 
{
    SFR(WL_CON2, 0,  4, PLL_RST);    // rst
    SFR(WL_CON1, 0,  8, PLL_T - RXLDO_T);          // rxLDO
    SFR(WL_CON1, 16, 8, PLL_T - TXLDO_T);          // txldo
    SFR(WL_CON1, 8,  8, RXLDO_T - RXEN_T);         // rxen
    SFR(WL_CON1, 24, 8, TXLDO_T - TXEN_T);         // txen

    SFR(WL_CON2, 8,  8, PLL_T - TXLDO_T);          // pllacc
    SFR(WL_CON2, 16, 8, 64);                       // lnav_bt
    SFR(WL_CON2, 24, 8, 32);                       // lnav_zb

    SFR(BT_MDM_CON1, 0, 8, PLL_T + PLL_RST-MDM_PREP_T);      // fsk 'stage0_en' block time select, time = (n)uS
    SFR(BT_MDM_CON2, 24, 8, (PLL_T+PLL_RST-10));   // fsk tx preamble early time select, time = (n)uS, set '0' means disable

}
/* AT_POWER */
static void RF_mdm_init(void)
{
    SFR(BT_MDM_CON0, 0, 1, 0);         // fsk symbol time track disable
    SFR(BT_MDM_CON0, 1, 3, 4);         // when to switch fsk mode to psk mode(in the 5uS guard)
    SFR(BT_MDM_CON0, 4, 4, 9);         // delay fsk data to fit the phase with psk data
    SFR(BT_MDM_CON0, 8, 3, 0);         // best sampling point offset
    SFR(BT_MDM_CON0, 11, 1, 0);        // psk symbol time track disable
    SFR(BT_MDM_CON0, 12, 2, 1);        // psk sync word match bit margin,  00: 6  01: 7  10: 8  11: 9
    SFR(BT_MDM_CON0, 14, 2, 0);        // fsk bandpass filter select
    SFR(BT_MDM_CON0, 16, 6, 0);        // fsk 'pmb_find' to 'stage2_find' delay, time = (n * 0.125)uS
    SFR(BT_MDM_CON0, 22, 1, 0);        // allow baseband to select hsb/lsb
    SFR(BT_MDM_CON0, 23, 1, 0);        // 0: hsb    1: lsb
    SFR(BT_MDM_CON0, 24, 1, 0);        // tx delta-sigma modulator level select,  0: 64 level  1: 32 level
    SFR(BT_MDM_CON0, 25, 1, 0);        // delta-sigma modulator enable mode,  1: always enable
    SFR(BT_MDM_CON0, 26, 3, 5);        // rx dc-cancelling setting
    SFR(BT_MDM_CON0, 29, 1, 0);        // fsk/psk amplitude separate, 1: fsk/psk use different amplitude
    SFR(BT_MDM_CON0, 30, 2, 0);        // psk carrier frequency deviation switch

    SFR(BT_MDM_CON1, 0, 8, 48);        // fsk 'stage0_en' block time select, time = (n)uS

    SFR(BT_MDM_CON1, 8, 4, 11);        // fsk 'stage0_find' to 'stage1_find' delay, time = (n * 0.125)uS
    SFR(BT_MDM_CON1, 12, 2, 0);        // fsk preamble power compare times, 0: 4    1: 3    2: 2.5    3: 2
    SFR(BT_MDM_CON1, 14, 2, 0);        // fsk carrier frequency deviation switch
    SFR(BT_MDM_CON1, 16, 1, 0);        // intermediate frequency enable
    SFR(BT_MDM_CON1, 17, 3, 0);        // intermediate frequency gate select, 0: 256uS  1: 512uS  ....  7: 32768uS
    SFR(BT_MDM_CON1, 20, 4, 3);        // v2.1 fsk sync word error bit margin
    SFR(BT_MDM_CON1, 24, 3, 4);        // psk first bests length, length = (best_len + 8)uS
    SFR(BT_MDM_CON1, 27, 1, 0);        // analog test enable(for testing only)
    SFR(BT_MDM_CON1, 28, 2, 0);        // fsk frequency offset compensation disable,  01: disable preamble  10: disable triler  11: disable both
    SFR(BT_MDM_CON1, 30, 1, 1);        // d8psk mode enable, 0: d8psk mode is disable
    SFR(BT_MDM_CON1, 31, 1, 0);        // dsf2psk agc select, 0: use fsk filter(rssi2)  1: use psk filter(rssi3)
    SFR(BT_MDM_CON2, 0, 7, 60);        // fsk preamble frequency threshold
    SFR(BT_MDM_CON2, 7, 3, 1);         // fsk preamble number threshold
    SFR(BT_MDM_CON2, 10, 1, 0);        // fsm preamble n sample match,  0: 4 sample  1: 3 sample
    SFR(BT_MDM_CON2, 12, 2, 3);        // fsk syncword mode,  01: by best sample phase  10: by syncword window  11: by both
    SFR(BT_MDM_CON2, 16, 4, 1);        // ble v4.x fsk sync word error bit margin
    SFR(BT_MDM_CON2, 20, 4, 0);        // noise level power select, np = 1 << nlp_sel
    SFR(BT_MDM_CON2, 24, 8, 0);        // fsk tx preamble early time select, time = (n)uS, set '0' means disable
    SFR(BT_MDM_CON3, 0, 10, 256);      // psk amplitude i channel
    SFR(BT_MDM_CON3, 10, 5, 24);        // fade in/fade out guard time setting, time = ((n + 1) * 0.125)uS
    SFR(BT_MDM_CON3, 15, 1, 0);        // fsk carrier filter delay select, 0: 48 samples    1: 32 samples
    SFR(BT_MDM_CON3, 16, 10, 256);     // psk amplitude q channel
    SFR(BT_MDM_CON3, 26, 2, 1);        // // fsk preamble power filter shift, 0: 256    1: 512    2: 744.7    3: 1024
    SFR(BT_MDM_CON4, 0, 10, 256);      // fsk amplitude i channel
    SFR(BT_MDM_CON4, 16, 10, 256);     // fsk amplitude q channel
    SFR(BT_MDM_CON5, 0, 10, 0);        // tx offset i channel
    SFR(BT_MDM_CON5, 16, 10, 0);       // tx offset q channel
    SFR(BT_MDM_CON6, 16, 12, 0);       // fsk carrier frequency deviation setting
        // BT_MDM_CON6[11:0]:          // fsk carrier frequency deviation value
    SFR(BT_MDM_CON7, 16, 10, 0);       // psk carrier frequency deviation setting
        // BT_MDM_CON7[9:0]:           // psk carrier frequency deviation value
    SFR(BT_MDM_CON8, 0, 8, 140);       // fsk v2.1 rx syncword timeout setting  71 + 69
    SFR(BT_MDM_CON8, 8, 8, 82);        // fsk v4.x rx syncword timeout setting  43 + 39

    BT_BSB_EDR  = (1<<7) | 57;         // rxmch  to edr sync
    SFR(BT_MDM_CON2, 14, 1, 1);        // FADE_EN
    SFR(BT_MDM_CON2, 15, 1, 0);        // FADE_EN_gt

    bredr_mdm_init(NULL);

    ble_mdm_init(NULL);

 	RF_mdm_set_timer();
}



void reload_bt_trim()
{
#if 0
    SFR(BT_MDM_CON4, 0,  14, 256);    // APM_I_256
    SFR(BT_MDM_CON4, 16, 14, 256);    // APsimsimM_Q_256
    SFR(BT_MDM_CON5, 0,  14, get_trim_i_dc());       // DC_I_0
    SFR(BT_MDM_CON5, 16, 14, get_trim_q_dc());
#endif
}

void set_single_carrier()
{
    SFR(BT_MDM_CON3, 0, 10, 0);      // psk amplitude i channel
    SFR(BT_MDM_CON3, 16, 10, 0);     // psk amplitude q channel
    SFR(BT_MDM_CON4, 0, 10, 0);      // fsk amplitude i channel
    SFR(BT_MDM_CON4, 16, 10, 0);     // fsk amplitude q channel
    SFR(BT_MDM_CON5, 0, 10, 256);        // tx offset i channel
    SFR(BT_MDM_CON5, 16, 10, 256);       // tx offset q channel
}

static void set_debug_io()
{
    PORTB_OUT |=BIT(0);
    delay(rf.delay_sys*10);
    PORTB_OUT&=~BIT(0);
}

static void reset_debug(u8 aa)
{
    /* DIAGCNTL =0; */
    /* DIAGCNTL = (1<<7)|(aa); */
}

static void rf_debug()
{
   //bt14Ð¡°åµÄ³ÌÐòÒª¸Ä
	SFR(WL_CON0,24,4,0);
	SFR(WL_CON0,8,1,1);
	SFR(BT_EXT_CON,2,3,2);

    /* DIAGCNTL = (1<<7)|(0); */
    BT_EXT_CON |=BIT(1);

    //¹Ø±Õspi¿ÚµÄ567¿Ú
    //SPI1_CON &= ~(1<<0);
    //IOMC1 &= ~(1<<4);

    //ÉèÖÃdebugÊä³ö¿Ú
	SFR(PORTB_DIR,0,8,0);
}
void RF_init()
{
    SFR(CLK_CON1, 14, 2, 0);
    delay(100);
    SFR(WL_CON0, 7, 1, 1);
    SFR(WL_CON0,12, 4, 1);
    SFR(WL_CON0,16, 4, 5);
    SFR(WL_CON0,20, 2, 2);


    SFR(WL_CON0, 0, 1, 1);
	delay(1000);
    BT_BSB_CON |= BIT(10);

	/* bt_puts("rf_pll_init 1\n"); */
	/* RWBTCNTL |= BIT(31)|BIT(30); */

    BT_BSB_CON |= BIT(14)|BIT(15)|BIT(13)|(1<<9)|(1<<6)|(1<<3)|BIT(8)|BIT(2)|BIT(1)|BIT(0);

    /* BT_LP_CON |= BIT(0);	//[>Dual mode<] */

	/* bt_puts("rf_pll_init 2\n"); */
	RF_mdm_init();
	/* rf_debug(); */

	/* bt_puts("rf_pll_init\n"); */
	if(bt_power_is_poweroff_post()) {
		return;
	}
	
	bt_rf_analog_init();
    bta_pll_config_init();

 	//bredr_init();
	/* bt_iq_trim(); */
}

///在main 开始的时候调用，trim 和读取记忆的数值
void bt_process_before_init()
{
    rf_debug();
	
}





