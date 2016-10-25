/*
 * Copyright (C) 2014 BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. Any redistribution, use, or modification is done solely for
 *    personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MATTHIAS
 * RINGWALD OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Please inquire about commercial licensing options at 
 * contact@bluekitchen-gmbh.com
 *
 */

/*
 *  l2cap_le.c
 *
 *  Logical Link Control and Adaption Protocol (L2CAP) for Bluetooth Low Energy
 *
 *  Created by Matthias Ringwald on 5/16/09.
 */

#include "ble/l2cap.h"
#include "ble/hci.h"
#include "ble/hci_dump.h"
#include "ble/debug.h"
#include "ble/btstack_memory.h"

#include <stdarg.h>
#include <string.h>

#include <stdio.h>

/* #define L2CAP_DEBUG_EN */

#ifdef L2CAP_DEBUG_EN
#define l2cap_putchar(x)        putchar(x)
#define l2cap_puts(x)           puts(x)
#define l2cap_u32hex(x)         put_u32hex(x)
#define l2cap_u8hex(x)          put_u8hex(x)
#define l2cap_buf(x,y)          printf_buf(x,y)
#define l2cap_printf            printf
#else
#define l2cap_putchar(...)
#define l2cap_puts(...)
#define l2cap_u32hex(...)
#define l2cap_u8hex(...)
#define l2cap_buf(...)
#define l2cap_printf(...)
#endif

static void l2cap_packet_handler(uint8_t packet_type, uint8_t *packet, uint16_t size);

