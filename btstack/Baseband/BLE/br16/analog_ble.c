#include "RF_ble.h"
#include "RF.h"
#include "stdarg.h"

/********************************************************************************/
/*
 *                   HW Analog  
 */

//===================================//
//sel: 1    auto_set agc
//sel: 0    set agc = inc (0~15)
//return: 0 set ok  1: req_dec  2: req_inc
//==================================//

char ble_agc_normal_set(void *fp, char sel , char inc)
{
   static char agc_set;
   char dato;
   struct ble_param * packet_ptr;
   packet_ptr = (struct ble_param *)fp;

  	if(!sel)
	{
        agc_set = inc;
	}
	else
	{
        if(packet_ptr->RSSI1  < 500){
			agc_set += 1;
		}
		else if(packet_ptr->RSSI1 > 1100){
			agc_set -= 1;
		}
	}

    if(agc_set< 0)
    {
        agc_set = 0;
    }
    else if(agc_set > 19)
    {
        agc_set = 19;
    }

    if(agc_set< 4)
    {
        dato = 1;
    }
    else if(agc_set > 16)
    {
        dato = 2;
    }
    else
    {
        dato = 0;
    }
                        //rx  pll_ivco lan_gsel1  pa_csel
    packet_ptr->RXTXSET &= ~(0XFFF<<12);
   // packet_ptr->RXTXSET |=((4<<8) | (1<<7) | (0x1f))<<12;
    switch(agc_set)
    {
                                // BP_GA  LNA_GSEL0 LNA_ISEL  DMIX_R_S  PA_CSEL   LNA_GSEL1
        case 0 : packet_ptr->RXGAIN0 = 0X1 | (0<<8) | (6<<9) | (6<<12) ;
                 packet_ptr->RXTXSET |=((4<<8) | (1<<7) | (0x1f))<<12;
                 break;
        case 1 : packet_ptr->RXGAIN0 = 0X2 | (0<<8) | (6<<9) | (6<<12) ;
                 packet_ptr->RXTXSET |=((4<<8) | (1<<7) | (0x1f))<<12;
                 break;
        case 2 : packet_ptr->RXGAIN0 = 0X3 | (0<<8) | (6<<9) | (6<<12) ;
                 packet_ptr->RXTXSET |=((4<<8) | (1<<7) | (0x1f))<<12;
                 break;
        case 3 : packet_ptr->RXGAIN0 = 0X0 | (1<<8) | (6<<9) | (6<<12) ;
                 packet_ptr->RXTXSET |=((4<<8) | (1<<7) | (0x1f))<<12;
                 break;
        case 4 : packet_ptr->RXGAIN0 = 0X1 | (1<<8) | (6<<9) | (6<<12) ;
                 packet_ptr->RXTXSET |=((4<<8) | (1<<7) | (0x1f))<<12;
                 break;
        case 5 : packet_ptr->RXGAIN0 = 0X2 | (1<<8) | (6<<9) | (6<<12) ;
                 packet_ptr->RXTXSET |=((4<<8) | (1<<7) | (0x1f))<<12;
                 break;
        case 6 : packet_ptr->RXGAIN0 = 0X3 | (1<<8) | (6<<9) | (6<<12) ;
                 packet_ptr->RXTXSET |=((4<<8) | (1<<7) | (0x1f))<<12;
                 break;
        case 7 : packet_ptr->RXGAIN0 = 0X4 | (1<<8) | (6<<9) | (6<<12) ;
                 packet_ptr->RXTXSET |=((4<<8) | (1<<7) | (0x1f))<<12;
                 break;
        case 8 : packet_ptr->RXGAIN0 = 0X5 | (1<<8) | (6<<9) | (6<<12) ;
                 packet_ptr->RXTXSET |=((4<<8) | (1<<7) | (0x1f))<<12;
                 break;
        case 9 : packet_ptr->RXGAIN0 = 0X6 | (1<<8) | (6<<9) | (6<<12) ;
                 packet_ptr->RXTXSET |=((4<<8) | (1<<7) | (0x1f))<<12;
                 break;
        case 10 : packet_ptr->RXGAIN0 = 0X7 | (1<<8) | (6<<9) | (6<<12) ;
                  packet_ptr->RXTXSET |=((4<<8) | (1<<7) | (0x1f))<<12;
                  break;
        case 11 : packet_ptr->RXGAIN0 = 0X8 | (1<<8) | (6<<9) | (6<<12) ;
                  packet_ptr->RXTXSET |=((4<<8) | (1<<7) | (0x1f))<<12;
                  break;
        case 12 : packet_ptr->RXGAIN0 = 0X8 | (1<<8) | (6<<9) | (6<<12) ;
                  packet_ptr->RXTXSET |=((4<<8) | (0<<7) | (0x1f))<<12;
                  break;
        case 13 : packet_ptr->RXGAIN0 = 0X9 | (1<<8) | (6<<9) | (6<<12) ;
                  packet_ptr->RXTXSET |=((4<<8) | (0<<7) | (0x1f))<<12;
                  break;
        case 14 : packet_ptr->RXGAIN0 = 0Xa |(1<<8) | (6<<9) | (6<<12) ;
                  packet_ptr->RXTXSET |=((4<<8) | (0<<7) | (0x1f))<<12;
                  break;
        case 15 : packet_ptr->RXGAIN0 = 0Xb |(1<<8) | (6<<9) | (6<<12) ;
                  packet_ptr->RXTXSET |=((4<<8) | (0<<7) | (0x1f))<<12;
                  break;
        case 16 : packet_ptr->RXGAIN0 = 0Xc |(1<<8) | (6<<9) | (6<<12) ;
                  packet_ptr->RXTXSET |=((4<<8) | (0<<7) | (0x1f))<<12;
                  break;
        case 17 : packet_ptr->RXGAIN0 = 0Xd |(1<<8) | (6<<9) | (6<<12) ;
                  packet_ptr->RXTXSET |=((4<<8) | (0<<7) | (0x1f))<<12;
                  break;
        case 18 : packet_ptr->RXGAIN0 = 0Xe |(1<<8) | (6<<9) | (6<<12) ;
                  packet_ptr->RXTXSET |=((4<<8) | (0<<7) | (0x1f))<<12;
                  break;
        case 19 : packet_ptr->RXGAIN0 = 0Xf |(1<<8) | (6<<9) | (6<<12) ;
                  packet_ptr->RXTXSET |=((4<<8) | (0<<7) | (0x1f))<<12;
                  break;
        default : packet_ptr->RXGAIN0 = 0Xf |(1<<8) | (6<<9) | (6<<12) ;
                  packet_ptr->RXTXSET |=((4<<8) | (0<<7) | (0x1f))<<12;
    }

    return dato;
}

