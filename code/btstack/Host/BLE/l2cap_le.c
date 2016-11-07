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

#include "l2cap.h"
#include "hci.h"
#include "ble/hci_dump.h"
#include "ble/debug.h"
#include "ble/btstack_memory.h"

#include <stdarg.h>
#include <string.h>

#include <stdio.h>

#define L2CAP_DEBUG_EN

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

// nr of buffered acl packets in outgoing queue to get max performance 
#define NR_BUFFERED_ACL_PACKETS 3

// used to cache l2cap rejects, echo, and informational requests
#define NR_PENDING_SIGNALING_RESPONSES 3

// internal table
#define L2CAP_FIXED_CHANNEL_TABLE_INDEX_ATTRIBUTE_PROTOCOL          0
#define L2CAP_FIXED_CHANNEL_TABLE_INDEX_SECURITY_MANAGER_PROTOCOL   1
#define L2CAP_FIXED_CHANNEL_TABLE_INDEX_CONNECTIONLESS_CHANNEL      2
#define L2CAP_FIXED_CHANNEL_TABLE_SIZE                              (L2CAP_FIXED_CHANNEL_TABLE_INDEX_CONNECTIONLESS_CHANNEL+1)

static void l2cap_packet_handler(uint8_t packet_type, uint8_t *packet, uint16_t size);

static void l2cap_hci_event_handler(uint8_t packet_type, uint16_t cid, uint8_t *packet, uint16_t size);
static void l2cap_acl_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size );

static btstack_packet_handler_t l2cap_event_packet_handler SEC(.btmem_highly_available);
static btstack_packet_callback_registration_t hci_event_callback_registration SEC(.btmem_highly_available);

typedef struct l2cap_fixed_channel {
    btstack_packet_handler_t callback;
    uint8_t waiting_for_can_send_now;
} l2cap_fixed_channel_t;

static linked_list_t l2cap_channels;
static linked_list_t l2cap_services;
static linked_list_t l2cap_le_channels;
static linked_list_t l2cap_le_services;

// used to cache l2cap rejects, echo, and informational requests
static l2cap_signaling_response_t signaling_responses[NR_PENDING_SIGNALING_RESPONSES];
static int signaling_responses_pending;


static btstack_packet_handler_t l2cap_event_packet_handler;
/* static btstack_packet_handler_t attribute_protocol_packet_handler SEC(.btmem_highly_available); */
/* static btstack_packet_handler_t security_protocol_packet_handler SEC(.btmem_highly_available); */
static l2cap_fixed_channel_t fixed_channels[L2CAP_FIXED_CHANNEL_TABLE_SIZE] SEC(.btmem_highly_available);

static uint16_t l2cap_fixed_channel_table_channel_id_for_index(int index){
    switch (index){
        case L2CAP_FIXED_CHANNEL_TABLE_INDEX_ATTRIBUTE_PROTOCOL:
            return L2CAP_CID_ATTRIBUTE_PROTOCOL;
        case L2CAP_FIXED_CHANNEL_TABLE_INDEX_SECURITY_MANAGER_PROTOCOL:
            return L2CAP_CID_SECURITY_MANAGER_PROTOCOL;
        case L2CAP_FIXED_CHANNEL_TABLE_INDEX_CONNECTIONLESS_CHANNEL:
            return L2CAP_CID_CONNECTIONLESS_CHANNEL;
        default:
            return 0;
    }  
}
static int l2cap_fixed_channel_table_index_for_channel_id(uint16_t channel_id){
    switch (channel_id){
        case L2CAP_CID_ATTRIBUTE_PROTOCOL:
            return L2CAP_FIXED_CHANNEL_TABLE_INDEX_ATTRIBUTE_PROTOCOL;
        case L2CAP_CID_SECURITY_MANAGER_PROTOCOL:
            return  L2CAP_FIXED_CHANNEL_TABLE_INDEX_SECURITY_MANAGER_PROTOCOL;
        case L2CAP_CID_CONNECTIONLESS_CHANNEL:
            return  L2CAP_FIXED_CHANNEL_TABLE_INDEX_CONNECTIONLESS_CHANNEL;
        default:
            return -1;
        }
}

