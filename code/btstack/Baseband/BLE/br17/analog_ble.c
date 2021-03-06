#include "RF_ble.h"
#include "analog_ble.h"
#include "RF.h"
#include "stdarg.h"

/********************************************************************************/
/*
 *                   HW Analog  
 */
char ble_agc_normal_set(void *fp, char sel , char inc);
void ble_agc_reset(void *fp)
{
    u8 agc_value = 0;

    struct ble_hw *hw = (struct ble_hw *)fp;
    struct ble_param *packet_ptr;
    packet_ptr = &hw->ble_fp;

    hw->scan_agc_set++;
    if (hw->scan_agc_set > 5) hw->scan_agc_set = 0;

    if (hw->scan_agc_set == 0 || hw->scan_agc_set == 1)
    {
        agc_value = 8;
    }
    if (hw->scan_agc_set == 2 || hw->scan_agc_set == 3)
    {
        agc_value = 21;
    }
    if (hw->scan_agc_set == 4 || hw->scan_agc_set == 5)
    {
        agc_value = 27;
    }
    ble_agc_normal_set(hw, 0, agc_value);
}

//===================================//
//sel: 1    auto_set agc
//sel: 0    set agc = inc (0~15)
//return: 0 set ok  1: req_dec  2: req_inc
//==================================//
#if 0
char ble_agc_normal_set(void *fp, char sel , char inc){}
#else
char ble_agc_normal_set(void *fp, char sel , char inc)
{
    struct ble_hw *hw = (struct ble_hw *)fp;
   /* static char agc_set; */
   char dato;
   /* static unsigned short agc_buf1, agc_buf2; */
   struct ble_param * packet_ptr;
   packet_ptr = &hw->ble_fp;

    dato = 0;
  	if(!sel)
	{
        hw->agc_set = inc;
        hw->agc_buf1 = 800;
        hw->agc_buf2 = 800;
	}
	else
    {
        hw->agc_buf2 = (hw->agc_buf2 + packet_ptr->RSSI1)/2;

        /* put_u16hex(packet_ptr->RSSI1);puts("/"); */

	  if(hw->agc_set < 27)                      /// LNA && DMIX
	  {
		   if(  hw->agc_buf2 < 611)       hw->agc_set += 1;     //-10.5
		   else if(hw->agc_buf2 > 864)    hw->agc_set -= 1;     //-7.5
		   else                      		hw->agc_set  = hw->agc_set;
	  }
	  else if(hw->agc_set == 27)
	  {
		   if(  hw->agc_buf2 < 686)       hw->agc_set += 1;     //-9.5
		   else if(hw->agc_buf2 > 969)    hw->agc_set -= 1;     //-6.5
		   else                      		hw->agc_set  = hw->agc_set;
	  }
	  else if(hw->agc_set == 28)
	  {
		   if(  hw->agc_buf2 < 770)       hw->agc_set += 1;     //-8.5
		   else if(hw->agc_buf2 > 1087)   hw->agc_set -= 1;     //-5.5
		   else                      		hw->agc_set  = hw->agc_set;
	  }
	  else if(hw->agc_set == 29)
	  {
		   if(  hw->agc_buf2 < 815)       hw->agc_set += 1;     //-8
		   else if(hw->agc_buf2 > 1152)   hw->agc_set -= 1;     //-5
		   else                      		hw->agc_set  = hw->agc_set;
	  }
	  else
	  {
		   if(  hw->agc_buf2 < 864)       hw->agc_set += 1;     //-7.5
		   else if(hw->agc_buf2 > 1220)   hw->agc_set -= 1;     //-4.5
		   else                      		hw->agc_set  = hw->agc_set;
	  }
	}

    if(hw->agc_set< 0)        hw->agc_set = 0;
    else if(hw->agc_set > 30) hw->agc_set = 30;
    else                  hw->agc_set  = hw->agc_set;

    if(hw->agc_set< 9)        dato = 1;
    else if(hw->agc_set > 28) dato = 2;
    else                  dato = 0;

    switch(hw->agc_set)
    {
                                   /// BP_GA LNA_IADJ LNA_GSEL0 LNA_ISEL  DMIX_R_S               ///  PA_CSEL LNA_GSEL1 PLL_IVCOS BG_ISEL
        case 0 :  packet_ptr->RXGAIN0 = 0X0 | (3<<4) | (0<<8) | (0<<9) | (0<<12); packet_ptr->RXSET = (0X1F) |  (1<<7) | (3<<8) | (0X1<<11);
                  break; //-10.7   175
        case 1 :  packet_ptr->RXGAIN0 = 0X0 | (3<<4) | (0<<8) | (1<<9) | (0<<12); packet_ptr->RXSET = (0X1F) |  (1<<7) | (3<<8) | (0X1<<11);
                  break; //-11.5
        case 2 :  packet_ptr->RXGAIN0 = 0X0 | (3<<4) | (0<<8) | (2<<9) | (0<<12); packet_ptr->RXSET = (0X1F) |  (1<<7) | (3<<8) | (0X1<<11);
                  break; //-12.3
        case 3 :  packet_ptr->RXGAIN0 = 0X0 | (1<<4) | (0<<8) | (2<<9) | (0<<12); packet_ptr->RXSET = (0X1F) |  (1<<7) | (3<<8) | (0X1<<11);
                  break; //-14.8
        case 4 :  packet_ptr->RXGAIN0 = 0X1 | (1<<4) | (0<<8) | (2<<9) | (0<<12); packet_ptr->RXSET = (0X1F) |  (1<<7) | (3<<8) | (0X1<<11);
                  break; //-17.7
        case 5 :  packet_ptr->RXGAIN0 = 0X2 | (1<<4) | (0<<8) | (2<<9) | (0<<12); packet_ptr->RXSET = (0X1F) |  (1<<7) | (3<<8) | (0X1<<11);
                 break; //-20.7
        case 6 :  packet_ptr->RXGAIN0 = 0X3 | (1<<4) | (0<<8) | (2<<9) | (0<<12); packet_ptr->RXSET = (0X1F) |  (1<<7) | (3<<8) | (0X1<<11);
                 break; //-23.5
        case 7 :  packet_ptr->RXGAIN0 = 0X4 | (1<<4) | (0<<8) | (2<<9) | (0<<12); packet_ptr->RXSET = (0X1F) | (1<<7) | (3<<8) | (0X1<<11);
                 break; //-26.8
        case 8 :  packet_ptr->RXGAIN0 = 0X5 | (1<<4) | (0<<8) | (2<<9) | (0<<12); packet_ptr->RXSET = (0X1F) | (1<<7) | (3<<8) | (0X1<<11);
                 break; //-29.5   175
        case 9 :  packet_ptr->RXGAIN0 = 0X4 | (1<<4) | (0<<8) | (2<<9) | (1<<12); packet_ptr->RXSET = (0X1F) | (1<<7) | (3<<8) | (0X1<<11);
                 break;//-31.5
        case 10 : packet_ptr->RXGAIN0 = 0X4 | (1<<4) | (0<<8) | (3<<9) | (1<<12); packet_ptr->RXSET = (0X1F) | (1<<7) | (3<<8) | (0X1<<11);
                 break;//-32.5
        case 11 : packet_ptr->RXGAIN0 = 0X4 | (1<<4) | (0<<8) | (2<<9) | (2<<12); packet_ptr->RXSET = (0X1F) | (1<<7) | (3<<8) | (0X1<<11);
                 break;//-34.6
        case 12 : packet_ptr->RXGAIN0 = 0X4 | (1<<4) | (0<<8) | (2<<9) | (3<<12); packet_ptr->RXSET = (0X1F) | (1<<7) | (3<<8) | (0X1<<11);
                 break;//-36.8
        case 13 : packet_ptr->RXGAIN0 = 0X4 | (1<<4) | (0<<8) | (2<<9) | (4<<12); packet_ptr->RXSET = (0X1F) | (1<<7) | (3<<8) | (0X1<<11);
                 break;//-38.5
        case 14 : packet_ptr->RXGAIN0 = 0X4 | (1<<4) | (0<<8) | (2<<9) | (5<<12); packet_ptr->RXSET = (0X1F) | (1<<7) | (3<<8) | (0X1<<11);
                 break;//-39.8
        case 15 : packet_ptr->RXGAIN0 = 0X4 | (1<<4) | (0<<8) | (2<<9) | (7<<12); packet_ptr->RXSET = (0X1F) | (1<<7) | (3<<8) | (0X1<<11);
                 break;//-41.9   180
        case 16 : packet_ptr->RXGAIN0 = 0X5 | (1<<4) | (0<<8) | (2<<9) | (7<<12); packet_ptr->RXSET = (0X1F) | (1<<7) | (3<<8) | (0X1<<11);
                 break;//-44.8
        case 17 : packet_ptr->RXGAIN0 = 0X6 | (1<<4) | (0<<8) | (2<<9) | (7<<12); packet_ptr->RXSET = (0X1F) | (1<<7) | (3<<8) | (0X1<<11);
                 break;//-47.8
        case 18 : packet_ptr->RXGAIN0 = 0X7 | (1<<4) | (0<<8) | (2<<9) | (7<<12); packet_ptr->RXSET = (0X1F) | (1<<7) | (3<<8) | (0X1<<11);
                 break;//-50.8
        case 19 : packet_ptr->RXGAIN0 = 0X8 | (1<<4) | (0<<8) | (2<<9) | (7<<12); packet_ptr->RXSET = (0X1F) | (1<<7) | (3<<8) | (0X1<<11);
                 break;//-53.6
        case 20 : packet_ptr->RXGAIN0 = 0X8 | (0<<4) | (0<<8) | (3<<9) | (7<<12); packet_ptr->RXSET = (0X1F) | (1<<7) | (3<<8) | (0X1<<11);
                 break;//-56.4   200
        case 21 : packet_ptr->RXGAIN0 = 0X4 | (0<<4) | (1<<8) | (3<<9) | (7<<12); packet_ptr->RXSET = (0X1F) | (1<<7) | (3<<8) | (0X1<<11);
                 break;//-58.3
        case 22 : packet_ptr->RXGAIN0 = 0X5 | (0<<4) | (1<<8) | (3<<9) | (7<<12); packet_ptr->RXSET = (0X1F) | (1<<7) | (3<<8) | (0X1<<11);
                 break;//-61.3
        case 23 : packet_ptr->RXGAIN0 = 0X6 | (0<<4) | (1<<8) | (3<<9) | (7<<12); packet_ptr->RXSET = (0X1F) | (1<<7) | (3<<8) | (0X1<<11);
                 break;//-64.3
        case 24 : packet_ptr->RXGAIN0 = 0X7 | (0<<4) | (1<<8) | (3<<9) | (7<<12); packet_ptr->RXSET = (0X1F) | (1<<7) | (3<<8) | (0X1<<11);
                 break;//-67.3
        case 25 : packet_ptr->RXGAIN0 = 0X8 | (0<<4) | (1<<8) | (3<<9) | (7<<12); packet_ptr->RXSET = (0X1F) | (1<<7) | (3<<8) | (0X1<<11);
                 break;//-70    250
        case 26 : packet_ptr->RXGAIN0 = 0X8 | (0<<4) | (1<<8) | (5<<9) | (7<<12); packet_ptr->RXSET = (0X1F) | (1<<7) | (3<<8) | (0X1<<11);
                 break;//-71.3
        case 27 : packet_ptr->RXGAIN0 = 0X8 | (0<<4) | (1<<8) | (5<<9) | (7<<12); packet_ptr->RXSET = (0X1F) | (0<<7) | (3<<8) | (0X1<<11);
                 break;//-74.2
        case 28 : packet_ptr->RXGAIN0 = 0X9 | (0<<4) | (1<<8) | (5<<9) | (7<<12); packet_ptr->RXSET = (0X1F) | (0<<7) | (3<<8) | (0X1<<11);
                 break;//-77.2
        case 29 : packet_ptr->RXGAIN0 = 0XA | (0<<4) | (1<<8) | (5<<9) | (7<<12); packet_ptr->RXSET = (0X1F) | (0<<7) | (3<<8) | (0X1<<11);
                  break;//-80.2
        case 30 : packet_ptr->RXGAIN0 = 0XB | (0<<4) | (1<<8) | (5<<9) | (7<<12); packet_ptr->RXSET = (0X1F) | (0<<7) | (3<<8) | (0X1<<11);
                  break;//-83.1  420
        case 31 : packet_ptr->RXGAIN0 = 0XC | (0<<4) | (1<<8) | (5<<9) | (7<<12); packet_ptr->RXSET = (0X1F) | (0<<7) | (3<<8) | (0X1<<11);
                  break;//-86.3
        case 32 : packet_ptr->RXGAIN0 = 0XD | (0<<4) | (1<<8) | (5<<9) | (7<<12); packet_ptr->RXSET = (0X1F) | (0<<7) | (3<<8) | (0X1<<11);
                  break;//-89.3
        case 33 : packet_ptr->RXGAIN0 = 0XE | (0<<4) | (1<<8) | (5<<9) | (7<<12); packet_ptr->RXSET = (0X1F) | (0<<7) | (3<<8) | (0X1<<11);
                  break;//-92.9
        case 34 : packet_ptr->RXGAIN0 = 0XF | (0<<4) | (1<<8) | (6<<9) | (7<<12); packet_ptr->RXSET = (0X1F) | (0<<7) | (3<<8) | (0X1<<11);
                  break;//-96.1
        default : packet_ptr->RXGAIN0 = 0XF | (0<<4) | (1<<8) | (5<<9) | (7<<12); packet_ptr->RXSET = (0X1F) | (0<<7) | (3<<8) | (0X1<<11);
    }

//printf("\n%d", agc_set);
    return dato;
}
#endif


