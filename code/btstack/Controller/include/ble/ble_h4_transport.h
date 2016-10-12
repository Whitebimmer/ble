#ifndef BLE_H4_TRANSPORT_H
#define BLE_H4_TRANSPORT_H


#include "typedef.h"


typedef void (*h4_transport_t)(int type, u8 *packet, int len);

typedef int (*h4_flow_control)(u8 packet_type);


#define  REGISTER_H4_HOST(fn) \
	const h4_transport_t ble_h4_packet_handler \
			__attribute__((section(".ble"))) = fn


#define REGISTER_H4_CONTROLLER(fn) \
	const h4_transport_t ble_h4_send_packet \
			__attribute__((section(".ble"))) = fn


#define REGISTER_H4_CONTROLLER_FLOW_CONTROL(fn) \
    const h4_flow_control ble_h4_can_send_packet_now \
			__attribute__((section(".ble"))) = fn


extern const h4_transport_t ble_h4_send_packet;

extern const h4_transport_t ble_h4_packet_handler;

extern const h4_flow_control ble_h4_can_send_packet_now;



#endif

