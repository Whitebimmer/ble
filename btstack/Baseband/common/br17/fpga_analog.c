#include "RF.h"
#include "cpu.h"
#include "typedef.h"


#define  WL_SPICS_CLOSE        PORTC_DIR  |= (1<<3)
#define  WL_SPICS_UP           PORTC_OUT  |= (1<<3)
#define  WL_SPICS_DW           PORTC_OUT  &= ~(1<<3)
/**
    fpga 环境上与蓝牙小板模块通信设置
*/

static volatile unsigned long spi_dat;

static unsigned long chdat(unsigned long dat)
{
    unsigned long  buf;
    buf = 0;
    buf |= (dat >> 24 & 0xff);
    buf |= (dat >> 16 & 0xff)<<8;
    buf |= (dat >> 8 & 0xff)<<16;
    buf |= (dat  & 0xff)<<24;

    return buf;
}


static void spi_int(void)
{
    SPI1_BAUD = 0X03;
    SPI1_CON |= (1<<1);   // slave_en
    SPI1_CON &= ~(1<<3);  // WIRE3EN
    SPI1_CON |= (1<<4);   // EDGE_SMP
    SPI1_CON &= ~(1<<5);  // EDGE_UD
    SPI1_CON &= ~(1<<6);  // CLK_IDST
    SPI1_CON |= (1<<12);  // SPI_DIR
    SPI1_CON |= (1<<13);  // IE
    //SPI1_CON |= (1<<0);   // en
    PORTC_DIR |=  (1<<4);  //CLK
    WL_SPICS_DW;
    PORTC_DIR &= ~(1<<3); //CS
    IOMC1 |= (1<<4);
}

static void spi_wr(unsigned char adr , unsigned long dat)
{
      volatile unsigned long spi_dat;

      SPI1_CON |= (1<<0);   // en
      while(!(SPI1_CON & (1<<15))) ;
      SPI1_CON &= ~(1<<12);  // SPI_DIR
      SPI1_BUF = adr | 0x80;
      WL_SPICS_UP;

      while(!(SPI1_CON & (1<<15))) ;
      SPI1_CON &= ~(1<<12);  // SPI_DIR
      spi_dat = dat;//chdat(dat);
      SPI1_ADR = (unsigned long) &spi_dat;
      SPI1_CNT = 4;
      WL_SPICS_DW;
      while(!(SPI1_CON & (1<<15))) ;
      SPI1_CON &= ~(1<<0);   // en
}

static unsigned long spi_rd(unsigned char adr)
{
      volatile unsigned long spi_dat;

      SPI1_CON |= (1<<0);   // en
      while(!(SPI1_CON & (1<<15))) ;
      SPI1_CON &= ~(1<<12);  // SPI_DIR
      SPI1_BUF = adr & 0x7f;
      WL_SPICS_UP;

      while(!(SPI1_CON & (1<<15))) ;
      SPI1_CON |= (1<<12);  // SPI_DIR
      SPI1_ADR = (unsigned long )&spi_dat;
      SPI1_CNT = 4;
      WL_SPICS_DW;
      while(!(SPI1_CON & (1<<15))) ;

      SPI1_CON &= ~(1<<0);   // en
      return  spi_dat;//chdat(spi_dat);
}

static void spi_wr_rg(unsigned char adr , unsigned char bit, unsigned char len, unsigned long dat)
{
    unsigned long buf;
    buf = spi_rd(adr);
    buf = (buf & (~((~(0xffffffffL<<len))<<bit))) | ((dat&(~(0xffffffffL<<len)))<<bit);
    spi_wr(adr, buf);
}

