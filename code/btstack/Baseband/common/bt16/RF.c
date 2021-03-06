#include "RF.h"
#include "typedef.h"


#define  fsk_offset_1     16
#define  fsk_offset_c1    11
#define  fsk_offset_op    5

volatile short fsk_offset_dly;
volatile short offset_cnt;

void fsk_offset(char set)
{
    long buf32;
    short in,bu16, out;

   in = BT_MDM_CON6 & 0XFFFF;
   /*printf("\n %d.%d", in, (short)(BT_MDM_CON7 & 0XFFFF));*/
   in = in << fsk_offset_op;
   if(set)
   {
      buf32 = (long) (in<<fsk_offset_c1) + (long) (fsk_offset_dly<<fsk_offset_1) - (long) (fsk_offset_dly<<(fsk_offset_c1+1));
      bu16 = buf32 >> fsk_offset_1;
      out = (bu16 + fsk_offset_dly)>>fsk_offset_op;
      fsk_offset_dly = bu16;

      if(offset_cnt > 50)
      {
          /*printf("*%d.", out );*/
          SFR(BT_MDM_CON6, 16, 12, out);
          //SFR(BT_MDM_CON7, 16, 12, out);
          SFR(BT_MDM_CON0, 30, 1, 1);        // fsk carrier frequency deviation switch
         // SFR(BT_MDM_CON0, 31, 1, 1);        // psk carrier frequency deviation switch
      }
      else
      {
          SFR(BT_MDM_CON0, 30, 1, 0);        // fsk carrier frequency deviation switch
        //  SFR(BT_MDM_CON0, 31, 1, 0);        // psk carrier frequency deviation switch
          offset_cnt++;
      }
   }
   else
   {
      fsk_offset_dly = 0;
      offset_cnt = 0;
   }
}