//===================================//
//sel 1:  inc: 0  auto_sub  inc: 1 auto_add
//sel 1:  set tx_pwr as inc (0 ~15)
//return: 0 set ok  1: min  2: max
//==================================//
#if 0
char ble_txpwr_normal_set(void *fp, char sel , char inc){}
#else
char ble_txpwr_normal_set(void *fp, char sel , char inc)
{
   /* static char pwr_set; */
    struct ble_hw *hw = (struct ble_hw*)fp;
   char dat0;
   struct ble_param * packet_ptr;
   packet_ptr = &hw->ble_fp;

   if(!sel)  // min
   {
       hw->pwr_set = inc;
   }
   else
   {
       if(inc) hw->pwr_set++;
       else hw->pwr_set--;
   }

    if(hw->pwr_set < 0)
    {
       hw->pwr_set = 0;
       dat0 = 1;
    }
    else if (hw->pwr_set > 7)
    {
       hw->pwr_set = 7;
       dat0 = 2;
    }
    else
    {
       dat0 = 0;
    }

    switch(hw->pwr_set)
   {                        ///       PA_GSEL MIXSR_LCSEL|mix_gisel|BIAS_SET|GAIN_SET                    PA_CSEL LNA_GSEL1 PLL_IVCOS  BG_ISEL
       case 0 : packet_ptr->TXPWER =  0X1 | (0Xe<<5) | (0<<9) | (0<<11) | (4<<14); packet_ptr->TXSET = (0X4f) | (0<<7) | (3<<8) | (0X1<<11);break; //-24.9  dBm
       case 1 : packet_ptr->TXPWER =  0X2 | (0Xe<<5) | (0<<9) | (0<<11) | (4<<14); packet_ptr->TXSET = (0X4f) | (0<<7) | (3<<8) | (0X1<<11);break; //-19.5  dBm
       case 2 : packet_ptr->TXPWER =  0X3 | (0Xe<<5) | (0<<9) | (0<<11) | (4<<14); packet_ptr->TXSET = (0X4f) | (0<<7) | (3<<8) | (0X1<<11);break; //-15.7  dBm
       case 3 : packet_ptr->TXPWER =  0X5 | (0Xe<<5) | (0<<9) | (0<<11) | (4<<14); packet_ptr->TXSET = (0X4f) | (0<<7) | (3<<8) | (0X1<<11);break; //-11.5   dBm
       case 4 : packet_ptr->TXPWER =  0X9 | (0Xe<<5) | (0<<9) | (0<<11) | (4<<14); packet_ptr->TXSET = (0X4f) | (0<<7) | (3<<8) | (0X1<<11);break; //-8.6   dBm
       case 5 : packet_ptr->TXPWER =  0Xc | (0Xe<<5) | (0<<9) | (0<<11) | (4<<14); packet_ptr->TXSET = (0X4f) | (0<<7) | (3<<8) | (0X1<<11);break; //-5.6  dBm
       case 6 : packet_ptr->TXPWER =  0Xf | (0Xe<<5) | (0<<9) | (0<<11) | (4<<14); packet_ptr->TXSET = (0X4f) | (0<<7) | (3<<8) | (0X1<<11);break; //-3.2  dBm
       case 7 : packet_ptr->TXPWER = 0X1f | (0Xe<<5) | (1<<9) | (0<<11) | (4<<14); packet_ptr->TXSET = (0X4f) | (0<<7) | (3<<8) | (0X1<<11);break; //0 dBm
     //case 8 : packet_ptr->TXPWER = 0X1f | (0Xe<<5) | (1<<9) | (0<<11) | (6<<14); packet_ptr->TXSET = (0X4f) | (0<<7) | (3<<8) | (0X2<<11);break; //2.8  dBm
      default : packet_ptr->TXPWER = 0X1F | (0Xe<<5) | (1<<9) | (0<<11) | (4<<14); packet_ptr->TXSET = (0X4f) | (0<<7) | (3<<8) | (0X1<<11); //
   }

   return dat0;
}
#endif