char ble_agc_lowpw_set(void *fp, char sel , char inc)
{
   static char agc_set;
   char dato;
   struct ble_param * packet_ptr;
   packet_ptr = (struct ble_param *) fp;

    if(!sel)
	{
        agc_set = inc;
	}
	else
	{
        if(packet_ptr->RSSI1  < 500){
			agc_set += 1;
		}
		else if(packet_ptr->RSSI1 > 1100){
			agc_set -= 1;
		}
	}

    if(agc_set< 0)
    {
        agc_set = 0;
    }
    else if(agc_set > 19)
    {
        agc_set = 19;
    }

    if(agc_set< 4)
    {
        dato = 1;
    }
    else if(agc_set > 16)
    {
        dato = 2;
    }
    else
    {
        dato = 0;
    }

                        //rx  pll_ivco lan_gsel1  pa_csel
    packet_ptr->RXTXSET &= ~(0XFFF<<12);
   // packet_ptr->RXTXSET |=((4<<8) | (1<<7) | (0x1f))<<12;
    switch(agc_set)
    {
                                    // BP_GA  LNA_GSEL0 LNA_ISEL  DMIX_R_S
        case 0 : packet_ptr->RXGAIN0 = 0X1 | (0<<8) | (3<<9) | (6<<12);
                 packet_ptr->RXTXSET |=((4<<8) | (1<<7) | (0x1f))<<12;
                 break;
        case 1 : packet_ptr->RXGAIN0 = 0X2 | (0<<8) | (3<<9) | (6<<12);
                 packet_ptr->RXTXSET |=((4<<8) | (1<<7) | (0x1f))<<12;
                 break;
        case 2 : packet_ptr->RXGAIN0 = 0X3 | (0<<8) | (3<<9) | (6<<12);
                 packet_ptr->RXTXSET |=((4<<8) | (1<<7) | (0x1f))<<12;
                 break;
        case 3 : packet_ptr->RXGAIN0 = 0X0 | (1<<8) | (3<<9) | (6<<12);
                 packet_ptr->RXTXSET |=((4<<8) | (1<<7) | (0x1f))<<12;
                 break;
        case 4 : packet_ptr->RXGAIN0 = 0X1 | (1<<8) | (3<<9) | (6<<12);
                 packet_ptr->RXTXSET |=((4<<8) | (1<<7) | (0x1f))<<12;
                 break;
        case 5 : packet_ptr->RXGAIN0 = 0X2 | (1<<8) | (3<<9) | (6<<12);
                 packet_ptr->RXTXSET |=((4<<8) | (1<<7) | (0x1f))<<12;
                 break;
        case 6 : packet_ptr->RXGAIN0 = 0X3 | (1<<8) | (3<<9) | (6<<12);
                 packet_ptr->RXTXSET |=((4<<8) | (1<<7) | (0x1f))<<12;
                 break;
        case 7 : packet_ptr->RXGAIN0 = 0X4 | (1<<8) | (3<<9) | (6<<12);
                 packet_ptr->RXTXSET |=((4<<8) | (1<<7) | (0x1f))<<12;
                 break;
        case 8 : packet_ptr->RXGAIN0 = 0X5 | (1<<8) | (3<<9) | (6<<12);
                 packet_ptr->RXTXSET |=((4<<8) | (1<<7) | (0x1f))<<12;
                 break;
        case 9 : packet_ptr->RXGAIN0 = 0X6 | (1<<8) | (3<<9) | (6<<12);
                 packet_ptr->RXTXSET |=((4<<8) | (1<<7) | (0x1f))<<12;
                 break;
        case 10 : packet_ptr->RXGAIN0 = 0X7 | (1<<8) | (3<<9) | (6<<12);
                  packet_ptr->RXTXSET |=((4<<8) | (1<<7) | (0x1f))<<12;
                  break;
        case 11 : packet_ptr->RXGAIN0 = 0X8 | (1<<8) | (3<<9) | (6<<12);
                  packet_ptr->RXTXSET |=((4<<8) | (1<<7) | (0x1f))<<12;
                  break;
        case 12 : packet_ptr->RXGAIN0 = 0X8 | (1<<8) | (3<<9) | (6<<12);
                  packet_ptr->RXTXSET |=((4<<8) | (0<<7) | (0x1f))<<12;
                  break;
        case 13 : packet_ptr->RXGAIN0 = 0X9 | (1<<8) | (3<<9) | (6<<12);
                  packet_ptr->RXTXSET |=((4<<8) | (0<<7) | (0x1f))<<12;
                  break;
        case 14 : packet_ptr->RXGAIN0 = 0Xa |(1<<8) | (3<<9) | (6<<12);
                  packet_ptr->RXTXSET |=((4<<8) | (0<<7) | (0x1f))<<12;
                  break;
        case 15 : packet_ptr->RXGAIN0 = 0Xb |(1<<8) | (3<<9) | (6<<12);
                  packet_ptr->RXTXSET |=((4<<8) | (0<<7) | (0x1f))<<12;
                  break;
        case 16 : packet_ptr->RXGAIN0 = 0Xc |(1<<8) | (3<<9) | (6<<12);
                  packet_ptr->RXTXSET |=((4<<8) | (0<<7) | (0x1f))<<12;
                  break;
        case 17 : packet_ptr->RXGAIN0 = 0Xd |(1<<8) | (3<<9) | (6<<12);
                  packet_ptr->RXTXSET |=((4<<8) | (0<<7) | (0x1f))<<12;
                  break;
        case 18 : packet_ptr->RXGAIN0 = 0Xe |(1<<8) | (3<<9) | (6<<12);
                  packet_ptr->RXTXSET |=((4<<8) | (0<<7) | (0x1f))<<12;
                  break;
        case 19 : packet_ptr->RXGAIN0 = 0Xf |(1<<8) | (3<<9) | (6<<12);
                  packet_ptr->RXTXSET |=((4<<8) | (0<<7) | (0x1f))<<12;
                  break;
        default : packet_ptr->RXGAIN0 = 0Xf |(1<<8) | (3<<9) | (6<<12);
                  packet_ptr->RXTXSET |=((4<<8) | (0<<7) | (0x1f))<<12;
    }

    return dato;
}

