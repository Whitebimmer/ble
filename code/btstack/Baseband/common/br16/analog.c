#include "RF.h"
#include "cpu.h"
#include "typedef.h"

#define sys_clk     64

unsigned char get_bta_pll_bank(unsigned char set_freq);
static unsigned char trim_iq(short trim_w)
{
    short data_t,m,n,i_dc,q_dc, flag, i, dat_o, dat_o_d;


    SFR(BTA_CON7, 13 , 1, 0);       //EN_TRIMI_12v
    SFR(BTA_CON7, 14 , 1, 0);       //EN_TRIMQ_12v
   // delay(200000);
    for(i = 0; i<2; i++)
    {
        SFR(BT_MDM_CON3, 0,  10, 0);     // psk amplitude i channel
        SFR(BT_MDM_CON3, 16, 10, 0);     // psk amplitude q channel
        SFR(BT_MDM_CON4, 0,  10, 0);     // fsk amplitude i channel
        SFR(BT_MDM_CON4, 16, 10, 0);     // fsk amplitude q channel

        SFR(BT_MDM_CON5, 0,  10, 0);      // tx offset i channel
        SFR(BT_MDM_CON5, 16, 10, 0);      // tx offset q channel

        SFR(BTA_CON7, 13 , 1, 0);         //EN_TRIMI_12v
        SFR(BTA_CON7, 14 , 1, 0);         //EN_TRIMQ_12v
        flag = 1;
        m = trim_w;
        n = -trim_w;
        dat_o_d = 3;
        for(data_t = -trim_w; data_t <= trim_w ; data_t += 1 )
        {
            if(i== 0)
            {
               SFR(BT_MDM_CON5, 0,  10, data_t);       // DC_I_0
            }
            else if(i== 1)
            {
               SFR(BT_MDM_CON5, 16,  10, data_t);       // DC_Q_0
            }
             delay(sys_clk*100);
             dat_o = 0;
             dat_o += BTA_CON16 & 0X1;
             dat_o += BTA_CON16 & 0X1;
             dat_o += BTA_CON16 & 0X1;
          //  printf("data: %d LNAV: %x %x\n", data_t, dat_o&0x1, dat_o_d);
            if((dat_o >= 2) && (dat_o_d >= 2))
            {
                if((flag == 0) && (data_t > n))
                {
                    n = data_t;
                }
                flag = 1;
            }
            else if((dat_o < 2) && (dat_o_d < 2))
            {
                if((flag == 1) && (data_t < m))
                {
                    m = data_t;
                }
                flag = 0;
            }
            dat_o_d = dat_o;
        }

        if(i == 0)
        {
            i_dc = (n + m -1)/2;
            if( (m == trim_w) || (n == -trim_w) )
            {
                return 0;
            }
        }
        else if(i == 1)
        {
            q_dc = (n + m -1)/2;
            if( (m == trim_w) || (n == -trim_w) )
            {
                return 0;
            }
        }
         printf("\n=========================");
         printf("0>1n %d: 1>0M %d", n, m);
    }

    printf("\ni_dc: %d q_dc: %d\n", i_dc, q_dc);
    SFR(BT_MDM_CON3, 0,  10, 256);      // psk amplitude i channel
    SFR(BT_MDM_CON3, 16, 10, 256);      // psk amplitude q channel
    SFR(BT_MDM_CON4, 0,  10, 256);      // fsk amplitude i channel
    SFR(BT_MDM_CON4, 16, 10, 256);      // fsk amplitude q channel

    SFR(BT_MDM_CON5, 0,  10, i_dc);        // tx offset i channel
    SFR(BT_MDM_CON5, 16, 10, q_dc);       // tx offset q channel
    return 1;
}