#define  fsk_offset_1     16
#define  fsk_offset_c1    11
#define  fsk_offset_op    5
volatile short fsk_offset_dly;
volatile short offset_cnt;
void ble_fsk_offset(void *fp, char set)
{
    long buf32;
    short in,bu16, out;
    struct ble_param *packet;
    packet = (struct ble_param *)fp;


   in = BT_MDM_CON6 & 0XFFFF;
  // printf("\n%d", in);
   in = in << fsk_offset_op;
   if(set)
   {
      buf32 = (long) (in<<fsk_offset_c1) + (long) (fsk_offset_dly<<fsk_offset_1) - (long) (fsk_offset_dly<<(fsk_offset_c1+1));
      bu16 = buf32 >> fsk_offset_1;
      out = (bu16 + fsk_offset_dly)>>fsk_offset_op;
      fsk_offset_dly = bu16;

      if(offset_cnt > 50)
      {
         // printf("\n%d.", out );
         // SFR(BT_MDM_CON6, 16, 12, out);
         // SFR(BT_MDM_CON7, 16, 12, out);
          packet->MDM_SET = (0xffff & out) <<16 | (0xffff & out);          //FSK
          SFR(BT_MDM_CON1, 14, 2, 3);        // fsk carrier frequency deviation switch
          SFR(BT_MDM_CON0, 30, 2, 3);        // psk carrier frequency deviation switch
      }
      else
      {
          packet->MDM_SET = 0;
          SFR(BT_MDM_CON1, 14, 2, 0);        // fsk carrier frequency deviation switch
          SFR(BT_MDM_CON0, 30, 2, 0);        // psk carrier frequency deviation switch
          offset_cnt++;
      }
   }
   else
   {
      fsk_offset_dly = 0;
      offset_cnt = 0;
   }
}

void ble_mdm_init(void *fp)
{
    struct ble_param  *packet;
    packet = (struct ble_param *)fp;

    /* packet->MDM_SET = 0<<16 | 0;       // PSK<<16 | FSK */
    /* ble_fsk_offset(fp, 0); */
}
