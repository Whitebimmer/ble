#ifndef BLE_H4_TRANSPORT_H
#define BLE_H4_TRANSPORT_H


#include "typedef.h"


typedef void (*h4_transport_t)(int type, u8 *packet, int len);




#define  REGISTER_H4_HOST(fn) \
	const h4_transport_t ble_h4_packet_handler \
			__attribute__((section(".ble"))) = fn


#define REGISTER_H4_CONTROLLER(fn) \
	const h4_transport_t ble_h4_send_packet \
			__attribute__((section(".ble"))) = fn




extern const h4_transport_t ble_h4_send_packet;

extern const h4_transport_t ble_h4_packet_handler;





#endif

