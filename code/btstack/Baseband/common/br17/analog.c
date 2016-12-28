#include "RF.h"
#include "cpu.h"
#include "typedef.h"

#define bt_printf(...)
/*
    bt trim vlaue deal
*/
u8 bt_trim_cfg;//[0]:data ok,no trim  [1]:trim always   [2]:data err,trim  [3]data null,trim and save
u8 bt_trim_error = 0;

/* void set_bt_trim_mode(u8 mode) */
/* { */
	/* //u8 tmp_buf[32]; */
    /* u32 bt_zone_addr; */

	/* bt_trim_error = 0; */
    /* bt_zone_addr = app_use_flash_cfg.flash_size - ((68+4)*1024); */
	/* //cache_read(tmp_buf,bt_zone_addr,32); */
	/* //puts("bt_trim_zone:");put_buf(tmp_buf,32); */
    /* if(mode){ */
        /* bt_puts("bt_trim_always\n"); */
        /* bt_trim_cfg = 1; */
    /* }else { */
        /* bt_puts("bt_trim_once\n"); */
        /* cache_read((u8 *)&trim_info, bt_zone_addr+8, sizeof(trim_info)); */
        /* printf("bw:0x%x     i_dc:%d      q_dc:%d\n",trim_info.bw_set,trim_info.i_dc,trim_info.q_dc); */
        /* if(trim_info.crc == CRC16((u8*)&trim_info+2,sizeof(trim_info)-2)){ */
            /* bt_puts("trim_info crc_ok\n"); */
            /* bt_trim_cfg = 0; */
        /* }else if((trim_info.crc != CRC16((u8*)&trim_info+2,sizeof(trim_info)-2)) && (trim_info.crc == 0xFFFF)){ */
            /* bt_puts("trim_info NULL\n"); */
            /* bt_trim_cfg = 3; */
        /* }else{ */
            /* bt_puts("trim_info crc_err!!!!!!!!!\n"); */
            /* //bt_trim_cfg = 2; */

			/* //erase cfg zone ,trim and save again */
			/* sfc_erase(SECTOR_ERASER,bt_zone_addr); */
            /* bt_trim_cfg = 3; */
        /* } */
    /* } */
/* } */
/* static u8 get_bt_trim_cfg(void) */
/* { */
    /* return bt_trim_cfg; */
/* } */
/* static void save_trim_value(void) */
/* { */
    /* u32 bt_zone_addr; */
    /* bt_puts("save_trim_value\n"); */
	/* if(bt_trim_error) { */
		/* printf("trim err:%d\n",bt_trim_error); */
		/* return; */
	/* } */
    /* bt_zone_addr = app_use_flash_cfg.flash_size - ((68+4)*1024); */
    /* trim_info.crc = CRC16((u8*)&trim_info+2, sizeof(trim_info)-2); */
    /* sfc_write((void *)&trim_info, bt_zone_addr+8,sizeof(struct rf_analog_trim)); */
/* } */

static void fski_apfda_set(unsigned short dat_set)
{
    unsigned short dat;

    dat = BT_MDM_CON4 & 0x3ff;

    if(dat > dat_set)
    {

        for( ; dat > dat_set; dat-- )
        {

            SFR(BT_MDM_CON4, 0,  10, dat);   // fsk amplitude i channel
        }
    }
    else
    {

       for( ; dat < dat_set; dat++ )
        {
            SFR(BT_MDM_CON4, 0,  10, dat);   // fsk amplitude i channel
        }
    }
    SFR(BT_MDM_CON4, 0,  10, dat);       // fsk amplitude i channel
}


static void fskq_apfda_set(unsigned short dat_set)
{
    unsigned short dat;

    dat = (BT_MDM_CON4 >>16) & 0x3ff;

    if(dat > dat_set)
    {
        for( ; dat > dat_set; dat-- )
        {
            SFR(BT_MDM_CON4, 16,  10, dat);   // fsk amplitude q channel
        }
    }
    else
    {
       for( ; dat < dat_set; dat++ )
        {
            SFR(BT_MDM_CON4, 16,  10, dat);   // fsk amplitude q channel
        }
    }
    SFR(BT_MDM_CON4, 16,  10, dat);   // fsk amplitude q channel
}


static void pski_apfda_set(unsigned short dat_set)
{
    unsigned short dat;

    dat = BT_MDM_CON3 & 0x3ff;

    if(dat > dat_set)
    {
        for( ; dat > dat_set; dat-- )
        {
            SFR(BT_MDM_CON3, 0,  10, dat);   // psk amplitude i channel
        }
    }
    else
    {
       for( ; dat < dat_set; dat++ )
        {
            SFR(BT_MDM_CON3, 0,  10, dat);   // psk amplitude i channel
        }
    }
    SFR(BT_MDM_CON3, 0,  10, dat);   // psk amplitude i channel
}


