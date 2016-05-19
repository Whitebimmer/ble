
#include "RF.h"
#include "cpu.h"

#define BTOSC_CLK   24

unsigned char bt_pll_cap0, bt_pll_cap1, bt_pll_cap2, bt_pll_cap3;

#define  sys_clk         64
#define  SET_CHANNEL     119
void bt_pll_scan(void)
{
	unsigned char udat8, dato, dath, datl, dat_buf, i;

	SFR(BTA_CON12,  9,  1,  1);   //PLL_VCD_EN
	SFR(BTA_CON12, 10,  1,  0);   //PLL_VCD_OE

	dath  = 74;

	for(i = 0; i < 5; i++)
	{
		udat8 = dath+20;
		datl  = dath;
		SFR(BTA_CON10, 2, 5, (0x1f>>i)); //PLL_CAPS   //<2420:F  >2420:7
		while(1)
		{
			SFR(BTA_CON11, 0, 8, udat8); //PLL_FS
			delay(sys_clk*200);
			dato  = 0;
			dato |= (BTA_CON15 >> 10) & 0x03;
			dato |= (BTA_CON15 >> 10) & 0x03;
			dato |= (BTA_CON15 >> 10) & 0x03;
			if(dato == 1) //PLL_VCH_DO = 1;
			{
				dath = 74;
				datl = 74;
				printf("a");
				break;
			}
			else if(dato == 0) //
			{
				while(dato !=1)
				{
					udat8++;
					if(udat8 > 163) break;
					SFR(BTA_CON11,  0,  8,  udat8); //PLL_FS
					delay(sys_clk*200);
					dato  = 0;
					dato |= (BTA_CON15 >> 10) & 0x03;
					dato |= (BTA_CON15 >> 10) & 0x03;
					dato |= (BTA_CON15 >> 10) & 0x03;
					printf("b");
				}
				dat_buf = udat8;

				while(dato !=2)
				{
					udat8--;
					if(udat8 < 74) break;
					SFR(BTA_CON11,  0,  8,  udat8); //PLL_FS
					delay(sys_clk*200);
					dato  = 0;
					dato |= (BTA_CON15 >> 10) & 0x03;
					dato |= (BTA_CON15 >> 10) & 0x03;
					dato |= (BTA_CON15 >> 10) & 0x03;
					printf("c");
				}
				datl = udat8;
				if(i == 1)
				{
					bt_pll_cap0 = (datl + dath)/2 + 2;
				}
				else if(i == 2)
				{
					bt_pll_cap1 = (datl + dath)/2 + 2;
				}
				else if(i == 3)
				{
					bt_pll_cap2 = (datl + dath)/2 + 2;
				}
				else
				{
					bt_pll_cap3 = (datl + dath)/2 + 2;
				}
				dath = dat_buf;
				break;
			}
			else
			{
				printf("d");
				if(udat8 > 245)
				{
					puts("\n\n\n...2 bt_pll_scan error...\n\n\n");
					break;
				}
				else udat8 = udat8 + 10;
			}
		}
		printf("\nl:%d h:%d",datl,dath);
	}
	SFR(BTA_CON12,  9,  1,  0);   //PLL_VCD_EN
	SFR(BTA_CON12, 10,  1,  0);   //PLL_VCD_OE

	if(bt_pll_cap0 > SET_CHANNEL)
	{
		SFR(BTA_CON10,  2,  5,  0x1f); //PLL_CAPS   //<2420:F  >2420:7
	}
	else if(bt_pll_cap1 > SET_CHANNEL)
	{
		SFR(BTA_CON10,  2,  5,  0xf); //PLL_CAPS   //<2420:F  >2420:7
	}
	else if(bt_pll_cap2 > SET_CHANNEL)
	{
		SFR(BTA_CON10,  2,  5,  0x7); //PLL_CAPS   //<2420:F  >2420:7
	}
	else if(bt_pll_cap3 > SET_CHANNEL)
	{
		SFR(BTA_CON10,  2,  5,  0x3); //PLL_CAPS   //<2420:F  >2420:7
	}
	else
	{
		SFR(BTA_CON10,  2,  5,  0x1); //PLL_CAPS   //<2420:F  >2420:7
	}

	SFR(BTA_CON11,  0,  8,  SET_CHANNEL); //PLL_FS     /// 2440 = 2321 + 119  (2402 - 2480) : (81 159)

	printf("\ncp0:%d cp1:%d cp2:%d cp3: %d",bt_pll_cap0,bt_pll_cap1,bt_pll_cap2, bt_pll_cap3);
}

u8 get_pll_param_for_frq(int frq)
{
	if (frq < bt_pll_cap0){
 		return 0x1f; 
	} else if(frq < bt_pll_cap1){
		return 0x0f;   
	} else if (frq < bt_pll_cap2){
		return 0x07;
	} else if (frq < bt_pll_cap3){
		return 0x03;
	}

	return 1;
}	