char spi_agc_inc(char sel, char inc)
{
   static char agc_set;
   char lna_gsel, dmix_r, bp_ga, bp_gb, dato;

   if(!sel)
   {
       if(inc < 0)
       {
           agc_set = 0;
           dato = 0;
       }
       else if(inc>15)
       {
           agc_set = 15;
           dato = 0;
       }
       else
       {
           agc_set = inc;
           dato = 1;
       }

   }
   else
   {
        agc_set = agc_set + inc;
        if(agc_set< 0)
        {
            agc_set = 0;
            dato = 0;
        }
        else if(agc_set > 15)
        {
            agc_set = 1;
            dato = 0;
        }
        else
        {
           dato = 1;
        }
   }
    switch(agc_set)
    {
        case 0 : lna_gsel=2; dmix_r =0; bp_ga= 0; bp_gb = 0;  break; //-1db
        case 1 : lna_gsel=0; dmix_r =0; bp_ga= 0; bp_gb = 0;  break; //2db
        case 2 : lna_gsel=2; dmix_r =1; bp_ga= 0; bp_gb = 0;  break; //5db
        case 3 : lna_gsel=0; dmix_r =1; bp_ga= 0; bp_gb = 0;  break; //8db
        case 4 : lna_gsel=3; dmix_r =0; bp_ga= 0; bp_gb = 0;  break; //11db
        case 5 : lna_gsel=1; dmix_r =0; bp_ga= 0; bp_gb = 0;  break; //14db
        case 6 : lna_gsel=3; dmix_r =1; bp_ga= 0; bp_gb = 0;  break; //17db
        case 7 : lna_gsel=1; dmix_r =1; bp_ga= 0; bp_gb = 0;  break; //20db
        case 8 : lna_gsel=1; dmix_r =2; bp_ga= 0; bp_gb = 0;  break; //23db
        case 9 : lna_gsel=1; dmix_r =2; bp_ga= 0; bp_gb = 2;  break; //26db
        case 10: lna_gsel=1; dmix_r =2; bp_ga= 2; bp_gb = 2;  break; //29db
        case 11 : lna_gsel=1; dmix_r =2; bp_ga= 0; bp_gb = 8;  break; //32db
        case 12 : lna_gsel=1; dmix_r =2; bp_ga= 2; bp_gb = 8;  break; //35db
        case 13 : lna_gsel=1; dmix_r =2; bp_ga= 4; bp_gb = 8;  break; //38db
        case 14 : lna_gsel=1; dmix_r =3; bp_ga= 4; bp_gb = 8; break; //41db
        case 15 : lna_gsel=1; dmix_r =3; bp_ga= 8; bp_gb = 8; break; //44db
       // case 16 : lna_gsel=1; dmix_r =4; bp_ga= 8; bp_gb = 8; break; //44db
        default : lna_gsel=1; dmix_r =3; bp_ga= 8; bp_gb = 8; dato = 0;
    }
    printf("\nagc:%d",agc_set);
    spi_wr_rg(0,28,2,lna_gsel);
    spi_wr_rg(1,9,3,dmix_r);
    spi_wr_rg(1,22,4,bp_ga);
    spi_wr_rg(1,26,4,bp_gb);
    return dato;
}


char spi_pwr_inc(char sel, char inc)
{
   static char pwr_set;
   char dat0, pwr_d;
   if(!sel)  // min
   {
      if(inc < 0)
      {
          pwr_set = 0;
          dat0 = 0;
      }
      else if(inc>14)
      {
          pwr_set = 14;
          dat0 = 0;
      }
      else
      {
          pwr_set = inc;
          dat0 = 0;
      }
   }
   else
   {
       pwr_set = pwr_set + inc;
       if(pwr_set < 0)
       {
           pwr_set = 0;
           dat0 = 0;
       }
       else if (pwr_set > 14)
       {
           pwr_set = 14;
           dat0 = 0;
       }
       else
       {
           dat0 = 1;
       }
   }

   switch(pwr_set)
   {
       case 0 : pwr_d = 1; break; //-23.9db
       case 1 : pwr_d = 2; break; //-5.9db
       case 2 : pwr_d = 3; break; //-5db
       case 3 : pwr_d = 4; break; //-1.4db
       case 4 : pwr_d = 5; break; //-1db
       case 5 : pwr_d = 8; break; //0.9db
       case 6 : pwr_d = 9; break; //1.2db
       case 7 : pwr_d = 6; break; //1.8db
       case 8 : pwr_d = 7; break; //2db
       case 9 : pwr_d = 10; break;//3.2db
       case 10 : pwr_d = 11; break; //3.3db
       case 11 : pwr_d = 12; break; //4.2db
       case 12 : pwr_d = 13; break; //4.4db
       case 13 : pwr_d = 14; break; //5.3db
       case 14 : pwr_d = 15; break; //5.5db
       default : pwr_d = 15; dat0 = 0;break;

   }
   spi_wr_rg(3,6,4,pwr_d);  // pa_gsel
   return dat0;
}