static void trim_iq_all(void)
{
   unsigned char dat;

    SFR(BTA_CON0, 1 , 1, 1);      //RXLDO_EN_12v
    SFR(BTA_CON7, 12 , 1, 1);     //TXLDO_EN_12v
    SFR(BTA_CON1, 15 , 1, 1);     //LNAV_EN_12v

    SFR(BT_MDM_CON3, 0,  10, 0);      // psk amplitude i channel
    SFR(BT_MDM_CON3, 16, 10, 0);     // psk amplitude q channel
    SFR(BT_MDM_CON4, 0,  10, 0);      // fsk amplitude i channel
    SFR(BT_MDM_CON4, 16, 10, 0);     // fsk amplitude q channel

    SFR(BT_MDM_CON5, 0, 10, 0);        // tx offset i channel
    SFR(BT_MDM_CON5, 16, 10, 0);       // tx offset q channel

    SFR(BTA_CON8,  1,  1,  0);      // EN_TRIM
    SFR(BT_MDM_CON1, 27, 1, 1);     // analog test enable(for testing only)

    delay(sys_clk*50);
    dat = 0;

   while(1)
   {
        SFR(BTA_CON2, 11 , 3, dat);       //LNAV_RSEL0_12v
        delay(sys_clk*100);
        printf("---------%d - %d \n", BTA_CON16 & 0X1, dat);
        if(BTA_CON16 & 0X1)      //LNAV_O
        {
           dat += 1;
           if(dat > 7)
           {
                dat = 0;
                printf("\n\n\n bt_rf iq trim error !!! \n\n\n");
                SFR(BT_MDM_CON3, 0,  10, 256);      // psk amplitude i channel
                SFR(BT_MDM_CON3, 16, 10, 256);      // psk amplitude q channel
                SFR(BT_MDM_CON4, 0,  10, 256);      // fsk amplitude i channel
                SFR(BT_MDM_CON4, 16, 10, 256);      // fsk amplitude q channel

                SFR(BT_MDM_CON5, 0,  10, 0);        // tx offset i channel
                SFR(BT_MDM_CON5, 16, 10, 0);       // tx offset q channel
                break ;
           }
        }
        else
        {
            if( trim_iq(256) )
            {
                break;
            }
            else
            {
                dat += 1;
                if(dat > 7)
                {
                    dat = 0;
                    printf("\n\n\n bt_rf iq trim error !!! \n\n\n");
                    SFR(BT_MDM_CON3, 0,  10, 256);      // psk amplitude i channel
                    SFR(BT_MDM_CON3, 16, 10, 256);      // psk amplitude q channel
                    SFR(BT_MDM_CON4, 0,  10, 256);      // fsk amplitude i channel
                    SFR(BT_MDM_CON4, 16, 10, 256);      // fsk amplitude q channel

                    SFR(BT_MDM_CON5, 0,  10, 0);        // tx offset i channel
                    SFR(BT_MDM_CON5, 16, 10, 0);       // tx offset q channel
                    break ;
                }
                SFR(BT_MDM_CON3, 0,  10, 0);     // psk amplitude i channel
                SFR(BT_MDM_CON3, 16, 10, 0);     // psk amplitude q channel
                SFR(BT_MDM_CON4, 0,  10, 0);     // fsk amplitude i channel
                SFR(BT_MDM_CON4, 16, 10, 0);     // fsk amplitude q channel
                SFR(BT_MDM_CON5, 0,  10, 0);     // tx offset i channel
                SFR(BT_MDM_CON5, 16, 10, 0);     // tx offset q channel
            }
        }
   }

   SFR(BTA_CON0,  1,  1,  0);     // RX_LDO
   SFR(BTA_CON1, 15,  1,  0);     // LNAV_EN_12v
  //SFR(BTA_CON8,  1,  1,  1);    // EN_TRIM
   SFR(BT_MDM_CON1, 27, 1, 0);    // analog test enable(for testing only)
}


#define  RCCL_ALL     99