static void (*packet_handler) (void * connection, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static btstack_packet_handler_t attribute_protocol_packet_handler SEC(.btmem_highly_available);
static btstack_packet_handler_t security_protocol_packet_handler SEC(.btmem_highly_available);

void le_l2cap_init(){
    
    packet_handler = NULL;
    attribute_protocol_packet_handler = NULL;
    security_protocol_packet_handler = NULL;
    
    // 
    // register callback with HCI
    //
    le_hci_register_packet_handler(l2cap_packet_handler);
#if 0
    hci_connectable_control(0); // no services yet
#endif
}

static uint16_t l2cap_max_mtu(void){
    return HCI_ACL_PAYLOAD_SIZE - L2CAP_HEADER_SIZE;
}

uint16_t l2cap_max_le_mtu(){
    return l2cap_max_mtu();
}

/** Register L2CAP packet handlers */
void le_l2cap_register_packet_handler(void (*handler)(void * connection, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)){
    packet_handler = handler;
}

int  le_l2cap_can_send_fixed_channel_packet_now(uint16_t handle){
    return le_hci_can_send_acl_packet_now(handle);
}
// @deprecated
int le_l2cap_can_send_connectionless_packet_now(void){
    // TODO provide real handle
    return le_l2cap_can_send_fixed_channel_packet_now(0x1234);
}


uint8_t *le_l2cap_get_outgoing_buffer(void){
    return le_hci_get_outgoing_packet_buffer() + COMPLETE_L2CAP_HEADER; // 8 bytes
}

int le_l2cap_reserve_packet_buffer(void){
    return le_hci_reserve_packet_buffer();
}

void le_l2cap_release_packet_buffer(void){
    le_hci_release_packet_buffer();
}

int le_l2cap_send_prepared_connectionless(uint16_t handle, uint16_t cid, uint16_t len){
    
    if (!le_hci_is_packet_buffer_reserved()){
        log_error("l2cap_send_prepared_connectionless called without reserving packet first");
        return BTSTACK_ACL_BUFFERS_FULL;
    }

    if (!le_hci_can_send_prepared_acl_packet_now(handle)){
        log_info("l2cap_send_prepared_connectionless handle %u,, cid %u, cannot send", handle, cid);
        return BTSTACK_ACL_BUFFERS_FULL;
    }
    
    log_debug("l2cap_send_prepared_connectionless handle %u, cid %u", handle, cid);
    
    uint8_t *acl_buffer = le_hci_get_outgoing_packet_buffer();

    // 0 - Connection handle : PB=00 : BC=00 
    bt_store_16(acl_buffer, 0, handle | (0 << 12) | (0 << 14));
    // 2 - ACL length
    bt_store_16(acl_buffer, 2,  len + 4);
    // 4 - L2CAP packet length
    bt_store_16(acl_buffer, 4,  len + 0);
    // 6 - L2CAP channel DEST
    bt_store_16(acl_buffer, 6, cid);    
    // send
    int err = le_hci_send_acl_packet_buffer(len+8);
	/* l2cap_puts("l2cap_send_acl_exit\n"); */
        
    return err;
}

int le_l2cap_send_connectionless(uint16_t handle, uint16_t cid, uint8_t *data, uint16_t len){

    if (!le_hci_can_send_acl_packet_now(handle)){
        log_info("l2cap_send_connectionless cid %u, cannot send", cid);
        return BTSTACK_ACL_BUFFERS_FULL;
    }

    le_hci_reserve_packet_buffer();
    uint8_t *acl_buffer = le_hci_get_outgoing_packet_buffer();

    memcpy(&acl_buffer[8], data, len);

    return le_l2cap_send_prepared_connectionless(handle, cid, len);
}

static void l2cap_event_handler( uint8_t *packet, uint16_t size ){
    
    // pass on
    if (packet_handler) {
        /* l2cap_puts("l2cap_event_handler\n"); */
        (*packet_handler)(NULL, HCI_EVENT_PACKET, 0, packet, size);
    }
    if (attribute_protocol_packet_handler){
        /* l2cap_puts("l2cap_attribute_handler\n"); */
        l2cap_puts("att_event\n");
        (*attribute_protocol_packet_handler)(HCI_EVENT_PACKET, 0, packet, size);
    } 
    if (security_protocol_packet_handler) {
        /* l2cap_puts("l2cap_security_handler\n"); */
        l2cap_puts("sm_event\n");
        (*security_protocol_packet_handler)(HCI_EVENT_PACKET, 0, packet, size);
    }
}

static void l2cap_acl_handler( uint8_t *packet, uint16_t size ){
        
    // Get Channel ID
    uint16_t channel_id = READ_L2CAP_CHANNEL_ID(packet); 
    hci_con_handle_t handle = READ_ACL_CONNECTION_HANDLE(packet);
    
    switch (channel_id) {
            
        case L2CAP_CID_ATTRIBUTE_PROTOCOL:
            if (attribute_protocol_packet_handler) {
                l2cap_puts("att_acl_packet\n");
                (*attribute_protocol_packet_handler)(ATT_DATA_PACKET, handle, &packet[COMPLETE_L2CAP_HEADER], size-COMPLETE_L2CAP_HEADER);
            }
            break;

        case L2CAP_CID_SECURITY_MANAGER_PROTOCOL:
            if (security_protocol_packet_handler) {
                l2cap_puts("sm_acl_packet\n");
                (*security_protocol_packet_handler)(SM_DATA_PACKET, handle, &packet[COMPLETE_L2CAP_HEADER], size-COMPLETE_L2CAP_HEADER);
            }
            break;
        case 0xff:
            l2cap_puts("********my_acl_packet********\n");
            l2cap_buf(packet, size);
            break;
            
            
        default: {
            break;
        }
    }
}

static void l2cap_packet_handler(uint8_t packet_type, uint8_t *packet, uint16_t size){
    l2cap_puts("--L2CAP PH ");
    switch (packet_type) {
        case HCI_EVENT_PACKET:
            l2cap_event_handler(packet, size);
            break;
        case HCI_ACL_DATA_PACKET:
            l2cap_acl_handler(packet, size);
            break;
        default:
            break;
    }
    l2cap_puts("\n L2CAP exit ");
    /* trig_fun(); */
}


// Bluetooth 4.0 - allow to register handler for Attribute Protocol and Security Manager Protocol
void le_l2cap_register_fixed_channel(btstack_packet_handler_t packet_handler, uint16_t channel_id) {
    switch(channel_id){
        case L2CAP_CID_ATTRIBUTE_PROTOCOL:
            attribute_protocol_packet_handler = packet_handler;
            break;
        case L2CAP_CID_SECURITY_MANAGER_PROTOCOL:
            security_protocol_packet_handler = packet_handler;
            break;
    }
}