static void RF_pll_init(void)
{

  spi_wr_rg(BTACON13, 7, 1, 1);  // XOSC_EN
  spi_wr_rg(BTACON13, 0, 1, 0);  // XOSC_AACEN
  spi_wr_rg(BTACON13, 8, 1, 1);  // XOSC_HCEN
  spi_wr_rg(BTACON13, 1, 3, 0);  // XOSC_CLS
  spi_wr_rg(BTACON13, 4, 3, 0);  // XOSC_CRS

  spi_wr_rg(BTACON10, 0, 5, 0x7);  // PLL_CASPS
  spi_wr_rg(BTACON10, 8, 5, 0x3);  // PLL_CFS
  spi_wr_rg(BTACON11, 0, 8, 127);  // FREQ_FS   /// 2440 = 2321 + 119  (2402 - 2480) : (81 159)
  spi_wr_rg(BTACON11, 8, 2, 3);  // PLL_ICP
  spi_wr_rg(BTACON10, 13, 3, 7); // PLL_IVCOS
  spi_wr_rg(BTACON12, 0, 2, 3);  // PLL_RFDS
  spi_wr_rg(BTACON11, 10, 5, 4); // PLL_RFS     /(n+2)

//  spi_wr_rg(BTACON12, 2, 2, 1);  // PLL_TEST
//  spi_wr_rg(BTACON12, 4, 1, 1);  // PLL_TEST_EN
//  spi_wr_rg(BTACON12, 5, 1, 1);  // PLL_VCD_EN
  spi_wr_rg(BTACON12, 6, 3, 2);  // PLL_VMS
//  spi_wr_rg(BTACON12, 9, 2, 1);  // PLL_VVCOS
//  spi_wr_rg(BTACON12, 11, 3, 1); // PLL_VVCOS_EN

//  spi_wr_rg(BTACON10, 5, 1, 1);  // PLL_CBEN
  spi_wr_rg(BTACON10, 6, 1, 1);  // PLL_EN
  spi_wr_rg(BTACON10, 7, 1, 1);  // PLL_CLK_TEN

  spi_wr_rg(BTACON11, 15, 1, 0); // PLL_RN
  delay(10);
  spi_wr_rg(BTACON11, 15, 1, 1); // PLL_RN

// only read : BTACON13[12:9]
// PLL_VVCOS PLL_VCL PLL_VCH PLL_PFD_DET
}