static void bt_rccl_trim(void)
{
    unsigned char cnt[64], cnt_all, buf, bw_set;
    unsigned short i;

    //rccl initial
    SFR(BTA_CON0, 1 , 1, 1);       //RXLDO_EN_12v
    SFR(BTA_CON3, 5 ,  1, 1);      //LNA_MIX_EN_12v
    SFR(BTA_CON2, 6 , 5, 4);       //BG_ISEL0_12v
    SFR(BTA_CON4, 5 ,  2, 1);      //LDOISEL0_12v
    SFR(BTA_CON0, 8 , 2, 0);       //VREF_S_12v 正常工作设置为1
    SFR(BTA_CON1, 10, 5, 0X10);    //ADC_VCM_S_12v


    SFR(BTA_CON14, 8 ,  2, 1);     //RCCL_DIV_SEL0_12v
    SFR(BTA_CON14, 10 ,  1, 1);     //RCCL_EN_12v
    SFR(BTA_CON14, 13 , 1, 0);     //RCCL_GO_12v
    SFR(BTA_CON14, 14 , 2, 1);       //bt_rccl_sel 0: 1'b1, 1:pll_24m 2:hsb_clk 3:pll_96

    delay(sys_clk*5000);           // 100 ms

    for(cnt_all = 0; cnt_all < 64; cnt_all++)
    {
        cnt[cnt_all] = 0;
    }
    for(cnt_all = 0; cnt_all < RCCL_ALL; cnt_all++)
    {
         SFR(BTA_CON14, 13 , 1, 0);     //RCCL_GO_12v
         delay(sys_clk*50);             // 100 us
         SFR(BTA_CON14, 13 , 1, 1);     //RCCL_GO_12v

         i = 0;
         while((BTA_CON16>>7)&0x01);   // det_vo:0
         {
             i++;
             if(i > sys_clk*100)
             {
                 printf("\n\n\n  bt_rccl_trim error !!! \n\n\n");
                 break;
             }
         }

         if(!((BTA_CON16>>7)&0x01))      // det_vo:0
         {
             buf = (BTA_CON16>>1)&0x3f;  // rccl_out
             cnt[buf] += 1;
           //  printf("\n0x%x", buf);
         }
    }

    buf = 0;
    bw_set = 0;
    for(cnt_all = 0; cnt_all < 64; cnt_all++)
    {
        if( buf < cnt[cnt_all] )
        {
            buf = cnt[cnt_all];
            bw_set = cnt_all;
        }
    }
    printf("\nbw_set: 0x%x  %d \n", bw_set, cnt[bw_set]);

    SFR(BTA_CON3, 6 ,  6, (bw_set&0x3f));    //BP_BW0_12v
    SFR(BTA_CON14, 10 ,  1, 0);     //RCCL_EN_12v
    SFR(BTA_CON14, 13 , 1, 0);     //RCCL_GO_12v
    SFR(BTA_CON14, 14 , 2, 0);     //bt_rccl_sel 0: 1'b1, 1:pll_24m 2:hsb_clk 3:pll_96
}

#define  scan_channel_l   74
#define  scan_channel_h   163
#define  sys_clk          64

unsigned char bta_pll_bank_limit[15];

static unsigned char bta_pll_bank_set(unsigned char bank)
{
    unsigned char set;
    switch (bank)
    {
        case 0: set = 0x3f; break;
        case 1: set = 0x37; break;
        case 2: set = 0x33; break;
        case 3: set = 0x31; break;
        case 4: set = 0x30; break;
        case 5: set = 0x1f; break;
        case 6: set = 0x17; break;
        case 7: set = 0x13; break;
        case 8: set = 0x11; break;
        case 9: set = 0x10; break;
        case 10: set = 0xf; break;
        case 11: set = 0x7; break;
        case 12: set = 0x3; break;
        case 13: set = 0x1; break;
        case 14: set = 0x0; break;
        default : ;
    }
    /* SFR(BTA_CON10, 5 ,  6, set);     //RLL_CAPS_12v */
    return set;
}