static int l2cap_fixed_channel_table_index_is_le(int index){
    if (index == L2CAP_CID_CONNECTIONLESS_CHANNEL) return 0;
    return 1;
}
void le_l2cap_init(){
    
    signaling_responses_pending = 0;
    
    l2cap_channels = NULL;
    l2cap_services = NULL;
    l2cap_le_services = NULL;
    l2cap_le_channels = NULL;

    l2cap_event_packet_handler = NULL;
    memset(fixed_channels, 0, sizeof(fixed_channels));
    // 
    // register callback with HCI
    //
    /* hci_event_callback_registration.callback = &l2cap_hci_event_handler; */
    /* hci_add_event_handler(&hci_event_callback_registration); */

    hci_register_acl_packet_handler(&l2cap_acl_handler);
#if 0
    hci_connectable_control(0); // no services yet
#endif
}

void l2cap_register_packet_handler(void (*handler)(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)){
    l2cap_event_packet_handler = handler;
}

static void l2cap_emit_can_send_now(btstack_packet_handler_t packet_handler, uint16_t channel) {
    log_info("L2CAP_EVENT_CHANNEL_CAN_SEND_NOW local_cid 0x%x", channel);
    uint8_t event[4];
    event[0] = L2CAP_EVENT_CAN_SEND_NOW;
    event[1] = sizeof(event) - 2;
    bt_store_16(event, 2, channel);
    /* hci_dump_packet( HCI_EVENT_PACKET, 0, event, sizeof(event)); */
    packet_handler(HCI_EVENT_PACKET, channel, event, sizeof(event));
}

static void l2cap_notify_channel_can_send(void){
    linked_list_iterator_t it;
    linked_list_iterator_init(&it, &l2cap_channels);
    while (linked_list_iterator_has_next(&it)){
        l2cap_channel_t * channel = (l2cap_channel_t *) linked_list_iterator_next(&it);
        if (!channel->waiting_for_can_send_now) continue;
        if (!hci_can_send_acl_packet_now(channel->con_handle)) continue;
        channel->waiting_for_can_send_now = 0;
        l2cap_emit_can_send_now(channel->packet_handler, channel->local_cid);
    }

    int i;
    for (i=0;i<L2CAP_FIXED_CHANNEL_TABLE_SIZE;i++){
        if (!fixed_channels[i].callback) continue;
        if (!fixed_channels[i].waiting_for_can_send_now) continue;
        int can_send;
        if (l2cap_fixed_channel_table_index_is_le(i)){
            can_send = hci_can_send_acl_le_packet_now();
        } else {
            can_send = hci_can_send_acl_classic_packet_now();
        } 
        if (!can_send) continue;
        fixed_channels[i].waiting_for_can_send_now = 0;
        l2cap_emit_can_send_now(fixed_channels[i].callback, l2cap_fixed_channel_table_channel_id_for_index(i));
    }
}

void l2cap_request_can_send_now_event(uint16_t local_cid){
    l2cap_channel_t *channel = l2cap_get_channel_for_local_cid(local_cid);
    if (!channel) return;
    channel->waiting_for_can_send_now = 1;
    l2cap_notify_channel_can_send();
}

void l2cap_request_can_send_fix_channel_now_event(hci_con_handle_t con_handle, uint16_t channel_id){
    int index = l2cap_fixed_channel_table_index_for_channel_id(channel_id);
    if (index < 0) return;
    fixed_channels[index].waiting_for_can_send_now = 1;
    l2cap_notify_channel_can_send();
}

int  l2cap_can_send_packet_now(uint16_t local_cid){
    l2cap_channel_t *channel = l2cap_get_channel_for_local_cid(local_cid);
    if (!channel) return 0;
    return hci_can_send_acl_packet_now(channel->con_handle);
}

int  l2cap_can_send_prepared_packet_now(uint16_t local_cid){
    l2cap_channel_t *channel = l2cap_get_channel_for_local_cid(local_cid);
    if (!channel) return 0;
    return hci_can_send_prepared_acl_packet_now(channel->con_handle);
}