#if 0
#define FPLL_BASE_SET   (BLE_EXM_BASE_ADR + 0X3000)
void fpll_set(void)
{
    unsigned long *ptr;
    unsigned short i;

    BT_FPLL_ADR =  FPLL_BASE_SET;

    ptr = (unsigned long *) FPLL_BASE_SET;
    for(i=0; i < 80 ; i++)
    {
        *ptr++ = 0x0;    //  [3:0] fpll_tsel [23:4] fpll_frac [31:24] fpp_int
    }
}
#endif

#define  RCCL_ALL     99
void bt_rccl_trim(void)
{
    unsigned char cnt[64], cnt_all, buf, bw_set;

    // set sys_clk  24M
    SFR(CLK_CON0,  8,  1,  0); // SYS_CLK: 0:RC_CLK; 1:MUX_CLK
    delay(10);
    SFR(CLK_CON0,  6,  2,  2); // SRC_MUX_CLK:  0:OSC_CLK; 1:RTC_32K; 2:RC_CLK; 3:PLL_SYS_CLK
    SFR(CLK_CON2,  0,  2,  1); // PLL_SYS_CLK:  0:PLL_192M; 1:PLL_480M; 2:FM_PLL; 3:0
    SFR(CLK_CON2,  2,  2,  2); // PLL_SYS_DIV1: 0:1/1; 1:1/3; 2:1/5; 3:/7
    SFR(CLK_CON2,  4,  2,  2); // PLL_SYS_DIV2: 0:1/1; 1:1/2; 2:1/4; 3:/8
    SFR(CLK_CON0,  6,  2,  3); // SRC_MUX_CLK:  0:OSC_CLK; 1:RTC_32K; 2:RC_CLK; 3:PLL_SYS_CLK
    delay(10);
    SFR(CLK_CON0,  8,  1,  1); // SYS_CLK: 0:RC_CLK; 1:MUX_CLK

    //rccl initial
    SFR(BTA_CON0,  0,  1,  1);    // RX_LDO
    SFR(BTA_CON3,  0,  1,  1);    // LNA_MIX_EN
    SFR(BTA_CON2,  2,  5, 0X01);  //BG_ISEL   1
    SFR(BTA_CON8,  3,  1,  1);   // LDOISEL
    SFR(BTA_CON0,  5,  2,  1);    // VREF_S    正常工作设置为1
    SFR(BTA_CON8,  0,  1,  1);    // LDO_EN
    SFR(BTA_CON1,  9,  5, 0X10);  // ADC_VCM_S
    SFR(BTA_CON11, 15,  1,  1);   // RCCL_EN
    SFR(BTA_CON14,  0,  1,  1);   // RCCL_CLK: 0:1; 1:SYS_CLK
    SFR(BTA_CON13, 14,  2,  1);   // RCCL_DIV  12m = SYS_CLK/N
    SFR(BTA_CON12, 15,  1,  0);   // RCCL_GO
    delay(12*50000);              // 100 ms

    for(cnt_all = 0; cnt_all < 64; cnt_all++)
    {
        cnt[cnt_all] = 0;
    }
    for(cnt_all = 0; cnt_all < RCCL_ALL; cnt_all++)
    {
         SFR(BTA_CON12, 15,  1,  0);     // RCCL_GO
         delay(12*50);                   // 100 us
         SFR(BTA_CON12, 15,  1,  1);     // RCCL_GO
         delay(12*500);                  // 1ms，  8*64*(1/12M)=43us

         if(!((BTA_CON15>>8)&0x01))      // det_vo:0
         {
             buf = (BTA_CON15>>2)&0x3f;  // rccl_out
             cnt[buf] += 1;
             printf("\n%x", buf);
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
    printf("\nbw_set: %x  %d \n", bw_set, cnt[bw_set]);

    SFR(BTA_CON3,  3,  6, (bw_set&0x3f)); //BP_BW
    SFR(BTA_CON11, 15,  1,  0);   // RCCL_EN
    SFR(BTA_CON14,  0,  1,  0);   // RCCL_CLK: 0:1; 1:SYS_CLK

    // set sys_clk  96M
    SFR(CLK_CON0,  8,  1,  0); // SYS_CLK: 0:RC_CLK; 1:MUX_CLK
    delay(10);
    SFR(CLK_CON0,  6,  2,  2); // SRC_MUX_CLK:  0:OSC_CLK; 1:RTC_32K; 2:RC_CLK; 3:PLL_SYS_CLK
    SFR(CLK_CON2,  0,  2,  0); // PLL_SYS_CLK:  0:PLL_192M; 1:PLL_480M; 2:FM_PLL; 3:0
    SFR(CLK_CON2,  2,  2,  1); // PLL_SYS_DIV1: 0:1/1; 1:1/3; 2:1/5; 3:/7
    SFR(CLK_CON2,  4,  2,  0); // PLL_SYS_DIV2: 0:1/1; 1:1/2; 2:1/4; 3:/8
    SFR(CLK_CON0,  6,  2, 3); // SRC_MUX_CLK:  0:OSC_CLK; 1:RTC_32K; 2:RC_CLK; 3:PLL_SYS_CLK
    delay(10);
    SFR(CLK_CON0,  8,  1,  1); // SYS_CLK: 0:RC_CLK; 1:MUX_CLK

 //   while(1);
}

void bt_analog_init(void)
{
//    bt_rccl_trim();

    SFR(BTA_CON8,  0,  1,  1); // LDO_EN
//********************** RX *********************//
    SFR(BTA_CON0,  0,  1,  0); // RX_LDO
    SFR(BTA_CON0,  1,  2,  2); // RX_O_EN_S   1:DM  2:BP
    SFR(BTA_CON0,  3,  1,  1); // TB_EN
    SFR(BTA_CON0,  4,  3,  4); // TB_SEL:1xx=TX; 0x0=DM; 0x1=BP
    SFR(BTA_CON0,  14,  2,  1); // VREF_S

    SFR(BTA_CON0,  7,  5, 0X10); //ADC_C_SEL
    SFR(BTA_CON0, 12,  1,  1);   //ADC_DEM_EN
    SFR(BTA_CON0, 13,  1,  1);   //ADC_EN
    SFR(BTA_CON1,  0,  5, 0Xf);  //ADC_IDAC_S
    SFR(BTA_CON1,  5,  2,  1);   //ADC_ISEL
    SFR(BTA_CON1,  7,  2,  1);   //ADC_LDO_S
    SFR(BTA_CON1,  9,  5, 0X10); //ADC_VCM_S

    SFR(BTA_CON1, 14,  2,  2);   //AVDD1_S
    SFR(BTA_CON2,  0,  2,  2);   //AVDDFE_S
    SFR(BTA_CON2,  2,  5, 0X1);  //BG_ISEL

    SFR(BTA_CON2,  7,  1,  1);   //LNAV_EN
    SFR(BTA_CON2,  8,  3,  3);   //LNAV_RSEL
    SFR(BTA_CON2, 11,  1,  1);   //LNA_GSEL0
    SFR(BTA_CON2, 12,  1,  0);   //LNA_GSEL1
    SFR(BTA_CON2, 13,  3,  7);   //LNA_ISEL
    SFR(BTA_CON3,  0,  1,  1);   //LNA_MIX_EN

    SFR(BTA_CON3,  1,  2,  1);   //BP_AVDD_S
    SFR(BTA_CON3,  3,  6, 0X22); //BP_BW
    SFR(BTA_CON4,  0,  4,  0xf); //BP_GA
    SFR(BTA_CON3,  9,  1,  1);   //BP_EN

    SFR(BTA_CON4,  4,  2,  0);   //BP_ISEL

    SFR(BTA_CON3, 10,  1,  0);   //IQ_BIAS_ISEL
    SFR(BTA_CON3, 11,  1,  1);   //LO_BIAS_ISEL
    SFR(BTA_CON3,  6,  2,  1);   //LDO_ISEL


    SFR(BTA_CON3, 12,  4,  1);   //DMIX_ADJ
    SFR(BTA_CON4,  8,  1,  1);   //DMIX_BIAS_SEL
    SFR(BTA_CON4,  9,  2,  3);   //DMIX_ISEL
    SFR(BTA_CON4, 11,  3,  7);   //DMIX_R_S

//********************** TX *********************//
    SFR(BTA_CON5,  0,  3,  0);   // DAC_BIAS_SET
    SFR(BTA_CON5,  3,  3, 0x5);  // DAC_GAIN_SET
    SFR(BTA_CON5,  6,  5, 0X10); // DAC_VCM
    SFR(BTA_CON5, 11,  1,  1);   // TXDAC_EN
    SFR(BTA_CON5, 12,  2,  2);   // TXDAC_LDO_S

//    SFR(BTA_CON6,  0,  6,  0X20);// IL_TRIM
//    SFR(BTA_CON6,  6,  6,  0X20);// IR_TRIM
//    SFR(BTA_CON7,  0,  6,  0X20);// QL_TRIM
//    SFR(BTA_CON7,  6,  6,  0X20);// QR_TRIM
//    SFR(BTA_CON4, 14,  2,  2);   //TRIM_CUR0
//    SFR(BTA_CON6, 15,  1,  0);   // EN_TRIMI
//    SFR(BTA_CON7, 12,  1,  0);   // EN_TRIMQ

    SFR(BTA_CON8,  1,  1,  1);   // EN_TRIM

    SFR(BTA_CON5, 14,  2,  1);   // TXIQ_LDO_S
    SFR(BTA_CON6, 12,  2,  1);   // UMIX_LDO_S

    SFR(BTA_CON6, 14,  1,  1);   // TX_LDO_EN
    SFR(BTA_CON7, 13,  1,  0);   // TX_EN_TEST
    SFR(BTA_CON7, 14,  2,  0);   // TX_IQ_BIAS_ISEL


    SFR(BTA_CON8,  2,  2,  0);   // AVDD18_S  0:1.99V;  1:1.88V;  2:1.79V  3:1.71V

    SFR(BTA_CON8,  4,  4,  0x7); // MIXSR_LCSEL
    SFR(BTA_CON8,  8,  2,  0x0); // MIXER_RCSEL
    SFR(BTA_CON8, 10,  4,  3);   // UMLO_BIAS_S
    SFR(BTA_CON8, 14,  2,  1);   // MIXER_GISEL
    SFR(BTA_CON10, 0,  1,  1);   // EN_TXBIAS
    SFR(BTA_CON9,  0,  1,  1);   // EN_TXIQ
    SFR(BTA_CON9,  1,  1,  1);   // EN_UM

    SFR(BTA_CON9,  2,  5,  0X10);// PA_CSEL  tx: 0x16
    SFR(BTA_CON9,  7,  4,  0Xf); // PA_G_SEL
    SFR(BTA_CON9, 11,  4,  0XA); // PA_BIAS_S          add
    SFR(BTA_CON9, 15,  1,  1);   // EN_PA


//    BTA_CON15[14]:FPLL_DETOUT
//    BTA_CON15[13]:FPLL_VCZH
//    BTA_CON15[12]:FPLL_VCZL
//    BTA_CON15[11]:PLL_VCL_DO
//    BTA_CON15[10]:PLL_VCH_DO
//    BTA_CON15[9]:PLL_PFD_DET
//    BTA_CON15[8]:CAL_DET
//    BTA_CON15[7:2]: RCCL_BW
//    BTA_CON15[1]: IQCAL_OUT
//    BTA_CON15[0]: LNAV_O

//********************** PLL *********************//
    SFR(BTA_CON13,  1,  4,  0x0); // XOSC_CLSEL
    SFR(BTA_CON13,  5,  4,  0x0); // XOSC_CRSEL
    if(BTOSC_CLK>12)
    {
         SFR(BTA_CON13, 10,  1,  1);   // XOSC_HEN
    }
    else
    {
         SFR(BTA_CON13, 10,  1,  0);   // XOSC_HEN
    }

    SFR(BTA_CON13,  0,  1,  0); // XOSC_AACEN
    SFR(BTA_CON14, 11,  1,  0); // BTOSC_EN_SEL
    SFR(BTA_CON13,  9,  1,  1); // XOSC_EN
    SFR(BTA_CON13, 11,  1,  1); // XOSC_BT_OE
    SFR(BTA_CON13, 12,  1,  1); // XOSC_FM_OE
    SFR(BTA_CON13, 13,  1,  1); // XOSC_SYS_OE

    SFR(BTA_CON10,  1,  1,  0);   //PLL_LDO_SEL
    SFR(BTA_CON12,  0,  5,  (BTOSC_CLK/2-2));   //PLL_REFS    /(n+2)  4
    SFR(BTA_CON11,  8,  2,  3);   //PLL_ICP
    SFR(BTA_CON11, 13,  2,  0);   //PLL_FDS
    SFR(BTA_CON12, 11,  4,  0x9); //PLL_VMS      //1001
    SFR(BTA_CON12,  9,  1,  1);   //PLL_VCD_EN
    SFR(BTA_CON12, 10,  1,  1);   //PLL_VCD_OE
    SFR(BTA_CON10,  8,  5,  0x7); //PLL_ABS
    SFR(BTA_CON10, 13,  1,  0);   //PLL_ABSEN

    SFR(BTA_CON11,  0,  8,  157); //PLL_FS     /// 2440 = 2321 + 119  (2402 - 2480) : (81 159)
    SFR(BTA_CON11, 10,  3,  7);   //PLL_IVCOS
    SFR(BTA_CON10,  2,  5,  0x6); //PLL_CAPS   //<2420:F  >2420:7
    SFR(BTA_CON10, 15,  1,  1);   //PLL_BIAS

    SFR(BTA_CON10, 14,  1,  1);   //PLL_CLK_EN
    SFR(BTA_CON12,  6,  2,  3);   //PLL_TEST
    SFR(BTA_CON12,  8,  1,  0);   //PLL_TEST_EN

    SFR(BTA_CON10,  7,  1,  1);   //PLL_EN
    SFR(BTA_CON12,  5,  1,  0);   //PLL_RN
    delay(200);
    SFR(BTA_CON12,  5,  1,  1);   //PLL_RN
    delay(200);
    bt_pll_scan();

//********************** PLL *********************//
    SFR(BTA_CON20,  0,  8,  0);   //FPLL_INTE
    SFR(BTA_CON20,  8,  8,  0);   //FPLL_FRAC[7:0]
    SFR(BTA_CON21,  0, 16,  0);   //FPLL_FRAC[23:0]
    SFR(BTA_CON22,  0,  4,  0);   //FPLL_TSEL
    SFR(BTA_CON24, 13,  1,  0);   //FPLL_RSTB
    SFR(BTA_CON22,  4,  2,  0);   //FPLL_REFS
    SFR(BTA_CON22,  6,  2,  0);   //FPLL_BIAS_ADJ
    SFR(BTA_CON23,  0,  1,  0);   //FPLL_BIAS_EN
    SFR(BTA_CON22,  8,  8,  0);   //FPLL_CAPS
    SFR(BTA_CON23,  1,  1,  0);   //FPLL_CKO75
    SFR(BTA_CON23,  2,  1,  0);   //FPLL_CKOANA
    SFR(BTA_CON23,  3,  1,  0);   //FPLL_DETE_EN
    SFR(BTA_CON23,  4,  2,  0);   //FPLL_DETS
    SFR(BTA_CON23,  6,  1,  0);   //FPLL_EN
    SFR(BTA_CON23,  7,  2,  0);   //FPLL_ICP
    SFR(BTA_CON23,  9,  3,  0);   //FPLL_IVCOS
    SFR(BTA_CON23, 12,  2,  0);   //FPLL_LPFS
    SFR(BTA_CON23, 14,  2,  0);   //FPLL_PFDS
    SFR(BTA_CON24,  0,  1,  0);   //FPLL_RN
    SFR(BTA_CON24,  1,  1,  0);   //FPLL_TEST_OE
    SFR(BTA_CON24,  2,  3,  0);   //FPLL_TEST_S
    SFR(BTA_CON24,  5,  1,  0);   //FPLL_VC_DETEN
    SFR(BTA_CON24,  6,  1,  0);   //FPLL_VC_DRVEN
    SFR(BTA_CON24,  7,  1,  0);   //FPLL_VC_OE
    SFR(BTA_CON24,  8,  3,  0);   //FPLL_VC_S
    SFR(BTA_CON24, 11,  1,  0);   //FPLL_VMS

    SFR(BTA_CON24, 14,  2,  0);   //BT_LO_D32: PLL75      1:FPLL75

   printf("\n\n analog  test  ......\n\n");
  // while(1);
}


void baseband_analog(char en)
{
  /*struct ble_param *ble_fp;*/
  /*ble_fp = (struct ble_param *) (BLE_EXM_BASE_ADR + 0x100);*/
    if(en == 0)
    {
        SFR(BTA_CON14,  1,  1,  0);  // RX_LDO  TXLDO
        SFR(BTA_CON14,  2,  1,  0);  // PLL_EN
        SFR(BTA_CON14,  3,  1,  0);  // PLL_RN
        SFR(BTA_CON14,  4,  1,  0);  // PLL_CAPS  PLL_FS
        SFR(BTA_CON14,  5,  1,  0);  // RX:BP_GA  BP_GB
        SFR(BTA_CON14,  6,  1,  0);  // RX:LNA_GSEL0, LNA_ISEL, DMIX_R_S
        SFR(BTA_CON14,  7,  1,  0);  // TX:PA_G_SEL
        SFR(BTA_CON14,  8,  1,  0);  // TX:MIXER_LCSEL MIXER_GISEL
        SFR(BTA_CON14,  9,  1,  0);  // TX:DAC_BIAS DAC_GAIN
        SFR(BTA_CON14, 10,  1,  0);  // RX_TX:LNA_GSEL1  PA_CSEL
        SFR(BTA_CON14, 11,  1,  0);  // BTOSC_EN
        SFR(BTA_CON14, 12,  1,  0);  // FPLL_LDO
        SFR(BTA_CON14, 13,  1,  0);  // FPLL_EN
        SFR(BTA_CON14, 14,  1,  0);  // FPLL_RN FPLL_rstb
        SFR(BTA_CON14, 15,  1,  0);  // FPLL_SET
    }
    else
    {
        SFR(BTA_CON14,  1,  1,  1);  // RX_LDO  TXLDO
        SFR(BTA_CON14,  2,  1,  1);  // PLL_EN
        SFR(BTA_CON14,  3,  1,  1);  // PLL_RN
        SFR(BTA_CON14,  4,  1,  1);  // PLL_CAPS  PLL_FS
        SFR(BTA_CON14,  5,  1,  1);  // RX:BP_GA  BP_GB
        SFR(BTA_CON14,  6,  1,  1);  // RX:LNA_GSEL0, LNA_ISEL, DMIX_R_S
        SFR(BTA_CON14,  7,  1,  1);  // TX:PA_G_SEL
        SFR(BTA_CON14,  8,  1,  1);  // TX:MIXER_LCSEL MIXER_GISEL
        SFR(BTA_CON14,  9,  1,  1);  // TX:DAC_BIAS DAC_GAIN
        SFR(BTA_CON14, 10,  1,  1);  // RX_TX:LNA_GSEL1  PA_CSEL
        SFR(BTA_CON14, 11,  1,  0);  // BTOSC_EN
        SFR(BTA_CON14, 12,  1,  0);  // FPLL_LDO
        SFR(BTA_CON14, 13,  1,  0);  // FPLL_EN
        SFR(BTA_CON14, 14,  1,  0);  // FPLL_RN FPLL_rstb
        SFR(BTA_CON14, 15,  1,  0);  // FPLL_SET
#if 0
//        analog->tx_pwer = 0;
//        analog->tx_pwer |= 0XF;     //[3:0] PA_GSEL
//        analog->tx_pwer |= (0XF<<4);//[7:4] MIXSR_LCSEL
//        analog->tx_pwer |= (1<<8);  //[8] MIXER_GISEL
//        analog->tx_pwer |= (0<<9);  //[11:9] DAC_BIAS_SET
//        analog->tx_pwer |= (4<<12); //[14:12] DAC_GAIN_SET
//        analog->tx_pwer |= (0X1F<<16);//[20:16] PA_CSEL
//        analog->tx_pwer |= (0<<21);  //LNA_GSEL1
        (*ble_fp).RFTXCNTL0 = 0;
        (*ble_fp).RFTXCNTL1 = 0;
        (*ble_fp).RFTXCNTL0 |= 0XF;     //[3:0] PA_GSEL
        (*ble_fp).RFTXCNTL0 |= (0Xd<<4);//[7:4] MIXSR_LCSEL
        (*ble_fp).RFTXCNTL0 |= (1<<8);  //[9:8] MIXER_GISEL
        (*ble_fp).RFTXCNTL0 |= (0<<10); //[12:10] DAC_BIAS_SET
        (*ble_fp).RFTXCNTL0 |= (4<<13); //[15:13] DAC_GAIN_SET
        (*ble_fp).RFTXCNTL1 |= 0X16;    //[4:0] PA_CSEL
        (*ble_fp).RFTXCNTL1 |= (0<<5);  //[5]   LNA_GSEL1

//        analog->rx_gain0  = 0;
//        analog->rx_gain0 |= 0X8;      //[3:0]BP_GA
//        analog->rx_gain0 |= 0X8<<4;   //[7:4]BP_GB
//        analog->rx_gain0 |= (1<<8);   //LNA_GSEL0
//        analog->rx_gain0 |= (3<<9);   //[11:9]LNA_ISEL
//        analog->rx_gain0 |= (1<<12);  //[15:12]DMIX_R_S
//        analog->rx_gain0 |= (0X10<<16);//[20:16]PA_CSEL
//        analog->rx_gain0 |= (0<<21);  //LNA_GSEL1
        (*ble_fp).RFRXCNTL0 = 0;
        (*ble_fp).RFRXCNTL1 = 0;
        (*ble_fp).RFRXCNTL0 |= 0X8;      //[3:0]BP_GA
        (*ble_fp).RFRXCNTL0 |= (1<<8);   //LNA_GSEL0
        (*ble_fp).RFRXCNTL0 |= (3<<9);   //[11:9]LNA_ISEL
        (*ble_fp).RFRXCNTL0 |= (1<<12);  //[15:12]DMIX_R_S
        (*ble_fp).RFRXCNTL1 |= 0X10;    //[4:0]PA_CSEL
        (*ble_fp).RFRXCNTL1 |= (0<<5);  //[5]   LNA_GSEL1;

        (*ble_fp).RFCONFIG = 0;
        (*ble_fp).RFCONFIG |= 1;      //[3:0]XOSC_CLSEL
        (*ble_fp).RFCONFIG |= (1<<5); //[7:4]XOSC_CRSEL

//        analog->rxtxcnt  = 0;
//        analog->rxtxcnt |= 80;     //TXLDO_CNT;
//        analog->rxtxcnt |= 80<<8;  //RXLDO_CNT;
#endif
    }
}

void trim_iq(short trim_w)
{
    short data_t,m,n,i_dc,q_dc, flag, i;

    SFR(BTA_CON6, 15,  1,  0);   // EN_TRIMI
    SFR(BTA_CON7, 12,  1,  0);   // EN_TRIMQ
   // delay(200000);
    for(i = 0; i<2; i++)
    {
        SFR(BT_MDM_CON4, 0,  14, 0);    // APM_I_255
        SFR(BT_MDM_CON4, 16, 14, 0);    // APsimsimM_Q_255    255
        SFR(BT_MDM_CON5, 0,  14, 0);       // DC_I_0
        SFR(BT_MDM_CON5, 16, 14, 0);       // DC_Q_0
        SFR(BTA_CON6, 15,  1,  0);   // EN_TRIMI
        SFR(BTA_CON7, 12,  1,  0);   // EN_TRIMQ
        flag = 1;
        m = trim_w;
        n = -trim_w;
        for(data_t = -trim_w; data_t <= trim_w ; data_t += 1 )
        {
            if(i== 0)
            {
               SFR(BT_MDM_CON5, 0,  14, data_t);       // DC_I_0
            }
            else if(i== 1)
            {
               SFR(BT_MDM_CON5, 16,  14, data_t);       // DC_Q_0
            }
             delay(sys_clk*100);
            printf("data: %d LNAV: %x\n", data_t, BTA_CON15 & 0X1);
            if(BTA_CON15 & 0X1)
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
         printf("\n=========================");
         printf("0>1n %d: 1>0M %d", n, m);
    }

    printf("\ni_dc: %d q_dc: %d", i_dc, q_dc);
    SFR(BT_MDM_CON4, 0,  14, 256);    // APM_I_256
    SFR(BT_MDM_CON4, 16, 14, 256);    // APsimsimM_Q_256
    SFR(BT_MDM_CON5, 0,  14, i_dc);   // DC_I_0
    SFR(BT_MDM_CON5, 16, 14, q_dc);   // DC_Q_0
}


void trim_iq_all(void)
{
   unsigned char dat;


    SFR(BTA_CON0,  0,  1,  1);   // RX_LDO
    SFR(BTA_CON6, 14,  1,  1);   // TX_LDO_EN
    SFR(BTA_CON2,  7,  1,  1);   // LNAV_EN

    SFR(BT_MDM_CON4, 0,  14, 0);    // APM_I_256
    SFR(BT_MDM_CON4, 16, 14, 0);    // APsimsimM_Q_256
    SFR(BT_MDM_CON5, 0,  14, 0);    // DC_I_0
    SFR(BT_MDM_CON5, 16, 14, 0);    // DC_Q_0
//    SFR(BTA_CON8,  1,  1,  1);      // EN_TRIM
    delay(5000);
    dat = 0;
   while(1)
   {
         SFR(BTA_CON2,  8,  3,  dat);
          delay(sys_clk*50);
         printf("---------%d \n", BTA_CON15 & 0X1);
         if(BTA_CON15 & 0X1)      //LNAV_O
        {
            // SFR(BTA_CON2,  8,  3,  1);   //LNAV_RSEL 0 4 2 6 1 5 3 7
//            switch(dat)
//            {
//                case 0 :  dat = 1; break;
//                case 1 :  dat = 2; break;
//                case 2 :  dat = 3; break;
//                case 3 :  dat = 4; break;
//                case 4 :  dat = 5; break;
//                case 5 :  dat = 6; break;
//                case 6 :  dat = 7; break;
//                case 7 :  dat = 0; break;
//                default : dat = 0;
//            }

           dat += 1;
           if(dat > 7) dat = 0;

           printf("-------------------------------");

        }
        else
        {
            trim_iq(256);
            break;
        }
   }
   SFR(BTA_CON0,  0,  1,  0);   // RX_LDO
   SFR(BTA_CON2,  7,  1,  1);   // LNAV_EN
  //SFR(BTA_CON8,  1,  1,  1);      // EN_TRIM
}

#if 0
//===================================//
//sel: 1    auto_set agc
//sel: 0    set agc = inc (0~15)
//return: 0 set ok  1: req_dec  2: req_inc
//==================================//

char agc_set(char sel ,  char inc)
{
   static char agc_set;
   char lna_gsel, lna_isel, bp_ga, bp_gb, dato;
   struct anal_param *fp_ptr;
   fp_ptr =  (struct anal_param *) BT_SPI_CPU;

   if(!sel)
   {
       if(inc < 0)
       {
           agc_set = 0;
           dato = 1;
       }
       else if(inc>17)
       {
           agc_set = 17;
           dato = 2;
       }
       else
       {
           agc_set = inc;
           dato = 0;
       }

   }
   else
   {
//        if(BT_RSSIDAT0  <  2500) agc_set++;
//        else if(BT_RSSIDAT0 > 3800) agc_set--;
//
//         if(BT_RSSIDAT0  <  160) agc_set++;
//        else if(BT_RSSIDAT0 > 280) agc_set--;
//
          if(BT_RSSIDAT2  < 500) agc_set++;
        else if(BT_RSSIDAT2 > 1400) agc_set--;

        if(agc_set< 0)
        {
            agc_set = 0;
            dato = 1;
        }
        else if(agc_set > 17)
        {
            agc_set = 17;
            dato = 2;
        }
        else
        {
           dato = 0;
        }
   }

 //   printf("agc_set:%d  %d",BT_RSSIDAT2,  agc_set);

    switch(agc_set)
    {
                            // BP_GA  BP_GB LNA_GSEL0 LNA_ISEL  DMIX_R_S   PA_CSEL    LNA_GSEL1
        case 0 : (*fp_ptr).RXGAIN0 = 0X0 | (1<<8) | (7<<9) | (6<<12) | (0X10<<16) | (0<<21); break; //-82db
        case 1 : (*fp_ptr).RXGAIN0 = 0X1 | (1<<8) | (7<<9) | (6<<12) | (0X10<<16) | (0<<21); break; //-82db  break; //-1db
        case 2 : (*fp_ptr).RXGAIN0 = 0X2 | (1<<8) | (7<<9) | (6<<12) | (0X10<<16) | (0<<21); break; //-82db  break; //-1db
        case 3 : (*fp_ptr).RXGAIN0 = 0X3 | (1<<8) | (7<<9) | (6<<12) | (0X10<<16) | (0<<21); break; //-82db  break; //-1db
        case 4 : (*fp_ptr).RXGAIN0 = 0X4 | (1<<8) | (7<<9) | (6<<12) | (0X10<<16) | (0<<21); break; //-82db  break; //2db
        case 5 : (*fp_ptr).RXGAIN0 = 0X5 | (1<<8) | (7<<9) | (6<<12) | (0X10<<16) | (0<<21); break; //-82db  break; //5db
        case 6 : (*fp_ptr).RXGAIN0 = 0X6 | (1<<8) | (7<<9) | (6<<12) | (0X10<<16) | (0<<21); break; //-82db  break; //8db
        case 7 : (*fp_ptr).RXGAIN0 = 0X7 | (1<<8) | (7<<9) | (6<<12) | (0X10<<16) | (0<<21); break; //-82db  break; //11db
        case 8 : (*fp_ptr).RXGAIN0 = 0X8 | (1<<8) | (7<<9) | (6<<12) | (0X10<<16) | (0<<21); break; //-82db  break; //14db
        case 9 : (*fp_ptr).RXGAIN0 = 0X9 | (1<<8) | (7<<9) | (6<<12) | (0X10<<16) | (0<<21); break; //-82db  break; //17db
        case 10 : (*fp_ptr).RXGAIN0 = 0Xa |(1<<8) | (7<<9) | (6<<12) | (0X10<<16) | (0<<21); break; //-82db  break; //20db
        case 11 : (*fp_ptr).RXGAIN0 = 0Xb |(1<<8) | (7<<9) | (6<<12) | (0X10<<16) | (0<<21); break; //-82db break; //23db
        case 12 : (*fp_ptr).RXGAIN0 = 0Xc |(1<<8) | (7<<9) | (6<<12) | (0X10<<16) | (0<<21); break; //-82db  break; //26db
        case 13 : (*fp_ptr).RXGAIN0 = 0Xd |(1<<8) | (7<<9) | (6<<12) | (0X10<<16) | (0<<21); break; //-82db  break; //29db
        case 14 : (*fp_ptr).RXGAIN0 = 0Xe |(1<<8) | (7<<9) | (6<<12) | (0X10<<16) | (0<<21); break; //-82db  break; //32db
        case 15 : (*fp_ptr).RXGAIN0 = 0Xf |(1<<8) | (7<<9) | (6<<12) | (0X10<<16) | (0<<21); break; //-82db  break; //32db
        case 16 : (*fp_ptr).RXGAIN0 = 0Xf |(1<<8) | (7<<9) | (6<<12) | (0X10<<16) | (0<<21); break; //-82db  break; //32db
        case 17 : (*fp_ptr).RXGAIN0 = 0XF |(1<<8) | (7<<9) | (6<<12) | (0X10<<16) | (0<<21); break; //-82db  break; //35db
        default : (*fp_ptr).RXGAIN0 = 0XF |(1<<8) | (7<<9) | (6<<12) | (0X10<<16) | (0<<21);
    }

    return dato;
}

//===================================//
//sel 1:  inc: 0  auto_sub  inc: 1 auto_add
//sel 1:  set tx_pwr as inc (0 ~15)
//return: 0 set ok  1: min  2: max
//==================================//
char tx_pwr_set(char sel , char inc)
{
   static char pwr_set;
   char dat0, pwr_d;
   struct anal_param *fp_ptr;
   fp_ptr =  (struct anal_param *) BT_SPI_CPU;

   if(!sel)  // min
   {
      if(inc < 0)
      {
          pwr_set = 0;
          dat0 = 1;
      }
      else if(inc>8)
      {
          pwr_set = 8;
          dat0 = 2;
      }
      else
      {
          pwr_set = inc;
          dat0 = 0;
      }
   }
   else
   {
       if(inc) pwr_set++;
       else pwr_set--;
       if(pwr_set < 0)
       {
           pwr_set = 0;
           dat0 = 1;
       }
       else if (pwr_set > 8)
       {
           pwr_set = 8;
           dat0 = 2;
       }
       else
       {
           dat0 = 0;
       }
   }

   switch(pwr_set)
   {            //       PA_GSEL MIXSR_LCSEL|LNA_GSEL0|BIAS_SET|GAIN_SET|PA_CSEL|LNA_GSEL1
       case 0 : (*fp_ptr).TXPWER = 0x1 | (0xd<<4) | (1<<8) | (0<<10) | (4<<13) | (0X16<<16)|(0<<21); break; //-18.6 dBm
       case 1 : (*fp_ptr).TXPWER = 0x2 | (0xd<<4) | (1<<8) | (0<<10) | (4<<13) | (0X16<<16)|(0<<21); break; //-15   dBm
       case 2 : (*fp_ptr).TXPWER = 0x4 | (0xd<<4) | (0<<8) | (0<<10) | (4<<13) | (0X16<<16)|(0<<21); break; //-12   dBm
       case 3 : (*fp_ptr).TXPWER = 0x6 | (0xd<<4) | (0<<8) | (0<<10) | (4<<13) | (0X16<<16)|(0<<21); break; //-9.6  dBm
       case 4 : (*fp_ptr).TXPWER = 0x4 | (0xd<<4) | (1<<8) | (0<<10) | (4<<13) | (0X16<<16)|(0<<21); break; //-7.3  dBm
       case 5 : (*fp_ptr).TXPWER = 0xa | (0xd<<4) | (0<<8) | (0<<10) | (4<<13) | (0X16<<16)|(0<<21); break; //-5.3  dBm
       case 6 : (*fp_ptr).TXPWER = 0xf | (0xd<<4) | (0<<8) | (0<<10) | (4<<13) | (0X16<<16)|(0<<21); break; //-3.1  dBm
       case 7 : (*fp_ptr).TXPWER = 0xa | (0xd<<4) | (1<<8) | (0<<10) | (4<<13) | (0X16<<16)|(0<<21); break; //-0.9  dBm
       case 8 : (*fp_ptr).TXPWER = 0xf | (0xd<<4) | (1<<8) | (0<<10) | (4<<13) | (0X16<<16)|(0<<21); break; //1.3   dBm
       default : (*fp_ptr).TXPWER = 0xf | (0xd<<4) | (1<<8) | (0<<10) | (4<<13) | (0X16<<16)|(0<<21);
   }

   return dat0;
}

#endif