static void bta_pll_bank_scan(unsigned char set_freq)  //n * kHz
{
    unsigned long  core_freq, j;
    unsigned char i, dat8;
    unsigned char  dat_l, dat_h, dat_buf, dat_buf1;

    SFR(BTA_CON14, 12 ,  1, 1);    //PLL_VC_DETEN_12v
    dat_l = scan_channel_l;
    dat_h = scan_channel_l;
    dat_buf1 = scan_channel_l;

    for(i = 0; i< 15; i++)
    {
        dat8 = bta_pll_bank_set(i);
        SFR(BTA_CON10, 5 ,  6, dat8);  //pLL_CAPS_12v
        printf("\nbak: %x %d ", dat8, i);

        dat_buf = dat_l;
        while(1)
        {
            SFR(BTA_CON11, 0 , 8, dat_buf);   //PLL_FBDS0_12v /// 2450 = 2321 + 129  (2402 - 2480) : (81 159
          //  printf("\n%d", dat16_buf);
            SFR(BTA_CON11, 15 , 1, 0);     //PLL_RN_12v         //70 us
            delay(sys_clk*5);                    //    > 5us
            SFR(BTA_CON11, 15 , 1, 1);     //PLL_RN_12v         //70 us
            delay(sys_clk*80);
            dat8 = 0;
            dat8 |= (BTA_CON16 >> 8) & 3;   //[l:h]
            dat8 |= (BTA_CON16 >> 8) & 3;
            dat8 |= (BTA_CON16 >> 8) & 3;   //[l:h]

            if(dat8 == 1) //FMPLL_VCZH_12v
            {
                dat_h = dat_buf;
                break ;
            }
            else if(dat8 == 2) //FMPLL_VCZL_12v
            {
                dat_l = dat_buf;
                dat_buf += 1;
                if(dat_buf > scan_channel_h)
                {
                    dat_buf = scan_channel_h;
                    dat_l = scan_channel_h;
                    dat_h = scan_channel_h;
                    break;
                }
            }
            else
            {
                dat_buf += 1;
                if(dat_buf > scan_channel_h)
                {
                    dat_buf = scan_channel_h;
                    dat_h = scan_channel_h;
                    break;
                }
            }
        }

        bta_pll_bank_limit[i] = (dat_buf1 + dat_l)/2;
        dat_buf1 = dat_h;
        printf("[%d : %d] %d", dat_l, dat_h, bta_pll_bank_limit[i]);
    }

    SFR(BTA_CON11, 0 , 8, set_freq);   //PLL_FBDS0_12v /// 2450 = 2321 + 129  (2402 - 2480) : (81 159
    dat8 =  get_bta_pll_bank(set_freq);
    SFR(BTA_CON10, 5 ,  6, dat8);     //RLL_CAPS_12v
    printf("\nsetbak:%x",dat8);

   SFR(BTA_CON14, 12 ,  1, 0);    //PLL_VC_DETEN_12v
}

static void bta_low_power_init(void)
{
//******** rx **********
    SFR(BTA_CON1, 6 , 2, 3);      //ADC_ISEL_12v        //low_p 3
    SFR(BTA_CON3, 2 ,  3, 3);       //LNA_ISEL0_12v     //low_p 3
    SFR(BTA_CON4, 7 ,  4, 2);       //DMIX_ADJ0_12v     //low_p 2
//******** tx **********
    SFR(BTA_CON5, 6 ,  3, 4);       //DAC_GAIN_SET0_12v //low_p 4
    SFR(BTA_CON8, 14 , 2, 2);       //MIXER_GISEL0_12v  //low_p 2
    SFR(BTA_CON10, 0 , 4, 0XE);     //PA_BIAS_S0_12v    //low_p 0xe
}

static void bta_normal_init(void)
{
//******** rx **********
    SFR(BTA_CON1, 6 , 2, 1);      //ADC_ISEL_12v        //low_p 3
    SFR(BTA_CON3, 2 ,  3, 6);       //LNA_ISEL0_12v     //low_p 3
    SFR(BTA_CON4, 7 ,  4, 1);       //DMIX_ADJ0_12v     //low_p 2
//******** tx **********
    SFR(BTA_CON5, 6 ,  3, 6);       //DAC_GAIN_SET0_12v //low_p 4
    SFR(BTA_CON8, 14 , 2, 3);       //MIXER_GISEL0_12v  //low_p 2
    SFR(BTA_CON10, 0 , 4, 0Xf);     //PA_BIAS_S0_12v    //low_p 0xe
}

#define BTOSC_CLK   12