//===================================//
//sel 1:  inc: 0  auto_sub  inc: 1 auto_add
//sel 1:  set tx_pwr as inc (0 ~15)
//return: 0 set ok  1: min  2: max
//==================================//
char ble_txpwr_normal_set(void *fp, char sel , char inc)
{
   static char pwr_set;
   char dat0;
   struct ble_param * packet_ptr;
   packet_ptr = (struct ble_param *) fp;

   if(!sel)  // min
   {
       pwr_set = inc;
   }
   else
   {
       if(inc) pwr_set++;
       else pwr_set--;
   }

    if(pwr_set < 0)
    {
       pwr_set = 0;
       dat0 = 1;
    }
    else if (pwr_set > 7)
    {
       pwr_set = 7;
       dat0 = 2;
    }
    else
    {
       dat0 = 0;
    }

    packet_ptr->RXTXSET &= ~(0XFFF);
                         //tx  pll_ivco lan_gsel1  pa_csel
    packet_ptr->RXTXSET |=(4<<8) | (1<<7) | (0x1f);
    switch(pwr_set)
    {                        //       PA_GSEL MIXSR_LCSEL|mix_gisel|BIAS_SET|GAIN_SET
        case 0 : packet_ptr->TXPWER = 0x1 | (0x7<<4) | (0<<9) | (0<<11) | (4<<14) | (0X4f<<16)|(0<<22)|(4<<23); break; //-30.4 dBm
        case 1 : packet_ptr->TXPWER = 0x1 | (0x7<<4) | (1<<9) | (0<<11) | (4<<14) | (0X4f<<16)|(0<<22)|(4<<23); break; //-26.1 dBm
        case 2 : packet_ptr->TXPWER = 0x1 | (0x7<<4) | (2<<9) | (0<<11) | (4<<14) | (0X4f<<16)|(0<<22)|(4<<23); break; //-23.9 dBm
        case 3 : packet_ptr->TXPWER = 0x3 | (0x7<<4) | (0<<9) | (0<<11) | (4<<14) | (0X4f<<16)|(0<<22)|(4<<23); break; //-20.7 dBm
        case 4 : packet_ptr->TXPWER = 0x2 | (0x7<<4) | (2<<9) | (0<<11) | (4<<14) | (0X4f<<16)|(0<<22)|(4<<23); break; //-17.6 dBm
        case 5 : packet_ptr->TXPWER = 0x7 | (0x7<<4) | (0<<9) | (0<<11) | (4<<14) | (0X4f<<16)|(0<<22)|(4<<23); break; //-13.7 dBm
        case 6 : packet_ptr->TXPWER = 0x4 | (0x7<<4) | (3<<9) | (0<<11) | (4<<14) | (0X4f<<16)|(0<<22)|(4<<23); break; //-10.4 dBm
        case 7 : packet_ptr->TXPWER = 0x6 | (0x7<<4) | (3<<9) | (0<<11) | (4<<14) | (0X4f<<16)|(0<<22)|(4<<23); break; //-7.2  dBm
        case 8 : packet_ptr->TXPWER = 0x0f | (0x7<<4) | (2<<9) | (0<<11) | (4<<14) | (0X4f<<16)|(0<<22)|(4<<23); break; //-3.5  dBm
        //   case 9 : packet_ptr->TX_PWER = 0x1f | (0x7<<4) | (3<<9) | (0<<11) | (6<<14) | (0X4f<<16)|(0<<22)|(4<<23); break; //0.7  dBm
        default : packet_ptr->TXPWER = 0x0f | (0x7<<4) | (2<<9) | (0<<11) | (4<<14) | (0X1f<<16)|(0<<22)|(4<<23); //
    }

   return dat0;
}