static void RF_rxtx_init()
{
//------------ RX ----------------//
  spi_wr_rg(BTACON0, 0, 1, 1);  //  LDO_EN

  /*if(connet)
  {
        spi_wr_rg(BTACON0, 2, 2, 2);  //  RX_O_EN_S     1           2
        spi_wr_rg(BTACON0, 1, 1, 0);  //  RXLDO_EN
        spi_wr_rg(BTACON0, 4, 1, 0);  //  TB_EN
        spi_wr_rg(BTACON0, 5, 1, 1);  //  TB_SEL        0 mix_dm   1_bp
  }
  else*/
  {
		spi_wr_rg(BTACON0, 2, 2, 2);   //RX_O_EN_S     1           2
		spi_wr_rg(BTACON0, 1, 1, 1);   //RXLDO_EN
		spi_wr_rg(BTACON0, 4, 1, 0);   //TB_EN
		spi_wr_rg(BTACON0, 5, 1, 1);   //TB_SEL        0 mix_dm   1_bp
  }

  spi_wr_rg(BTACON0, 6, 2, 1);  //  VREF_S

  spi_wr_rg(BTACON0, 10, 5, 0x10);  // ADC_C_SEL
  spi_wr_rg(BTACON0, 15, 1, 1);  // ADC_DEM_EN
  spi_wr_rg(BTACON1, 0, 1, 1);   // ADC_EN
  spi_wr_rg(BTACON1, 1, 7, 0xf);   // ADC_IDAC_S
  spi_wr_rg(BTACON1, 8, 2, 1);   // ADCDVDD_S
  spi_wr_rg(BTACON1, 10, 2, 1);  // ADC_LDO_S
  spi_wr_rg(BTACON1, 12, 2, 1);  // AVDD1_S
  spi_wr_rg(BTACON1, 14, 2, 1);  // AVDDFE_S
  spi_wr_rg(BTACON2, 0, 5, 0x10);   // ADC_VCM_S
  spi_wr_rg(BTACON2, 5, 5, 0x1);   // BG_ISEL

  spi_wr_rg(BTACON2, 10, 1, 1);  // LNAV_EN
  spi_wr_rg(BTACON2, 11, 3, 1);  // LNAV_RSEL
  spi_wr_rg(BTACON2, 14, 2, 1);  // LNA_GSL
  spi_wr_rg(BTACON3, 0, 3, 1);   // LNA_ISEL
  spi_wr_rg(BTACON3, 3, 1, 1);   // LNA_MIX_EN

  spi_wr_rg(BTACON3, 11, 1, 1);  // IQ_BIAS_ISEL
  spi_wr_rg(BTACON3, 12, 1, 1);  // LO_BIAS_ISEL

  spi_wr_rg(BTACON3, 13, 3, 1);  // DMIX_ADJ
  spi_wr_rg(BTACON4, 9, 1, 1);   // DMIX_BIAS_SEL
  spi_wr_rg(BTACON4, 10, 2, 1);  // DMIX_ISEL
  spi_wr_rg(BTACON4, 12, 3, 0);  // DMIX_R_S

  spi_wr_rg(BTACON3, 4, 2, 1);    // BP_AVDD_S
  spi_wr_rg(BTACON3, 6, 5, 0x11); // BP_BW
  spi_wr_rg(BTACON4, 0, 4, 0x8);  // BP_GA
  spi_wr_rg(BTACON4, 4, 4, 0x8);  // BP_GB
  spi_wr_rg(BTACON4, 8, 1, 1);    // BP_EN

//------------ TX ----------------//
  spi_wr_rg(BTACON5, 0, 3, 0);     // DAC_BIAS_SET
  spi_wr_rg(BTACON5, 3, 3, 0x4);  // DAC_GAIN_SET  6
  spi_wr_rg(BTACON5, 6, 5, 0x10);     // DAC_VCM_S  1
  spi_wr_rg(BTACON5, 11, 1, 1);    // TRIM_CUR
  spi_wr_rg(BTACON5, 12, 1, 1);    // TRIM_SE
  spi_wr_rg(BTACON5, 13, 2, 2);    // TXDAC_LDO_S

  spi_wr_rg(BTACON6, 12, 1, 1);    // TXIQ_LDO_S
  spi_wr_rg(BTACON6, 14, 2, 2);    // UMIX_LDO_S

  spi_wr_rg(BTACON8, 0, 1, 1);     // TX_IQ_BISA_ISEL
  spi_wr_rg(BTACON8, 4, 4, 0x8);   // MIXER_LCSEL
  spi_wr_rg(BTACON8, 8, 3, 0x0);   // MIXER_RCSEL
  spi_wr_rg(BTACON8, 11, 5, 0x1f); // PA_CSEL
  spi_wr_rg(BTACON9, 0, 4, 0xf);   // PA_G_SEL
//
  spi_wr_rg(BTACON8, 2, 1, 1);     // LDO28_1_EN
//  spi_wr_rg(BTACON8, 3, 1, 1);   // LDO28_2_EN
  spi_wr_rg(BTACON5, 15, 1, 0);    // TXLDO_EN
  spi_wr_rg(BTACON7, 12, 1, 1);    // TX_EN
  spi_wr_rg(BTACON7, 15, 1, 0);    // TX_EN_TEST


//  spi_wr_rg(BTACON7, 13, 1, 1);  // TX_EN_I
//  spi_wr_rg(BTACON7, 14, 1, 1);  // TX_EN_Q
//  spi_wr_rg(BTACON6, 0, 6, 1);  // IL_TRIM
//  spi_wr_rg(BTACON6, 6, 6, 1);  // IR_TRIM
//  spi_wr_rg(BTACON7, 0, 6, 1);  // QL_TRIM
//  spi_wr_rg(BTACON7, 6, 6, 1);  // QR_TRIM
//  spi_wr_rg(BTACON8, 1, 1, 1);  // TX_TRIM_EN

  spi_wr_rg(BTACON9, 4, 2, 1); // RCCL_DIV_SEL
  spi_wr_rg(BTACON9, 6, 1, 1); // RCCL_EN
  spi_wr_rg(BTACON9, 7, 1, 0); // RCCL_CLK_12M  : 0 EN 1 DISABLE

//spi_wr_rg(RFICON, 14, 1, 1);   //baseband control rx/tx ldo
// read only BTACON9[13:8]
// RCCL_BW[4:0] IQCAL_OUT


//  spi_wr_rg(LDOCON, 0, 1, 0);  // BT_LDO_EN
//  spi_wr_rg(LDOCON, 1, 1, 0);  // FM_LDO_EN

}