static void bt_analog_init(void)
{
    SFR(BTA_CON0, 0 , 1, 1);      //LDO_EN_12v
    bt_rccl_trim();
//	SFR(BTA_CON3, 6 ,  6, 0X20);    //BP_BW0_12v
//*****RX***********
    SFR(BTA_CON0, 1 , 1, 0);      //RXLDO_EN_12v
    SFR(BTA_CON0, 2 , 2, 2);      //RX_O_EN_S_12v  1:DM  2:BP
    SFR(BTA_CON0, 4 , 1, 1);      //TB_EN_12v
    SFR(BTA_CON0, 5 , 3, 1);      //TB_SEL_12v     1xx=TX; 0x0=DM; 0x1=BP
    SFR(BTA_CON0, 8 , 2, 2);      //VREF_S_12v

    SFR(BTA_CON0, 10, 5, 0X10);   //ADC_C_SEL_12v   04

    SFR(BTA_CON0, 15, 1, 1);      //ADC_DEM_EN_12v
    SFR(BTA_CON1, 0 , 1, 1);      //ADC_EN_12v

    SFR(BTA_CON1, 1 , 5, 0XF);    //ADC_IDAC_S_12v
    SFR(BTA_CON1, 6 , 2, 3);        //ADC_ISEL_12v      //normal 1

    SFR(BTA_CON1, 8 , 2, 2);      //ADC_LDO_S_12v
    SFR(BTA_CON1, 10, 5, 0X10);   //ADC_VCM_S_12v

    SFR(BTA_CON2, 0 , 3, 1);        //AVDD1_S_12v
    SFR(BTA_CON2, 3 , 3, 1);        //AVDDFE_S_12v

    SFR(BTA_CON2, 6 , 5, 1);        //BG_ISEL0_12v

    SFR(BTA_CON1, 15 , 1, 0);       //LNAV_EN_12v
    SFR(BTA_CON2, 11 , 3, 0);       //LNAV_RSEL0_12v
    SFR(BTA_CON3, 0 ,  2, 1);       //LNA_GSEL0_12v
    SFR(BTA_CON3, 2 ,  3, 3);       //LNA_ISEL0_12v     //normal 6
    SFR(BTA_CON3, 5 ,  1, 1);       //LNA_MIX_EN_12v

    SFR(BTA_CON2, 14 , 2, 1);       //BP_AVDD_S0_12v
    
    SFR(BTA_CON3, 12 , 4, 0XF);     //BP_GA0_12v
    SFR(BTA_CON4, 0 ,  1, 1);       //BP_EN_12v

    SFR(BTA_CON4, 1 ,  2, 0);       //BP_ISEL0_12v

    SFR(BTA_CON4, 3 ,  1, 1);       //IQ_BIAS_ISEL_12v
    SFR(BTA_CON4, 4 ,  1, 1);       //LO_BIAS_ISEL_12v

    SFR(BTA_CON4, 5 ,  2, 1);       //LDOISEL0_12v
    SFR(BTA_CON4, 7 ,  4, 2);       //DMIX_ADJ0_12v     //normal 1

    SFR(BTA_CON4, 11 , 1, 1);       //DMIX_BIAS_SEL_12v
    SFR(BTA_CON4, 12 , 2, 3);       //DMIX_ISEL1_12v
    SFR(BTA_CON5, 0 ,  3, 6);       //DMIX_R_S0_12v

    //*****TX*************
    SFR(FMA_CON1, 8, 1, 1);   // FMLDO_EN_12v  
    SFR(BTA_CON5, 3 ,  3, 0);       //DAC_BIAS_SET0_12v
    SFR(BTA_CON5, 6 ,  3, 4);       //DAC_GAIN_SET0_12v //normal 6
    SFR(BTA_CON5, 9 ,  5, 0X10);    //DAC_VCM_S0_12v
    SFR(BTA_CON8, 0 ,  1, 1);       //TXDAC_EN_12v
 /*
    SFR(BTA_CON6, 0 ,  6, 1);       //IL_TRIM0_12v
    SFR(BTA_CON6, 6 ,  6, 1);       //IR_TRIM0_12v
    SFR(BTA_CON7, 0 ,  6, 1);       //QL_TRIM0_12v
    SFR(BTA_CON7, 6 ,  6, 1);       //QR_TRIM0_12v
    SFR(BTA_CON4, 14 , 2, 1);       //TRIM_CUR0_12v
    SFR(BTA_CON7, 13 , 1, 1);       //EN_TRIMI_12v
    SFR(BTA_CON7, 14 , 1, 1);       //EN_TRIMQ_12v
  */
    SFR(BTA_CON8, 3 , 1, 0);        //EN_TRIM_12v
    SFR(BTA_CON5, 14 , 2, 1);       //TXDAC_LDO_S0_12v
    SFR(BTA_CON6, 12 , 2, 1);       //TXIQ_LDO_S0_12v
    SFR(BTA_CON6, 14 , 2, 1);       //UMIX_LDO_S0_12v

    SFR(BTA_CON7, 12 , 1, 1);       //TXLDO_EN_12v
    SFR(BTA_CON7, 15 , 1, 0);       //TX_EN_TEST_12v
    SFR(BTA_CON8, 1 , 2, 1);        //TX_IQ_BIAS_ISEL_12v

    SFR(BTA_CON8, 4 ,  4, 0X7);       //MIXER_LCSEL0_12v
    SFR(BTA_CON8, 8 ,  2, 0);       //MIXER_RCSEL0_12v
    SFR(BTA_CON8, 10 , 4, 2);       //UMLO_BIAS_S0_12v
    SFR(BTA_CON8, 14 , 2, 3);       //MIXER_GISEL0_12v  //normal 3

    SFR(BTA_CON9, 0 ,  1, 1);       //EN_TXBIAS_12v
    SFR(BTA_CON9, 1 ,  1, 1);       //EN_TXIQ_12v
    SFR(BTA_CON9, 2 ,  1, 1);       //EN_UM_12v

    SFR(BTA_CON9, 3 ,  7, 0X4F);    //PA_CSEL0_12v
    SFR(BTA_CON9, 10 ,  5, 0XF);     //PA_G_SEL0_12v

    SFR(BTA_CON10, 0 , 4, 0XF);     //PA_BIAS_S0_12v    //normal 0xf
    SFR(BTA_CON10, 4 , 1, 1);       //EN_PA_12v
    SFR(BTA_CON14, 8 ,  2, 1);     //RCCL_DIV_SEL0_12v
    SFR(BTA_CON14, 10 , 1, 0);     //RCCL_EN_12v
    SFR(BTA_CON14, 13 , 1, 0);     //RCCL_GO_12v
    SFR(BTA_CON14, 14 , 2, 1);       //bt_rccl_sel 0: 1'b1, 1:pll_24m 2:hsb_clk 3:pll_96
    //SFR(BTA_CON16, 0 , 1, 1);      //LNAV_O_12v
    //SFR(BTA_CON16, 1 , 1, 1);      //RCCL_BW0_12v
    //SFR(BTA_CON16, 2 , 1, 1);      //RCCL_BW1_12v
    //SFR(BTA_CON16, 3 , 1, 1);      //RCCL_BW2_12v
    //SFR(BTA_CON16, 4 , 1, 1);      //RCCL_BW3_12v
    //SFR(BTA_CON16, 5 , 1, 1);      //RCCL_BW4_12v
    //SFR(BTA_CON16, 6 , 1, 1);      //RCCL_BW5_12v
    //SFR(BTA_CON16, 7 , 1, 1)       //RCAL_DET_12v



//********************** PLL *********************//
    SFR(BTA_CON13, 5 ,  5, 0);     //XOSC_CLS0_12v
    SFR(BTA_CON13, 10 , 5, 0);     //XOSC_CRS0_12v
    SFR(BTA_CON13, 15 , 1, 1);     //XOSC_EN_12v
    SFR(BTA_CON14, 0 ,  1, 0);     //XOSC_TEST_12v
    SFR(BTA_CON14, 5 ,  1, 1);     //XCK_BT_OE_12v
    SFR(BTA_CON14, 6 ,  1, 1);     //XCK_FM_OE_12v
    SFR(BTA_CON14, 7 ,  1, 1);     //XCK_SYS_OE_12v

    if(BTOSC_CLK > 12)
    {
      SFR(BTA_CON14, 1, 4,  4); // BTOSC_Hcs
    }
    else
    {
      SFR(BTA_CON14, 1, 4,  2); // BTOSC_Hcs
    }

    SFR(BTA_CON12, 2 ,  5, (BTOSC_CLK/2-2));     //PLL_REFS0_12v
    SFR(BTA_CON14, 11 ,  1, 0);    //PLL_REFCLKSEL_12v : 0 BT 1:RT

    SFR(BTA_CON12, 8 ,  2, 0);     //PLL_BIAS_ADJ0_12v
    SFR(BTA_CON11, 12 , 3, 4);     //PLL_IVCOS0_12v
    SFR(BTA_CON12, 0 ,  2, 1);     //PLL_PFD0_12v
    SFR(BTA_CON10, 11 , 2, 2);     //PLL_ICP0_12v
    SFR(BTA_CON10, 13 , 2, 0);     //PLL_LPF0_12v
    SFR(BTA_CON14, 12 ,  1, 0);    //PLL_VC_DETEN_12v
    SFR(BTA_CON10, 15 , 1, 0);     //PLL_VC_DRVEN_12v
    SFR(BTA_CON11, 8 ,  3, 1);     //PLL_VC_S0_12v
    SFR(BTA_CON11, 11 , 1, 0);     //PLL_VC_OE_12v
    SFR(BTA_CON10, 5 ,  6, 0x13);  //RLL_CAPS_12v
    SFR(BTA_CON11, 0 ,  8, 129);   //PLL_FBDS0_12v /// 2450 = 2321 + 129  (2402 - 2480) : (81 159)

    SFR(BTA_CON12, 10 , 3, 2);     //PLL_TEST_S0_12v
    SFR(BTA_CON12, 13 , 1, 0);     //PLL_TEST_OE_12v

    SFR(BTA_CON12, 14 , 2, 1);     //PLL_VMS0_12v
    SFR(BTA_CON13, 0 ,  1, 0);     //PLL_DETEN_12v
    SFR(BTA_CON13, 1 ,  2, 0);     //PLL_DETS0_12v

    SFR(BTA_CON13, 3 ,  1, 0);     //PLL_CKO75M_OE_12v
    SFR(BTA_CON12, 7 ,  1, 1);     //PLL_BIAS_EN_12v

    SFR(BTA_CON11, 15 , 1, 0);     //PLL_RN_12v
    SFR(BTA_CON9, 15 ,  1, 1);     //PLL_EN_12v

    delay(sys_clk*5);                    //    > 5us
    SFR(BTA_CON11, 15 , 1, 1);     //PLL_RN_12v         //70 us
    delay(sys_clk*80);

    bta_pll_bank_scan(129);

    trim_iq_all(); 

    SFR(BTA_CON0, 1 , 1, 0);      //RXLDO_EN_12v
    SFR(BTA_CON7, 12 , 1, 1);     //TXLDO_EN_12v

    //SFR(BTA_CON14, 12 ,  1, 0);    //PLL_VC_DETEN_12v
    //SFR(BTA_CON16, 8 ,  1, 1);     //PLL_VCZH_12v
    //SFR(BTA_CON16, 9 ,  1, 1);     //PLL_VCZL_12v
    //SFR(BTA_CON16, 10 , 1, 1);     //PLL_DETOUT_12v
    printf("\n\n br16 analog  test  ......\n\n");
    // while(1);
}


