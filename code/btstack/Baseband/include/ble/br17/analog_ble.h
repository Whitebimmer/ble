#ifndef ANALOG_BLE_H
#define ANALOG_BLE_H


#include "typedef.h"




char ble_agc_normal_set(void *fp, char sel , char inc);
char ble_agc_lowpw_set(void *fp, char sel , char inc);
char ble_txpwr_normal_set(void *fp, char sel , char inc);
char ble_agc_lowpw_set(void *fp, char sel , char inc);
void ble_mdm_init(void *fp);











#endif