static void RF_analog_init(char en)
{
    if(en)
    {
       spi_wr_rg(RFICON, 12, 1, 1);   //baseband control pll_fs/pa_csel/pa_gsel/bp_ga/bp_gb/lna_csel/lna_isel/lnav_rsel
//
      /*(*ble_fp).RFTXCNTL0 = 0xFFFF;                                 //[14:0] TXPWR
      (*ble_fp).RFRXCNTL0 = (8<<9) | (8<<5) | (3<<2) | 1;           //[14:0] RXGAIN*/

//       SFR(BT_TXPWER, 0, 5, 0X1f); // PA_CSEL
//       SFR(BT_TXPWER, 5, 4, 0Xf); // PA_GSEL
//
//       SFR(BT_RXGAIN0, 0, 2, 0X3);  // LNA_GSEL
//       SFR(BT_RXGAIN0, 2, 3, 0X1);  // LNA_ISEL
//       SFR(BT_RXGAIN0, 5, 4, 0X8);  // BP_GA
//       SFR(BT_RXGAIN0, 9, 4, 0X8);  // BP_GB

       spi_wr_rg(RFICON, 13, 1, 0);   //baseband control SEL RXGAIN0 OR RXGAIN1

       spi_wr_rg(RFICON, 14, 1, 1);   //baseband control rx/tx ldo
     //  spi_wr_rg(RFICON, 15, 1, 1);   //baseband control pll/btxsco en
    }
    else
    {
       spi_wr_rg(RFICON, 12, 1, 0);   //baseband control pll_fs/pa_csel/pa_gsel/bp_ga/bp_gb/lna_csel/lna_isel/lnav_rsel

       spi_wr_rg(RFICON, 14, 1, 0);   //baseband control rx/tx ldo
    // spi_wr_rg(RFICON, 15, 1, 0);   //baseband control pll/btxsco en
    }
}

static void RF_trim_iq(short trim_w)
{
	short data_t,m,n,i_dc,q_dc, flag, i;

	for(i = 0; i<2; i++)
	{
		SFR(BT_MDM_CON4, 0,  14, 0);    // APM_I_4096
		SFR(BT_MDM_CON4, 16, 14, 0);    // APsimsimM_Q_4096    4066
		SFR(BT_MDM_CON5, 0,  14, 0);       // DC_I_0
		SFR(BT_MDM_CON5, 16, 14, 0);       // DC_Q_0
		flag = 1;
		m = trim_w;
		n = -trim_w;
		for(data_t = -trim_w; data_t <= trim_w ; data_t +=10 )
		{
			if(i== 0)
			{
				SFR(BT_MDM_CON5, 0,  14, data_t);       // DC_I_0
			}
			else if(i== 1)
			{
				SFR(BT_MDM_CON5, 16,  14, data_t);       // DC_Q_0
			}
			delay(1000);
			if(spi_rd(BTACON4) & BIT(15))
			{
				if((flag == 0) && (data_t > n))
				{
					n = data_t;
				}
				flag = 1;
			}
			else
			{
				if((flag == 1) && (data_t < m))
				{
					m = data_t;
				}
				flag = 0;
			}
		}

		if(i == 0)
		{
			i_dc = (n + m)/2;
		}
		else if(i == 1)
		{
			q_dc = (n + m)/2;
		}
	}

	SFR(BT_MDM_CON4, 0,  14, 4096);    // APM_I_4096
	SFR(BT_MDM_CON4, 16, 14, 4096);    // APsimsimM_Q_4096    4066
	SFR(BT_MDM_CON5, 0,  14, i_dc);       // DC_I_0
	SFR(BT_MDM_CON5, 16, 14, q_dc);       // DC_Q_0
}