static void baseband_analog(char en)
{
  /*struct ble_param *ble_fp;*/
  /*ble_fp = (struct ble_param *) (BLE_EXM_BASE_ADR + 0x100);*/
    if(en == 0)
    {
        //********************** BASEBAND CONTROL ENABLE *********************//
        SFR(BTA_CON15, 0 , 1, 0);       //bt_ldo_sel
        SFR(BTA_CON15, 1 , 1, 0);       //bt_pllen_sel
        SFR(BTA_CON15, 2 , 1, 0);       //bt_pllb_sel
        SFR(BTA_CON15, 3 , 1, 0);       //bt_pllrn_sel
        SFR(BTA_CON15, 4 , 1, 0);       //bt_pll_sel
        SFR(BTA_CON15, 5 , 1, 0);       //bt_rxbp_sel
        SFR(BTA_CON15, 6 , 1, 0);       //bt_dmix_sel
        SFR(BTA_CON15, 7 , 1, 0);       //bt_txpa_sel
        SFR(BTA_CON15, 8 , 1, 0);       //bt_umix_sel
        SFR(BTA_CON15, 9 , 1, 0);       //bt_dac_sel
        SFR(BTA_CON15, 10 ,1, 0);       //bt_rxtx_sel
        SFR(BTA_CON15, 11 ,1, 0);       //bt_btosc_sel
        SFR(BTA_CON15, 12 ,1, 0);       //bt_rxen_sel
        SFR(BTA_CON15, 13 ,1, 0);       //bt_txen_sel
        SFR(BTA_CON15, 14 ,1, 0);       //bt_ldo_en_sel
    }
    else
    {
        //********************** BASEBAND CONTROL ENABLE *********************//
        SFR(BTA_CON15, 0 , 1, 1);       //bt_ldo_sel
        SFR(BTA_CON15, 1 , 1, 1);       //bt_pllen_sel
        SFR(BTA_CON15, 2 , 1, 1);       //bt_pllb_sel
        SFR(BTA_CON15, 3 , 1, 1);       //bt_pllrn_sel
        SFR(BTA_CON15, 4 , 1, 1);       //bt_pll_sel
        SFR(BTA_CON15, 5 , 1, 1);       //bt_rxbp_sel
        SFR(BTA_CON15, 6 , 1, 1);       //bt_dmix_sel
        SFR(BTA_CON15, 7 , 1, 1);       //bt_txpa_sel
        SFR(BTA_CON15, 8 , 1, 1);       //bt_umix_sel
        SFR(BTA_CON15, 9 , 1, 1);       //bt_dac_sel
        SFR(BTA_CON15, 10 ,1, 1);       //bt_rxtx_sel
        SFR(BTA_CON15, 11 ,1, 0);       //bt_btosc_sel
        SFR(BTA_CON15, 12 ,1, 1);       //bt_rxen_sel
        SFR(BTA_CON15, 13 ,1, 1);       //bt_txen_sel
        SFR(BTA_CON15, 14 ,1, 0);       //bt_ldo_en_sel
    }
}