int  l2cap_can_send_fixed_channel_packet_now(hci_con_handle_t con_handle, uint16_t channel_id){
    return hci_can_send_acl_packet_now(con_handle);
}

static uint16_t l2cap_max_mtu(void){
    return HCI_ACL_PAYLOAD_SIZE - L2CAP_HEADER_SIZE;
}

uint16_t l2cap_max_le_mtu(){
    return l2cap_max_mtu();
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

static void l2cap_hci_event_handler(uint8_t packet_type, uint16_t cid, uint8_t *packet, uint16_t size)
{
    l2cap_puts("Layer - l2cap_hci_event_handler : ");
            
    /*-TODO-*/

}

// MARK: L2CAP_RUN
// process outstanding signaling tasks
static void l2cap_run(void){
}

static void l2cap_emit_connection_parameter_update_response(hci_con_handle_t con_handle, uint16_t result){
    uint8_t event[6];
    event[0] = L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE;
    event[1] = 4;
    bt_store_16(event, 2, con_handle);
    bt_store_16(event, 4, result);
    /* hci_dump_packet( HCI_EVENT_PACKET, 0, event, sizeof(event)); */
    if (!l2cap_event_packet_handler) return;
    (*l2cap_event_packet_handler)(HCI_EVENT_PACKET, 0, event, sizeof(event));
}

static void l2cap_register_signaling_response(hci_con_handle_t handle, uint8_t code, uint8_t sig_id, uint16_t data){
    // Vol 3, Part A, 4.3: "The DCID and SCID fields shall be ignored when the result field indi- cates the connection was refused."
    if (signaling_responses_pending < NR_PENDING_SIGNALING_RESPONSES) {
        signaling_responses[signaling_responses_pending].handle = handle;
        signaling_responses[signaling_responses_pending].code = code;
        signaling_responses[signaling_responses_pending].sig_id = sig_id;
        signaling_responses[signaling_responses_pending].data = data;
        signaling_responses_pending++;
        l2cap_run();
    }
}

static void l2cap_acl_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size ){
        
    // Get Channel ID
    uint16_t channel_id = READ_L2CAP_CHANNEL_ID(packet); 
    hci_con_handle_t handle = READ_ACL_CONNECTION_HANDLE(packet);

    l2cap_puts("Layer - l2cap_acl_handler: ");
    
    switch (channel_id) {
            
        case L2CAP_CID_ATTRIBUTE_PROTOCOL:
            l2cap_puts("L2CAP_CID_ATTRIBUTE_PROTOCOL\n");
            if (fixed_channels[L2CAP_FIXED_CHANNEL_TABLE_INDEX_ATTRIBUTE_PROTOCOL].callback)
            {
                (*fixed_channels[L2CAP_FIXED_CHANNEL_TABLE_INDEX_ATTRIBUTE_PROTOCOL].callback)(ATT_DATA_PACKET, handle, &packet[COMPLETE_L2CAP_HEADER], size-COMPLETE_L2CAP_HEADER);
            }
            break;

        case L2CAP_CID_SECURITY_MANAGER_PROTOCOL:
            l2cap_puts("L2CAP_CID_SECURITY_MANAGER_PROTOCOL\n");
            if (fixed_channels[L2CAP_FIXED_CHANNEL_TABLE_INDEX_SECURITY_MANAGER_PROTOCOL].callback)
            {
                (*fixed_channels[L2CAP_FIXED_CHANNEL_TABLE_INDEX_SECURITY_MANAGER_PROTOCOL].callback)(SM_DATA_PACKET, handle, &packet[COMPLETE_L2CAP_HEADER], size-COMPLETE_L2CAP_HEADER);
            }
            break;
        case L2CAP_CID_CONNECTIONLESS_CHANNEL:
            l2cap_puts("L2CAP_CID_SECURITY_MANAGER_PROTOCOL\n");
            if (fixed_channels[L2CAP_FIXED_CHANNEL_TABLE_INDEX_CONNECTIONLESS_CHANNEL].callback) 
            {
                (*fixed_channels[L2CAP_FIXED_CHANNEL_TABLE_INDEX_CONNECTIONLESS_CHANNEL].callback)(UCD_DATA_PACKET, handle, &packet[COMPLETE_L2CAP_HEADER], size-COMPLETE_L2CAP_HEADER);
            }
            break;

        case L2CAP_CID_SIGNALING_LE: {
            l2cap_puts("L2CAP_CID_SIGNALING_LE\n");
            switch (packet[8]){
                case CONNECTION_PARAMETER_UPDATE_RESPONSE: {
                    uint16_t result = READ_BT_16(packet, 12);
                    l2cap_emit_connection_parameter_update_response(handle, result);
                    break;
                }
                case CONNECTION_PARAMETER_UPDATE_REQUEST: {
                    uint8_t event[10];
                    event[0] = L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_REQUEST;
                    event[1] = 8;
                    memcpy(&event[2], &packet[12], 8);
                
                    hci_connection_t * connection = le_hci_connection_for_handle(handle);
                    if (connection){ 
                        if (connection->role != HCI_ROLE_MASTER){
                            // reject command without notifying upper layer when not in master role
                            uint8_t sig_id = packet[COMPLETE_L2CAP_HEADER + 1]; 
                            l2cap_register_signaling_response(handle, COMMAND_REJECT_LE, sig_id, L2CAP_REJ_CMD_UNKNOWN);
                            break;
                        }
                        int update_parameter = 1;
                        le_connection_parameter_range_t existing_range;
                        gap_le_get_connection_parameter_range(&existing_range);
                        uint16_t le_conn_interval_min = READ_BT_16(packet,12);
                        uint16_t le_conn_interval_max = READ_BT_16(packet,14);
                        uint16_t le_conn_latency = READ_BT_16(packet,16);
                        uint16_t le_supervision_timeout = READ_BT_16(packet,18);

                        if (le_conn_interval_min < existing_range.le_conn_interval_min) update_parameter = 0;
                        if (le_conn_interval_max > existing_range.le_conn_interval_max) update_parameter = 0;
                        
                        if (le_conn_latency < existing_range.le_conn_latency_min) update_parameter = 0;
                        if (le_conn_latency > existing_range.le_conn_latency_max) update_parameter = 0;

                        if (le_supervision_timeout < existing_range.le_supervision_timeout_min) update_parameter = 0;
                        if (le_supervision_timeout > existing_range.le_supervision_timeout_max) update_parameter = 0;

                        if (update_parameter){
                            connection->le_con_parameter_update_state = CON_PARAMETER_UPDATE_SEND_RESPONSE;
                            connection->le_conn_interval_min = le_conn_interval_min;
                            connection->le_conn_interval_max = le_conn_interval_max;
                            connection->le_conn_latency = le_conn_latency;
                            connection->le_supervision_timeout = le_supervision_timeout;
                        } else {
                            connection->le_con_parameter_update_state = CON_PARAMETER_UPDATE_DENY;
                        }
                        connection->le_con_param_update_identifier = packet[COMPLETE_L2CAP_HEADER + 1];
                    }
                
                    /* hci_dump_packet( HCI_EVENT_PACKET, 0, event, sizeof(event)); */
                    if (!l2cap_event_packet_handler) break;
                    (*l2cap_event_packet_handler)( HCI_EVENT_PACKET, 0, event, sizeof(event));
                    break;
                }
                default: {
                    uint8_t sig_id = packet[COMPLETE_L2CAP_HEADER + 1]; 
                    l2cap_register_signaling_response(handle, COMMAND_REJECT_LE, sig_id, L2CAP_REJ_CMD_UNKNOWN);
                    break;
                }
            }
            break;
        }
        
        //Vendor command
        case 0xff:
            l2cap_puts("********my_acl_packet********\n");
            l2cap_buf(packet, size);
            break;
            
            
        default: {
            break;
        }
    }
}



// Bluetooth 4.0 - allows to register handler for Attribute Protocol and Security Manager Protocol
void l2cap_register_fixed_channel(btstack_packet_handler_t the_packet_handler, uint16_t channel_id) {
    int index = l2cap_fixed_channel_table_index_for_channel_id(channel_id);
    if (index < 0) return;
    fixed_channels[index].callback = the_packet_handler;
}