char ble_txpwr_lowpw_set(void *fp, char sel , char inc)
{
   static char pwr_set;
   char dat0;
   struct ble_param * packet_ptr;
   packet_ptr = (struct ble_param *) fp;

   if(!sel)  // min
   {
       pwr_set = inc;
   }
   else
   {
       if(inc) pwr_set++;
       else pwr_set--;
   }

    if(pwr_set < 0)
    {
       pwr_set = 0;
       dat0 = 1;
    }
    else if (pwr_set > 7)
    {
       pwr_set = 7;
       dat0 = 2;
    }
    else
    {
       dat0 = 0;
    }

    packet_ptr->RXTXSET &= ~(0XFFF);
                         //tx  pll_ivco lan_gsel1  pa_csel
    packet_ptr->RXTXSET |=(4<<8) | (1<<7) | (0x1f);

   switch(pwr_set)
   {
                         //       PA_GSEL MIXSR_LCSEL|mix_gisel|BIAS_SET|GAIN_SET
       case 0 : packet_ptr->TXPWER = 0x1 | (0x7<<4) | (0<<9) | (0<<11) | (6<<14); break; //-30.4 dBm
       case 1 : packet_ptr->TXPWER = 0x1 | (0x7<<4) | (1<<9) | (0<<11) | (6<<14); break; //-26.1   dBm
       case 2 : packet_ptr->TXPWER = 0x1 | (0x7<<4) | (2<<9) | (0<<11) | (6<<14); break; //-23.9   dBm
       case 3 : packet_ptr->TXPWER = 0x3 | (0x7<<4) | (0<<9) | (0<<11) | (6<<14); break; //-20.7  dBm
       case 4 : packet_ptr->TXPWER = 0x2 | (0x7<<4) | (2<<9) | (0<<11) | (6<<14); break; //-17.6  dBm
       case 5 : packet_ptr->TXPWER = 0x4 | (0x7<<4) | (3<<9) | (0<<11) | (6<<14); break; //-10.4  dBm
       case 6 : packet_ptr->TXPWER = 0x6 | (0x7<<4) | (3<<9) | (0<<11) | (6<<14); break; //-7.2  dBm
       case 7 : packet_ptr->TXPWER = 0x0f | (0x7<<4) | (2<<9) | (0<<11) | (6<<14); break; //-3.5   dBm
       default : packet_ptr->TXPWER = 0x0f | (0x7<<4) | (2<<9) | (0<<11) | (6<<14); break; //
   }
   return dat0;
}

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