void bt_lo_freq(unsigned char set_freq )
{
    unsigned char dat8;

    SFR(BTA_CON11, 0 , 8, set_freq);   //PLL_FBDS0_12v /// 2450 = 2321 + 129  (2402 - 2480) : (81 159
    dat8 =  get_bta_pll_bank(set_freq);
    SFR(BTA_CON10, 5 ,  6, dat8);     //RLL_CAPS_12v

    SFR(BTA_CON0, 1 , 1, 0);      //RXLDO_EN_12v
    SFR(BTA_CON7, 12 , 1, 1);     //TXLDO_EN_12v

    SFR(BT_MDM_CON3, 0,  10, 0);      // psk amplitude i channel
    SFR(BT_MDM_CON3, 16, 10, 0);     // psk amplitude q channel
    SFR(BT_MDM_CON4, 0,  10, 0);      // fsk amplitude i channel
    SFR(BT_MDM_CON4, 16, 10, 0);     // fsk amplitude q channel
    SFR(BT_MDM_CON5, 0, 10, 256);        // tx offset i channel
    SFR(BT_MDM_CON5, 16, 10, 256);       // tx offset q channel

    SFR(BT_MDM_CON1, 27, 1, 1);        // analog test enable(for testing only)
}

void rf_debug()
{
    /* DIAGCNTL = (1<<7)|(5);//EN NUM */
    BT_EXT_CON |=BIT(1);  //DBG EN
    BT_EXT_CON |=BIT(5); //1:BLE 0:BREDR

    //设置debug输出口
    PORTB_DIR&=~0x0ff0;
    PORTB_DIR&=~BIT(0);
    PORTB_OUT&=~BIT(0);
}

void bt_rf_analog_init()
{
	baseband_analog(0);
	bt_analog_init();
	baseband_analog(1);
}

unsigned char get_bta_pll_bank(unsigned char set_freq)
{
    char i;

    for(i=1; i< 15; i++)
    {
        if(set_freq < bta_pll_bank_limit[i])
        {
            return bta_pll_bank_set(i-1);
        }
    }
}