void RF_mdm_init(void)
{
// BT_MDM_CON2 is reserved !!!
    SFR(BT_MDM_CON0, 1, 3, 4);         // when to switch fsk mode to psk mode(in the 5uS guard)
    SFR(BT_MDM_CON0, 4, 4, 9);         // delay fsk data to fit the phase with psk data
    SFR(BT_MDM_CON0, 8, 3, 0);         // best sampling point offset
    SFR(BT_MDM_CON0, 11, 1, 0);        // psk symbol time track disable
    SFR(BT_MDM_CON0, 12, 2, 1);        // psk preamble detect mode
    SFR(BT_MDM_CON0, 14, 2, 0);        // fsk bandpass filter select
    SFR(BT_MDM_CON0, 16, 4, 8);        // 4.0 fsk demodulate state control, threshold = de4_con * 16
    SFR(BT_MDM_CON0, 22, 1, 1);        // tx i/q channel exchange
    SFR(BT_MDM_CON0, 23, 1, 0);        // rx i/q channel exchange
    SFR(BT_MDM_CON0, 24, 1, 1);        // tx delta-sigma modulator level select,  0: 64 level  1: 32 level
    SFR(BT_MDM_CON0, 25, 1, 0);        // delta-sigma modulator enable mode,  1: always enable
    SFR(BT_MDM_CON0, 26, 3, 5);        // rx dc-cancelling setting
    SFR(BT_MDM_CON0, 29, 1, 0);        // fsk/psk amplitude separate, 1: fsk/psk use different amplitude
    SFR(BT_MDM_CON0, 30, 1, 0);        // fsk carrier frequency deviation switch
    SFR(BT_MDM_CON0, 31, 1, 0);        // psk carrier frequency deviation switch
    SFR(BT_MDM_CON1, 0, 4, 0);         // tx fsk carrier frequency offset setting
    SFR(BT_MDM_CON1, 4, 4, 2);         // fsk preamble block time select, time = (n * 16)uS
    SFR(BT_MDM_CON1, 8, 4, 0);         // fsk preamble test control, [3:2]: (13/9/5/1), [1:0]: (16/12/8/4)
    SFR(BT_MDM_CON1, 12, 1, 0);        // fsk BT add 8KHz
    SFR(BT_MDM_CON1, 13, 1, 1);        // rx hilbert filter enable
    SFR(BT_MDM_CON1, 14, 1, 1);        // rx fsk bandpass filter enable
    SFR(BT_MDM_CON1, 15, 1, 1);        // rx psk bandpass filter enable
    SFR(BT_MDM_CON1, 16, 1, 0);        // intermediate frequency enable
    SFR(BT_MDM_CON1, 17, 3, 0);        // intermediate frequency gate select, 0: 256uS  1: 512uS  ....  7: 32768uS
    SFR(BT_MDM_CON1, 20, 4, 6);        // 2.1 fsk demodulate state control, threshold = dem_con * 16
    SFR(BT_MDM_CON1, 24, 3, 4);        // first bests length, length = (best_len + 8)uS
    SFR(BT_MDM_CON1, 27, 1, 0);        // analog test enable(for testing only)
    SFR(BT_MDM_CON1, 28, 1, 0);        // psk frequency offset compensation disable
    SFR(BT_MDM_CON1, 29, 1, 0);        // fsk frequency offset compensation disable
    SFR(BT_MDM_CON1, 30, 1, 0);        // d8psk mode enable, 0: d8psk mode is disable
    SFR(BT_MDM_CON1, 31, 1, 0);        // dsf2psk agc select, 0: use fsk filter(rssi2)  1: use psk filter(rssi3)
    SFR(BT_MDM_CON3, 0, 10, 256);      // psk amplitude i channel
    SFR(BT_MDM_CON3, 16, 10, 256);     // psk amplitude q channel
    SFR(BT_MDM_CON4, 0, 10, 256);      // fsk amplitude i channel
    SFR(BT_MDM_CON4, 16, 10, 256);     // fsk amplitude q channel
    SFR(BT_MDM_CON5, 0, 10, 0);        // tx offset i channel
    SFR(BT_MDM_CON5, 16, 10, 0);       // tx offset q channel
    SFR(BT_MDM_CON6, 16, 12, 0);       // fsk carrier frequency deviation setting
        // BT_MDM_CON6[11:0]:          // fsk carrier frequency deviation value
    SFR(BT_MDM_CON7, 16, 10, 0);       // psk carrier frequency deviation setting
        // BT_MDM_CON7[9:0]:           // psk carrier frequency deviation value
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


void RF_rxtx_init()
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
		spi_wr_rg(BTACON0, 4, 1, 1);   //TB_EN
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
  spi_wr_rg(BTACON5, 13, 2, 2);    // TXDAC_LDO_S

  spi_wr_rg(BTACON6, 12, 1, 1);    // TXIQ_LDO_S
  spi_wr_rg(BTACON6, 14, 2, 2);    // UMIX_LDO_S

  spi_wr_rg(BTACON8, 0, 1, 1);     // TX_IQ_BISA_ISEL
  spi_wr_rg(BTACON8, 4, 4, 0xf);   // MIXER_LCSEL
  spi_wr_rg(BTACON8, 8, 3, 0x0);   // MIXER_RCSEL
  spi_wr_rg(BTACON8, 11, 5, 0x1f); // PA_CSEL
  spi_wr_rg(BTACON9, 0, 4, 0x1);   // PA_G_SEL
//
  spi_wr_rg(BTACON8, 2, 1, 1);     // LDO28_1_EN
//  spi_wr_rg(BTACON8, 3, 1, 1);   // LDO28_2_EN
  spi_wr_rg(BTACON7, 12, 1, 1);    // TX_EN

  /*if(connet)
  {
    spi_wr_rg(BTACON5, 15, 1, 1);    // TXLDO_EN
    spi_wr_rg(BTACON7, 15, 1, 1);    // TX_EN_TEST
  }
  else*/
  {
    spi_wr_rg(BTACON5, 15, 1, 0);    // TXLDO_EN
    spi_wr_rg(BTACON7, 15, 1, 0);    // TX_EN_TEST
  }

//  spi_wr_rg(BTACON7, 13, 1, 1);  // TX_EN_I
//  spi_wr_rg(BTACON7, 14, 1, 1);  // TX_EN_Q
//  spi_wr_rg(BTACON6, 0, 6, 1);  // IL_TRIM
//  spi_wr_rg(BTACON6, 6, 6, 1);  // IR_TRIM
//  spi_wr_rg(BTACON7, 0, 6, 1);  // QL_TRIM
//  spi_wr_rg(BTACON7, 6, 6, 1);  // QR_TRIM
//  spi_wr_rg(BTACON8, 1, 1, 1);  // TX_TRIM_EN
  spi_wr_rg(BTACON5, 11, 1, 0);    // TRIM_CUR
  spi_wr_rg(BTACON5, 12, 1, 0);    // TRIM_SE

  spi_wr_rg(BTACON9, 4, 2, 1); // RCCL_DIV_SEL
  spi_wr_rg(BTACON9, 6, 1, 0); // RCCL_EN
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


void RF_trim_iq_all(void)
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

#define BLE_ENABLE 1
#define BREDR_ENABLE 0

void RF_init()
{
    

#ifdef FPGA 
	RF_pll_init();
	RF_rxtx_init();
	RF_analog_init(1);
#else
	BT_FPLL_ADR =  0;
	bt_analog_init();
	baseband_analog(1);
#endif

    RFI_CON |= BIT(2);
	RFI_CON |= BIT(1);

	BTA_CON14 = 0XFF;

#if BLE_ENABLE
    ble_rf_init();
#endif

#if BREDR_ENABLE
    BT_BSB_EDR = (1<<7) | 58;  // 61
#endif

	RF_mdm_init();
	
#ifndef FPGA 
	fsk_offset(0);
#endif
}