static void RF_trim_iq_all(void)
{
	unsigned char dat = 0;

	spi_wr_rg(BTACON5, 15, 1, 1);    // TXLDO_EN
	spi_wr_rg(BTACON0,1,1, 1);   // RX LDO
	spi_wr_rg(BTACON2,10,1, 1);   //LNAV_EN_12v
	SFR(BT_MDM_CON4, 0,  14, 0);    // APM_I_4096
	SFR(BT_MDM_CON4, 16, 14, 0);    // APsimsimM_Q_4096    4066
	SFR(BT_MDM_CON5, 0,  14, 0);       // DC_I_0
	SFR(BT_MDM_CON5, 16, 14, 0);       // DC_Q_0

	while(1)
	{
		spi_wr_rg(BTACON2,11,3, dat);
		delay(1000);
		if((spi_rd(BTACON4))&BIT(15))
		{
			switch(dat)
			{
				case 0 :  dat = 4; break;
				case 4 :  dat = 2; break;
				case 2 :  dat = 6; break;
				case 6 :  dat = 1; break;
				case 1 :  dat = 5; break;
				case 5 :  dat = 3; break;
				case 3 :  dat = 7; break;
				case 7 :  dat = 0; break;
				default : dat = 0;
			}
		}
		else
		{
			RF_trim_iq(2000);
			break;
		}
	}
	spi_wr_rg(BTACON0,1,1, 0);   // RX LDO
	spi_wr_rg(BTACON2,10,1, 0);   //LNAV_EN_12v
}

void rf_debug()
{
    //bt14小板的程序要改
    spi_wr_rg(20, 7, 1, 0); // RCCL_CLK_12M  : 0 EN 1 DISABLE

    /* DIAGCNTL = (1<<7)|(5);//EN NUM */
    BT_EXT_CON |=BIT(1);  //DBG EN
    BT_EXT_CON |=BIT(5); //1:BLE 0:BREDR

    //关闭spi口的567口
    SPI1_CON &= ~(1<<0);
    IOMC1 &= ~(1<<4);

    //设置debug输出口
    PORTB_DIR&=~0x0ff0;
    PORTB_DIR&=~BIT(0);
    PORTB_OUT&=~BIT(0);
}

unsigned char get_bta_pll_bank(unsigned char set_freq)
{

}

void bta_check_pll_back(u8 mode)
{

}

unsigned long bt_pll_config_ram[80 * 2];
void bt_rf_freq_init(void) 
{
    u8 i;
	int frq_mid;   // 89 mid 2402  2M MID_FREQ_UP;
	printf("\n--func=%s\n", __FUNCTION__);
	BT_PLLCONFIG_ADR = bt_pll_config_ram;
	frq_mid = 81 - 2;
	for(i = 0; i < 79; i++)
	{
		bt_pll_config_ram[i * 2] = (frq_mid + i);
		bt_pll_config_ram[i * 2 + 1] = 0;//(pll_param<<8)|(frq_mid + i);
	}
}

void bt_rf_analog_init(void)
{

    /* puts("-1\n"); */
    spi_int();
    /* bt_printf("BTPORTC_DIR %x\n", PORTC_DIR); */
    /* bt_printf("BTPORTC_OUT %x\n", PORTC_OUT); */
    /* puts("-2\n"); */
    delay(3000);
    /* puts("-3\n"); */
	RF_pll_init();
    /* puts("-4\n"); */
	RF_rxtx_init();
    /* puts("-5\n"); */
	RF_analog_init(1);
    /* puts("-6\n"); */
    /* ENABLE_INT(); */
}


