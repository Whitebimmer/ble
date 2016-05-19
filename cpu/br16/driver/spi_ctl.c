
#include "cpu.h"

unsigned long chdat(unsigned long dat)
{
    unsigned long  buf;
    buf = 0;
    buf |= (dat >> 24 & 0xff);
    buf |= (dat >> 16 & 0xff)<<8;
    buf |= (dat >> 8 & 0xff)<<16;
    buf |= (dat  & 0xff)<<24;

    return buf;
}

void spi_int(void)
{
	 // PB5 PB6 PB7
    /*PORTB_DIR |= (1<<5);                //CS*/
    /*PORTB_OUT &=~(1<<5);*/
    /*delay(10000);*/

    SPI1_BAUD = 0X03;
    SPI1_CON |= (1<<1);   // slave_en
    SPI1_CON &= ~(1<<3);  // WIRE3EN
    SPI1_CON |= (1<<4);   // EDGE_SMP
    SPI1_CON &= ~(1<<5);  // EDGE_UD
    SPI1_CON &= ~(1<<6);  // CLK_IDST
    SPI1_CON |= (1<<12);  // SPI_DIR
    SPI1_CON |= (1<<13);  // IE
    //SPI1_CON |= (1<<0);   // en
    PORTB_DIR |=  (1<<6);  //CLK
    PORTB_OUT &= ~(1<<5);
    PORTB_DIR &= ~(1<<5); //CS
    IOMC1 |= (1<<4);
}

void spi_wr(unsigned char adr , unsigned long dat)
{
      volatile unsigned long spi_dat;

      SPI1_CON |= (1<<0);   // en
      while(!(SPI1_CON & (1<<15)));
      SPI1_CON &= ~(1<<12);  // SPI_DIR
      SPI1_BUF = adr | 0x80;
      PORTB_OUT |= (1<<5 );

      while(!(SPI1_CON & (1<<15))) ;
      SPI1_CON &= ~(1<<12);  // SPI_DIR
      spi_dat = dat;//chdat(dat);
      SPI1_ADR = &spi_dat;
      SPI1_CNT = 4;
      delay(100);
      PORTB_OUT &= ~(1<<5 );
      while(!(SPI1_CON & (1<<15))) ;
      SPI1_CON &= ~(1<<0);   // en
}

unsigned long spi_rd(unsigned char adr)
{
      volatile unsigned long spi_dat;

      SPI1_CON |= (1<<0);   // en
      while(!(SPI1_CON & (1<<15)));

      SPI1_CON &= ~(1<<12);  // SPI_DIR
      SPI1_BUF = adr & 0x7f;
      PORTB_OUT |= (1<<5 );

      while(!(SPI1_CON & (1<<15)));
      SPI1_CON |= (1<<12);  // SPI_DIR
      SPI1_ADR = &spi_dat;
      SPI1_CNT = 4;
      delay(100);
      PORTB_OUT &= ~(1<<5 );
      while(!(SPI1_CON & (1<<15)));

      SPI1_CON &= ~(1<<0);   // en
      return  spi_dat;//chdat(spi_dat);
}

void spi_wr_rg(unsigned char adr , unsigned char bit, unsigned char len, unsigned long dat)
{
    unsigned long buf;
    buf = spi_rd(adr);
    buf = buf & (~((~(0xffffffffL<<len))<<bit)) | ((dat&(~(0xffffffffL<<len)))<<bit);
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




