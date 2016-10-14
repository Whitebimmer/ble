

#ifndef _ANALOG_H_
#define _ANALOG_H_

void set_bt_trim_mode(u8 mode);
void bt_power_mode_init();
void bt_analog_init(void);
void RF_analog_init(char en);
void bta_reset_pll_bank();

void bt_iq_trim();
unsigned char get_bta_pll_bank(unsigned char set_freq);
void bt_rf_analog_init(void);








#endif