static void pskq_apfda_set(unsigned short dat_set)
{
    unsigned short dat;

    dat = (BT_MDM_CON3 >>16) & 0x3ff;

    if(dat > dat_set)
    {
        for( ; dat > dat_set; dat-- )
        {
            SFR(BT_MDM_CON3, 16,  10, dat);   // psk amplitude q channel
        }
    }
    else
    {
       for( ; dat < dat_set; dat++ )
        {
            SFR(BT_MDM_CON3, 16,  10, dat);   // psk amplitude q channel
        }
    }
    SFR(BT_MDM_CON3, 16,  10, dat);   // psk amplitude q channel
}


static void offseti_fda_set(short dat_set)
{
    short dat;

    dat = BT_MDM_CON5 & 0xffff;
    if(dat > dat_set)
    {
        for( ; dat > dat_set; dat-- )
        {
            SFR(BT_MDM_CON5, 0,  10, dat);   // tx offset i channel
        }
    }
    else
    {
       for( ; dat < dat_set; dat++ )
        {
            SFR(BT_MDM_CON5, 0,  10, dat);   // tx offset i channel
        }
    }
    SFR(BT_MDM_CON5, 0,  10, dat);   // tx offset i channel
}


static void offsetq_fda_set(short dat_set)
{
    short dat;

    dat = (BT_MDM_CON5 >>16) & 0xffff;

    if(dat > dat_set)
    {
        for( ; dat > dat_set; dat-- )
        {
            SFR(BT_MDM_CON5, 16,  10, dat);   // tx offset q channel
        }
    }
    else
    {
       for( ; dat < dat_set; dat++ )
        {
            SFR(BT_MDM_CON5, 16,  10, dat);   // tx offset q channel
        }
    }
    SFR(BT_MDM_CON5, 16,  10, dat);   // tx offset q channel
}

