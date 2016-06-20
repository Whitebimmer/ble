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
    SFR(BT_MDM_CON0, 0, 1, 0);         // fsk symbol time track disable
    SFR(BT_MDM_CON0, 1, 3, 4);         // when to switch fsk mode to psk mode(in the 5uS guard)
    SFR(BT_MDM_CON0, 4, 4, 9);         // delay fsk data to fit the phase with psk data
    SFR(BT_MDM_CON0, 8, 3, 0);         // best sampling point offset
    SFR(BT_MDM_CON0, 11, 1, 0);        // psk symbol time track disable
    SFR(BT_MDM_CON0, 12, 2, 1);        // psk sync word match bit margin,  00: 6  01: 7  10: 8  11: 9
    SFR(BT_MDM_CON0, 14, 2, 1);        // fsk bandpass filter select
    SFR(BT_MDM_CON0, 16, 6, 0);        // fsk 'pmb_find' to 'stage2_find' delay, time = (n * 0.125)uS
    SFR(BT_MDM_CON0, 22, 1, 1);        // tx i/q channel exchange
    SFR(BT_MDM_CON0, 23, 1, 0);        // rx i/q channel exchange
    SFR(BT_MDM_CON0, 24, 1, 0);        // tx delta-sigma modulator level select,  0: 64 level  1: 32 level
    SFR(BT_MDM_CON0, 25, 1, 0);        // delta-sigma modulator enable mode,  1: always enable
    SFR(BT_MDM_CON0, 26, 3, 5);        // rx dc-cancelling setting
    SFR(BT_MDM_CON0, 29, 1, 0);        // fsk/psk amplitude separate, 1: fsk/psk use different amplitude
    SFR(BT_MDM_CON0, 30, 2, 0);        // psk carrier frequency deviation switch
    SFR(BT_MDM_CON1, 0, 8, 120);        // fsk 'stage0_en' block time select, time = (n)uS
    SFR(BT_MDM_CON1, 8, 4, 13);        // fsk 'stage0_find' to 'stage1_find' delay, time = (n * 0.125)uS
    SFR(BT_MDM_CON1, 12, 1, 0);        // fsk V2.X FSK BT add 8KHz
    SFR(BT_MDM_CON1, 13, 1, 1);        // fsk preamble power compare 5(or 4) times
    SFR(BT_MDM_CON1, 14, 2, 0);        // fsk carrier frequency deviation switch
    SFR(BT_MDM_CON1, 16, 1, 0);        // intermediate frequency enable
    SFR(BT_MDM_CON1, 17, 3, 0);        // intermediate frequency gate select, 0: 256uS  1: 512uS  ....  7: 32768uS
    SFR(BT_MDM_CON1, 20, 4, 7);        // fsk sync word error bit margin
    SFR(BT_MDM_CON1, 24, 3, 4);        // psk first bests length, length = (best_len + 8)uS
    SFR(BT_MDM_CON1, 27, 1, 0);        // analog test enable(for testing only)
    SFR(BT_MDM_CON1, 28, 2, 0);        // fsk frequency offset compensation disable,  01: disable preamble  10: disable triler  11: disable both
    SFR(BT_MDM_CON1, 30, 1, 1);        // d8psk mode enable, 0: d8psk mode is disable
    SFR(BT_MDM_CON1, 31, 1, 0);        // dsf2psk agc select, 0: use fsk filter(rssi2)  1: use psk filter(rssi3)
    SFR(BT_MDM_CON2, 0, 6, 60);        // fsk preamble amplitude threshold
    SFR(BT_MDM_CON2, 6, 4, 11);         // fsk preamble number threshold
    SFR(BT_MDM_CON2, 10, 1, 0);        // fsk preamble disable
    SFR(BT_MDM_CON2, 11, 1, 0);        // v4.x fsk BT ADD 8kHZ
    SFR(BT_MDM_CON2, 12, 2, 3);        // fsk syncword mode,  01: by best sample phase  10: by syncword window  11: by both
    SFR(BT_MDM_CON2, 16, 4, 1);        // BLE V4.X FSK SYNC WORD ERROR BIT MARGIN

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

    SFR(BT_MDM_CON1, 0, 8, 120);        // fsk preamble block time select, time = (n)uS
    SFR(BT_MDM_CON6, 16, 12, 0);
    SFR(BT_MDM_CON7, 16, 12, 0);
    SFR(BT_MDM_CON0, 30, 2, 0);        // fsk carrier frequency deviation switch
    SFR(BT_MDM_CON1, 14, 2, 0);        // psk carrier frequency deviation switch

    BT_BSB_EDR  = (1<<7) | 57;         // rxmch  to edr sync
    ///调制解调器寄存器相关设置
    ble_mdm_init(NULL);
}



void RF_init()
{
    SFR(CLK_CON1, 14, 2, 0);
    delay(100);
    SFR(WL_CON0, 7, 1, 1);
    SFR(WL_CON0,12, 4, 0);
    SFR(WL_CON0,16, 4, 3);
    SFR(WL_CON0,20, 2, 0);


    WL_CON1 = 0;
    WL_CON2 = 0;
	delay(1000);

    SFR(WL_CON0, 0, 1, 1);
	delay(1000);
    BT_BSB_CON |= BIT(10);

	/* bt_puts("rf_pll_init 1\n"); */

    BT_BSB_CON |= BIT(14)|BIT(15)|BIT(13)|(1<<9)|(1<<6)|(1<<3)|BIT(8)|BIT(2)|BIT(1)|BIT(0);

    /* BT_LP_CON |= BIT(0);	//[>Dual mode<] */

	/* bt_puts("rf_pll_init 2\n"); */
	RF_mdm_init();
    //rf_debug();
    bt_rf_freq_init();
	/* bt_puts("rf_pll_init 3\n"); */
	//bt_puts("rf_pll_init\n");
	if(bt_power_is_poweroff_post()) {
		return;
	}
	/* bt_puts("rf_pll_init 4\n"); */

    //map to fpga or chip
    bt_rf_analog_init();
	/* bt_puts("rf_pll_init 5\n"); */

    ble_rf_init();
}

///在main 开始的时候调用，trim 和读取记忆的数值
void bt_process_before_init()
{
    rf_debug();
	
}