static unsigned char RF_trim_iq(short trim_w)
{
    short data_t,m,n,i_dc,q_dc, flag, i, dat_o, dat_o_d;

    for(i = 0; i<2; i++)
    {
        offseti_fda_set(0);
        offsetq_fda_set(0);
        flag = 1;
        m = trim_w;
        n = -trim_w;
        dat_o_d = 3;

        for(data_t = -trim_w; data_t <= trim_w ; data_t += 1 )
        {
            if(i== 0)
            {
                offseti_fda_set(data_t);  // DC_I_0

            }
            else if(i== 1)
            {
                offsetq_fda_set(data_t); // DC_Q_0
            }
             delay(rf.delay_sys*100);
             dat_o = 0;
             dat_o += WLA_CON30 & 0X1;
             dat_o += WLA_CON30 & 0X1;
             dat_o += WLA_CON30 & 0X1;

           // bt_printf("data: %d LNAV: %x %x\n", data_t, dat_o&0x1, dat_o_d);

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
         bt_printf("\n=========================");
         bt_printf("0>1n %d: 1>0M %d \n", n, m);
    }

    bt_printf("\ni_dc: %d q_dc: %d\n", i_dc, q_dc);


    offseti_fda_set(i_dc);  // tx offset i channel
    offsetq_fda_set(q_dc);  // tx offset q channel
    /* trim_info.i_dc = i_dc; */
    /* trim_info.q_dc = q_dc; */
    return 1;
}

static void RF_trim_iq_all(void)
{
   unsigned short dat;

    SFR(WLA_CON0, 1 , 1, 1);     //RXLDO_EN_12v
    SFR(WLA_CON7, 12, 1, 1);     //TXLDO_EN_12v
    SFR(WLA_CON1, 15, 1, 1);     //LNAV_EN_12v
    SFR(WLA_CON2, 15, 1, 1);     //LNA_MIX_EN_12v
    SFR(WLA_CON3,  2, 3, 4);     //LNA_ISEL_12v
    SFR(WLA_CON8,  5, 1, 0);     // EN_TRIM

    fski_apfda_set(0);
    fskq_apfda_set(0);
    pski_apfda_set(0);
    pskq_apfda_set(0);
    offseti_fda_set(0);
    offsetq_fda_set(0);

  delay(rf.delay_sys*50);
  dat = 0;
   while(1)
   {
        SFR(WLA_CON2,  11, 4, dat);       //LNAV_RSEL0_12v
        delay(rf.delay_sys*100);
        bt_printf("---------%d - %d \n", WLA_CON30 & 0X1, dat);
        if(WLA_CON30 & 0X1)      //LNAV_O
        {
           dat += 1;
           if(dat > 0xf)
           {
                dat = 0;
                bt_printf("\n\n\n bt_rf iq trim error !!! \n\n\n");
                fski_apfda_set(256);
                fskq_apfda_set(256);
                pski_apfda_set(256);
                pskq_apfda_set(256);
                offseti_fda_set(0);
                offsetq_fda_set(0);
                break ;
           }
        }
        else
        {
            if( RF_trim_iq(256) )
            {
                fski_apfda_set(256);
                fskq_apfda_set(256);
                pski_apfda_set(256);
                pskq_apfda_set(256);
                break;
            }
            else
            {
                dat += 1;
                if(dat > 0xf)
                {
                    dat = 0;
                    bt_printf("\n\n\n bt_rf iq trim error !!! \n\n\n");
					bt_trim_error = 2;
                    fski_apfda_set(256);
                    fskq_apfda_set(256);
                    pski_apfda_set(256);
                    pskq_apfda_set(256);
                    offseti_fda_set(0);
                    offsetq_fda_set(0);
                    break ;
                }
                fski_apfda_set(0);
                fskq_apfda_set(0);
                pski_apfda_set(0);
                pskq_apfda_set(0);
                offseti_fda_set(0);
                offsetq_fda_set(0);
            }
        }
   }

   SFR(WLA_CON0,  1,  1,  0);    // RX_LDO
   SFR(WLA_CON1, 15,  1,  0);    // LNAV_EN_12v
   SFR(WLA_CON8,  5,  1,  0);    // EN_TRIM
   SFR(BT_MDM_CON1, 27, 1, 0);   // analog test enable(for testing only)
}

/* void bt_iq_trim() */
/* { */
    /* if(get_bt_trim_cfg() == 0) { */
		/* bt_printf("bt_trim_cfg,i_dac=%d,q_dc=%d\n",trim_info.i_dc,trim_info.q_dc); */
        /* offseti_fda_set(trim_info.i_dc);  // tx offset i channel */
        /* offsetq_fda_set(trim_info.q_dc);  // tx offset q channel */
		/* return; */
	/* }	 */
	/* bt_puts("bt_iq_trim\n"); */
	/* struct ctrl_frame * frame = bd_get_frame(); */
	/* int index = (frame->txtog & BIT(0))?1:0; */
	/* bd_set_frame(frame); */

	/* u8 *p = (u8*)bd_malloc(ACL_SIZE); */
     /* writew((u16*)&frame->txptr0+index,PHY_TO_RADIO(p));	 */
     /* writew((u16*)&frame->txphdr0+index,(10<<3)|(1<<2)|1);	 */
	/* __write_reg_format(frame,0,MASTER_CONNECT); */
	/* __write_reg_linkcntl(frame,index,BIT(14)|BIT(4)|DM1); */
	/* bd_free(p); */
	/* RFTESTCNTL |= (1<<2); */
	/* delay_2slot_rise(1); */
	/* radio_set_eninv(1); */
	/* delay_2slot_rise(5); */

	/* RF_analog_init(0); */
	/* delay(10000); */
	/* RF_trim_iq_all(); */
	/* RFTESTCNTL &= ~(1<<2); */
	/* RF_analog_init(1); */
	
	/* if(get_bt_trim_cfg() == 3) { */
		/* save_trim_value(); */
	/* } */
/* } */

#define  RCCL_ALL     99

static void bt_rccl_trim(void)
{
    unsigned char cnt[64], cnt_all, buf, bw_set;
    unsigned short i;

    //rccl initial
    SFR(WLA_CON0, 0 , 1, 1);      //LDO_EN_12v
    SFR(WLA_CON0, 1 , 1, 1);      //RXLDO_EN_12v
    SFR(WLA_CON2, 15, 1, 1);      //LNA_MIX_EN_12v
    SFR(WLA_CON2, 6 , 5, 4);      //BG_ISEL0_12v
    SFR(WLA_CON4, 8 , 2, 1);      //LDOISEL0_12v
    SFR(WLA_CON0, 8 , 2, 0);      //VREF_S_12v 正常工作设置为1
    SFR(WLA_CON1, 10, 5, 0X10);   //ADC_VCM_S_12v

    SFR(WLA_CON18, 8 ,  2, 1);    //RCCL_DIV_SEL0_12v
    SFR(WLA_CON18, 10,  1, 1);    //RCCL_EN_12v
    SFR(WLA_CON18, 12,  1, 0);    //RCCL_GO_12v
    SFR(WLA_CON18, 14,  2, 1);    //bt_rccl_sel 0: 1'b1, 1:pll_24m 2:hsb_clk 3:pll_96

    delay(rf.delay_sys*5000);           // 100 ms

    for(cnt_all = 0; cnt_all < 64; cnt_all++)
    {
        cnt[cnt_all] = 0;
    }


    for(cnt_all = 0; cnt_all < RCCL_ALL; cnt_all++)
    {

         SFR(WLA_CON18, 12 , 1, 0);     //RCCL_GO_12v
         delay(rf.delay_sys*50);             // 100 us
         SFR(WLA_CON18, 12 , 1, 1);     //RCCL_GO_12v

         i = 0;
         while((WLA_CON30>>7)&0x01)   // det_vo:0
         {
             i++;
             if(i > rf.delay_sys*100)
             {
                 bt_printf("\n\n\n  bt_rccl_trim error !!! \n\n\n");
                 break;
             }
         }

         if(!((WLA_CON30>>7)&0x01))      // det_vo:0
         {
             buf = (WLA_CON30>>1)&0x3f;  // rccl_out
             cnt[buf] += 1;
            // bt_printf("\n0x%x", buf);
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
    bt_printf("\nbw_set: 0x%x  %d \n", bw_set, cnt[bw_set]);
	if((bw_set > 0x2A) || (bw_set <= 0x18)) {
		bt_printf("bt_trim_error---1\n");
		bt_trim_error = 1;		
		bw_set = 0x20;
	}
	/* trim_info.bw_set = bw_set;	 */
    SFR(WLA_CON3,  9,  6, (bw_set&0x3f));    //BP_BW0_12v
    SFR(WLA_CON18, 10 ,  1, 0);     //RCCL_EN_12v
    SFR(WLA_CON18, 12 , 1, 0);     //RCCL_GO_12v
    SFR(WLA_CON18, 14 , 2, 0);     //bt_rccl_sel 0: 1'b1, 1:pll_24m 2:hsb_clk 3:pll_96
}

#define  scan_channel_l   74
#define  scan_channel_h   163
#define  sys_clk          64

static unsigned char bta_pll_bank_set(unsigned char bank)
{
    return 0x7f - bank;
}

unsigned char get_bta_pll_bank(unsigned char set_freq)
{
    int i;

    for(i=1; i< 16*2; i++)
    {
        if(set_freq < rf.bta_pll_bank_limit[i])
        {
            return bta_pll_bank_set(i-1);
        }
    }
    return 0;
}


static void bta_pll_bank_scan(unsigned char set_freq)  //n * kHz
{
  //  unsigned long j;
    unsigned char i, dat8;
    unsigned char  dat_l, dat_h, dat_buf, dat_buf1;

    SFR(WLA_CON12, 6,  1,  1);     //PLL_VC_DETEN_12v
    dat_l = scan_channel_l;
    dat_h = scan_channel_l;
    dat_buf1 = scan_channel_l;
    for(i = 0; i< 16*2; i++)
    {
        dat8 = bta_pll_bank_set(i);
        SFR(WLA_CON11, 0,  8,  dat8);  //pLL_CAPS_12v
        bt_printf("\nbak: %x %d ", dat8, i);

        dat_buf = dat_l;
        while(1)
        {
            SFR(WLA_CON11, 8 , 8, dat_buf);   //PLL_FBDS0_12v /// 2450 = 2321 + 129  (2402 - 2480) : (81 159
          //  bt_printf("\n%d", dat16_buf);
            SFR(WLA_CON12, 15 , 1, 0);     //PLL_RN_12v         //70 us
            delay(rf.delay_sys*5);                    //    > 5us
            SFR(WLA_CON12, 15 , 1, 1);     //PLL_RN_12v         //70 us
            delay(rf.delay_sys*100);
            dat8 = 0;
            dat8 |= (WLA_CON30 >> 8) & 3;   //[l:h]
            dat8 |= (WLA_CON30 >> 8) & 3;
            dat8 |= (WLA_CON30 >> 8) & 3;   //[l:h]

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

        rf.bta_pll_bank_limit[i] = (dat_buf1 + dat_l + 3)/2;
        dat_buf1 = dat_h;
        bt_printf("[%d : %d] %d", dat_l, dat_h, rf.bta_pll_bank_limit[i]);
    }

    SFR(WLA_CON11, 8 , 8, set_freq);   //PLL_FBDS0_12v /// 2450 = 2321 + 129  (2402 - 2480) : (81 159
    dat8 =  get_bta_pll_bank(set_freq);
    SFR(WLA_CON11, 0 ,  8, dat8);     //RLL_CAPS_12v
    bt_printf("\nsetbak:%x",dat8);

   SFR(WLA_CON12, 6,  1,  0);     //PLL_VC_DETEN_12v
}

static void bt_pll_init(void)
	
{
    /* struct packet_param * packet_ptr; */
    /* packet_ptr = (struct packet_param *) FRAME0_CPU; */

    /* packet_ptr->TX_PWER   = 0; */
    /* packet_ptr->TX_SET  = 0; */
    /* packet_ptr->TX_PWER  |= 0X1F;      //[4:0] PA_GSEL */
    /* packet_ptr->TX_PWER  |= (0XB<5);     //[8:5] MIXER_LCSEL */
    /* packet_ptr->TX_PWER  |= (2<9);     //[10:9] MIXER_GISEL */
    /* packet_ptr->TX_PWER  |= (0<11);    //[13:11] DAC_BIAS_SET */
    /* packet_ptr->TX_PWER  |= (6<14);    //[16:14] DAC_GAIN_SET */
    /* packet_ptr->TX_SET   |= (0X1F);    //[6:0] PA_CSEL */
    /* packet_ptr->TX_SET   |= (0<<7);    //[7] LNA_GSEL1 */
    /* packet_ptr->TX_SET   |= (3<<8);    //[10:8] PLL_IVCOS */
    /* packet_ptr->TX_SET   |= (0X4<<11);  //[15:11] BG_ISEL */

    /* packet_ptr->RX_GAIN0  = 0; */
    /* packet_ptr->RX_SET    = 0; */
    /* packet_ptr->RX_GAIN0 |= (0XF);     //[3:0] BP_GA */
    /* packet_ptr->RX_GAIN0 |= (0<<4);    //[5:4] LNA_IADJ */
    /* packet_ptr->RX_GAIN0 |= (1<<8);    //[8]   LNA_GSEL0 */
    /* packet_ptr->RX_GAIN0 |= (5<<9);    //[11:9] LNA_ISEL */
    /* packet_ptr->RX_GAIN0 |= (6<<12);   //[14:12] DMIX_R_S */
    /* packet_ptr->RX_SET   |= (0X1F);    //[6:0] PA_CSEL */
    /* packet_ptr->RX_SET   |= (0<<7);    //[7] LNA_GSEL1 */
    /* packet_ptr->RX_SET   |= (3<<8);    //[10:8] PLL_IVCOS */
    /* packet_ptr->RX_SET   |= (0X4<<11);  //[15:11] BG_ISEL */

    /* packet_ptr->OSC_SET  = 0; */
    /* packet_ptr->OSC_SET |= (0);        //[4:0] xosc_cl */
    /* packet_ptr->OSC_SET |= (0<<5);     //[9:5] xosc_cr */

    /* packet_ptr->MDM_SET  = 0; */
    /* packet_ptr->MDM_SET |= (0);        //[11:0] FSK */
    /* packet_ptr->MDM_SET |= (0<<16);    //[25:16] PSK */

}
    

void bt_analog_init(void)
{
    /* SFR(BTA_CON0, 0 , 1, 1);      //LDO_EN_12v */
    bt_rccl_trim();
//======================= rx ===========================//
    SFR(WLA_CON0,  0,  1, 1);      //LDO_EN_12v
    SFR(WLA_CON0,  1,  1, 0);      //RXLDO_EN_12v
    SFR(WLA_CON0,  2,  2, 0);      //RX_O_EN_S_12v  1:dmixer 2:cbpf
    SFR(WLA_CON0,  4,  1, 0);      //TB_EN_12v
    SFR(WLA_CON0,  5,  3, 1);      //TB_SEL_12v       000:OUTA=IFI_P OUTB=IFQ_P;001:OUTA=voi OUTB=voq
    SFR(WLA_CON0,  8,  2, 1);      //VREF_S_12v

    SFR(WLA_CON0,  10, 5, 0x11);    //ADC_C_SEL_12v
    SFR(WLA_CON0,  15, 1, 0);      //ADC_DEM_EN_12v
    SFR(WLA_CON1,  0,  1, 1);      //ADC_EN_12v
    SFR(WLA_CON1,  1,  5, 0x7);    //ADC_IDAC_S_12v
    SFR(WLA_CON1,  6,  2, 0);      //ADC_ISEL_12v
    SFR(WLA_CON1,  8,  2, 1);      //ADC_LDO_S_12v
    SFR(WLA_CON1,  10, 5, 0x04);    //ADC_VCM_S_12v
    SFR(WLA_CON2,  0,  3, 1);      //AVDD1_S_12v
    SFR(WLA_CON2,  3,  3, 1);      //AVDDFE_S_12v
    SFR(WLA_CON2,  6,  5, 0x1);    //BG_ISEL_12v

    SFR(WLA_CON1,  15, 1, 0);      //LNAV_EN_12v
    SFR(WLA_CON2,  11, 4, 0x0);    //LNAV_RSEL_12v
    SFR(WLA_CON3,  0,  1, 1);      //LNA_GSEL_12v
    SFR(WLA_CON3,  1,  1, 0);      //LNA_GSEL1_12v
    SFR(WLA_CON3,  2,  3, 5);      //LNA_ISEL_12v
    SFR(WLA_CON2,  15, 1, 1);      //LNA_MIX_EN_12v
    SFR(WLA_CON3,  5,  2, 0);      //LNA_IADJ_12v

    SFR(WLA_CON3,  7,  2, 1);      //BP_AVDD_S_12v
    SFR(WLA_CON4,  0,  4, 0xf);    //BP_GA_12v
    SFR(WLA_CON3,  15, 1, 1);      //BP_EN_12v

    SFR(WLA_CON4,  4,  2, 0);      //BP_ISEL_12v

    SFR(WLA_CON4,  6,  1, 1);      //IQ_BIAS_ISEL_12v
    SFR(WLA_CON4,  7,  1, 0);      //LO_BIAS_ISEL_12v

    SFR(WLA_CON4,  8,  2, 1);      //LDOISEL_12v

    SFR(WLA_CON4,  10, 4, 1);      //DMIX_ADJ_12v
    SFR(WLA_CON4,  14, 1, 0);      //DMIX_BIAS_SEL_12v
    SFR(WLA_CON5,  0,  2, 3);      //DMIX_ISEL_12v
    SFR(WLA_CON5,  2,  3, 7);      //DMIX_R_S_12v

//======================= tx ===========================//
    SFR(WLA_CON5,  5,  3,  0);     //DAC_BIAS_SET_12v
    SFR(WLA_CON5,  8,  3,  4);     //DAC_GAIN_SET_12v        nor  6 low 4
    SFR(WLA_CON5,  11, 3,  0);    //DAC_VCM_S_12v

//    SFR(WLA_CON6,  0,  6,  0x1f);  //IL_TRIM_12v
//    SFR(WLA_CON6,  6,  6,  0x1f);  //IR_TRIM_12v
//    SFR(WLA_CON7,  0,  6,  0x1f);  //QL_TRIM_12v
//    SFR(WLA_CON7,  6,  6,  0x1f);  //QR_TRIM_12v
    SFR(WLA_CON8,  0,  1,  1);     //TXDAC_EN_12v

    SFR(WLA_CON5,  14, 2,  0);     //TRIM_CUR_12v
    SFR(WLA_CON8,  1,  2,  1);     //TXDAC_LDO_S_12v
    SFR(WLA_CON6,  12, 2,  1);     //TXIQ_LDO_S_12v
    SFR(WLA_CON6,  14, 2,  1);     //UMIX_LDO_S_12v

    SFR(WLA_CON7,  12, 1,  1);     //TXLDO_EN_12v
    SFR(WLA_CON7,  13, 1,  0);     //EN_TRIMI_12v
    SFR(WLA_CON7,  14, 1,  0);     //EN_TRIMQ_12v
    SFR(WLA_CON7,  15, 1,  0);     //TX_EN_TEST_12v
    SFR(WLA_CON8,  3,  1,  1);     //TX_IQ_BIAS_ISEL_12v
    SFR(WLA_CON8,  4,  1,  1);     //TX_IQ_LO_ISEL_12v
    SFR(WLA_CON8,  5,  1,  0);     //EN_TRIM_12v
    SFR(WLA_CON1,  14, 1,  1);     //TX_IQ_BIAS_SEL_12v
    SFR(WLA_CON8,  6,  4,  0xB);   //MIXER_LCSEL_12v
    SFR(WLA_CON8,  10, 2,  0);     //MIXER_RCSEL_12v

    SFR(WLA_CON8,  12, 4,  0x3);   //UMLO_BIAS_S_12v

    SFR(WLA_CON9,  0,  2,  1);     //MIXER_GISEL_12v
    SFR(WLA_CON9,  2,  1,  1);     //EN_TXBIAS_12v
    SFR(WLA_CON9,  3,  1,  1);     //EN_TXIQ_12v
    SFR(WLA_CON10, 15, 1,  1);     //EN_UM_12v

    SFR(WLA_CON9,  4,  5,  0x10);   //PAVSEL0_12v
    SFR(WLA_CON9,  9,  7,  0x4f);  //PA_CSEL0_12v

    SFR(WLA_CON10, 0,  5,  0x1f);   //PA_G_SEL0_12v      nor 0X1F low 0xb
    SFR(WLA_CON10, 5,  4,  0xa);   //PA_BIAS_S0_12v      nor 0XA  low 0x8
    SFR(WLA_CON10, 9,  1,  1);     //EN_PA_12v

  //======================= osc ===========================//
    //SFR(WLA_CON17, 0,  5,  0X00); // BTOSC_CLSEL 0x12
    //SFR(WLA_CON17, 5,  5,  0X00); // BTOSC_CRSEL 0x12
    SFR(WLA_CON17, 10, 4,  3); // BTOSC_Hcs

    SFR(WLA_CON29, 11, 1,  0); // BTOSC_EN_SEL_BASEBAND
    SFR(WLA_CON14, 10, 1,  1); // BTOSC_EN
    SFR(WLA_CON14, 13, 1,  1); // BTOSC_BT_OE
    SFR(WLA_CON14, 14, 1,  1); // BTOSC_FM_OE

    SFR(WLA_CON14, 11, 1,  0); // BTOSCLDO
    SFR(WLA_CON14, 12, 1,  0); // BTOSC_TEST

    SFR(WLA_CON18, 2, 2,   0);  // BTOSC_XHD_S
    SFR(WLA_CON17, 14, 2,  2); // BTOSC_LDO_S

   //======================= pll ===========================//
    SFR(WLA_CON10, 10, 1,  0);     //PLL_ABSEN_12v
    SFR(WLA_CON10, 11, 1,  1);     //PLL_CKO5G_OE_12v
    SFR(WLA_CON10, 12, 2,  1);     //PLL_CKO5G_S0_12v

    SFR(WLA_CON11, 0,  8,  0x77);  //PLL_CAPS0_12v

    SFR(WLA_CON18, 0,  1,  0);     //PLL_MODE1_12v   MODE: 00=2M, 01=4M, 10=Frac
    SFR(WLA_CON14, 2,  1,  0);     //PLL_MODE0_12v
    SFR(WLA_CON12, 1,  1,  0);     //PLL_REFCLKSEL_12v  0：BTXOSC
    SFR(WLA_CON13, 2,  5,  10);    //PLL_REFS_12v     // 1
    SFR(WLA_CON14, 3,  1,  1);     //PLL_REFDIVEN_12v //frac : 0
    SFR(WLA_CON11, 8,  8,  127);   //PLL_FBDS0_12v   2321 + n  ; 2322+ (39 - 79)*2 // 2400 - 2480

    SFR(WLA_CON14, 0,  1,  1);     //PLL_ACCTB_12v       //1 normal
    SFR(WLA_CON12, 6,  1,  0);     //PLL_VC_DETEN_12v
    SFR(WLA_CON12, 7,  1,  0);     //PLL_VC_DRVEN_12v
    SFR(WLA_CON12, 8,  3,  1);     //PLL_VC_S0_12v
    SFR(WLA_CON12, 11, 1,  0);     //PLL_VC_OE_12v

    SFR(WLA_CON12, 12, 3,  3);     //PLL_IVCOS0_12v
    SFR(WLA_CON13, 0,  2,  0);     //PLL_PFD0_12v
    SFR(WLA_CON12, 4,  2,  3);     //PLL_LPF0_12v
    SFR(WLA_CON12, 2,  2,  3);     //PLL_ICP0_12v

    SFR(WLA_CON13, 10, 3,  3);     //PLL_TEST_S0_12v
    SFR(WLA_CON13, 13, 1,  0);     //PLL_TEST_OE_12v
    SFR(WLA_CON14, 1,  1,  0);     //PLL_CKO75M_OE_12v

    SFR(WLA_CON13, 14, 2,  1);     //PLL_VMS0_12v
    SFR(WLA_CON13, 8,  2,  0);     //PLL_BIAS_ADJ0_12v

    SFR(WLA_CON13, 7,  1,  0);     //PLL_BIAS_EN_12v
    SFR(WLA_CON12, 0,  1,  0);     //PLL_EN_12v         // note !!!
    SFR(WLA_CON12, 15, 1,  0);     //PLL_RN_12v

     //fractional PLL dsmSFR-top
    SFR(WLA_CON14, 4,  1,   0);    //dsms_12v
    SFR(WLA_CON15, 0,  16,  0);    //intf0_12v
    SFR(WLA_CON16, 0,  8,   0);    //intf16_12v
    SFR(WLA_CON16, 8,  8,   0);    //intf24_12v
    SFR(WLA_CON18, 4,  4,   0);    //intf32_12v
    SFR(WLA_CON14, 6,  4,   0xd);  //tsel0_12v

    SFR(WLA_CON12, 15, 1,  0);     //PLL_RN_12v
    SFR(WLA_CON14, 5,  1,  0);     //rst_12v
    SFR(WLA_CON12, 0,  1,  1);     //PLL_EN_12v         // note !!!
    SFR(WLA_CON13, 7,  1,  1);     //PLL_BIAS_EN_12v
    delay(rf.delay_sys*100);
    SFR(WLA_CON14, 5,  1,  1);     //rst_12v
    SFR(WLA_CON12, 15, 1,  1);     //PLL_RN_12v
   
    delay(rf.delay_sys*100);

    bta_pll_bank_scan(127);

    bt_printf("\n\n br17 analog  test  ......\n\n");
}


void RF_analog_init(char en)
{
    if(en == 0)
    {
        //********************** BASEBAND CONTROL ENABLE *********************//
        SFR(WLA_CON29, 0 , 1, 0);       //1 bt_ldo_sel
        SFR(WLA_CON29, 1 , 1, 0);       //1 bt_pllen_sel
        SFR(WLA_CON29, 2 , 1, 0);       //1 bt_pllb_sel
        SFR(WLA_CON29, 3 , 1, 0);       //1 bt_pllrn_sel
        SFR(WLA_CON29, 4 , 1, 0);       //1 bt_pll_sel
        SFR(WLA_CON29, 5 , 1, 0);       //1 bt_rxbp_sel
        SFR(WLA_CON29, 6 , 1, 0);       //1 bt_dmix_sel
        SFR(WLA_CON29, 7 , 1, 0);       //1 bt_txpa_sel
        SFR(WLA_CON29, 8 , 1, 0);       //1 bt_umix_sel
        SFR(WLA_CON29, 9 , 1, 0);       //1 bt_dac_sel
        SFR(WLA_CON29, 10 ,1, 0);       //1 bt_rxtx_sel

        SFR(WLA_CON29, 11 ,1, 0);       //bt_btosc_sel
        SFR(WLA_CON29, 14 ,1, 0);       //bt_en_sel
        SFR(WLA_CON29, 15 ,1, 0);       //bt_pll_acc

        SFR(WLA_CON28, 0 ,1, 0);        // 1 wl_dacen_sel
        SFR(WLA_CON28, 1 ,2, 0);        // 1 wl_paen_sel
        SFR(WLA_CON28, 3 ,1, 0);        // 1 wl_umixen_sel
        SFR(WLA_CON28, 4 ,1, 0);        // 1 wl_adcen_sel
        SFR(WLA_CON28, 5 ,1, 0);        // 1 wl_lnaen_sel
        SFR(WLA_CON28, 6 ,1, 0);        // 1 wl_bpen_sel
        SFR(WLA_CON28, 7 ,1, 0);        // 1 wl_pagain_sel

        SFR(WLA_CON28, 8 ,1, 0);        //wl_pllmode_sel
        SFR(WLA_CON28, 9 ,1, 0);        //wl_pllref_sel
        SFR(WLA_CON28, 10 ,1, 0);       //wl_dsm_sel
    }
    else
    {
        //********************** BASEBAND CONTROL ENABLE *********************//
        SFR(WLA_CON29, 0 , 1, 1);       //1 bt_ldo_sel

        SFR(WLA_CON29, 1 , 1, 1);       //1 bt_pllen_sel
        SFR(WLA_CON29, 2 , 1, 1);       //1 bt_pllb_sel
        SFR(WLA_CON29, 3 , 1, 1);       //1 bt_pllrn_sel
        SFR(WLA_CON29, 4 , 1, 1);       //1 bt_pll_sel
        SFR(WLA_CON29, 15 ,1, 0);       // bt_pll_acc
        SFR(WLA_CON28, 8 , 1, 1);       // wl_pllmode_sel
        SFR(WLA_CON28, 9 , 1, 1);       // wl_pllref_sel
        SFR(WLA_CON28, 10, 1, 0);       // wl_dsm_sel

        SFR(WLA_CON29, 5 , 1, 1);       //1 bt_rxbp_sel
        SFR(WLA_CON29, 6 , 1, 1);       //1 bt_dmix_se
        SFR(WLA_CON29, 10, 1, 1);       //1 bt_rxtx_sel
        SFR(WLA_CON28, 4 , 1, 1);       //1 wl_adcen_sel
        SFR(WLA_CON28, 5 , 1, 1);       //1 wl_lnaen_sel
        SFR(WLA_CON28, 6 , 1, 1);       //1 wl_bpen_sel

        SFR(WLA_CON29, 8 , 1, 1);       //1 bt_umix_sel
        SFR(WLA_CON29, 9 , 1, 1);       //1 bt_dac_sel
        SFR(WLA_CON29, 7 , 1, 1);       //1 bt_txpa_sel
        SFR(WLA_CON28, 0 , 1, 1);       //1 wl_dacen_sel
        SFR(WLA_CON28, 1 , 2, 1);       //1 wl_paen_sel
        SFR(WLA_CON28, 3 , 1, 1);       //1 wl_umixen_sel

        SFR(WLA_CON28, 7 ,1, 0);        // wl_pagain_sel
        SFR(WLA_CON29, 11 ,1, 0);       // bt_btosc_sel
        SFR(WLA_CON29, 14 ,1, 0);       // bt_en_sel

    }
}

void bt_osc_internal_cfg(u8 sel_l,u8 sel_r)
{
    SFR(WLA_CON17, 0,  5,  sel_l); // BTOSC_CLSEL 0x12
    SFR(WLA_CON17, 5,  5,  sel_r); // BTOSC_CRSEL 0x12
}

/* void bta_reset_pll_bank() */
/* { */
    /* RF_analog_init(0); */
    /* bta_pll_bank_scan(129); */
    /* RF_analog_init(1); */
    /* bd_freq_table_init(1); */
/* } */

void bt_rf_analog_init(void)
{
    RF_analog_init(0);
    //RF_rxtx_init();
    bt_analog_init();
    RF_analog_init(1);
}
