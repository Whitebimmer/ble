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
    return hci_can_send_acl_packet_now(handle);
}
// @deprecated
int le_l2cap_can_send_connectionless_packet_now(void){
    // TODO provide real handle
    return le_l2cap_can_send_fixed_channel_packet_now(0x1234);
}


uint8_t *le_l2cap_get_outgoing_buffer(void){
    return hci_get_outgoing_packet_buffer() + COMPLETE_L2CAP_HEADER; // 8 bytes
}

int le_l2cap_reserve_packet_buffer(void){
    return hci_reserve_packet_buffer();
}

void le_l2cap_release_packet_buffer(void){
    hci_release_packet_buffer();
}

int le_l2cap_send_prepared_connectionless(uint16_t handle, uint16_t cid, uint16_t len){
    
    if (!hci_is_packet_buffer_reserved()){
        log_error("l2cap_send_prepared_connectionless called without reserving packet first");
        return BTSTACK_ACL_BUFFERS_FULL;
    }

    if (!le_hci_can_send_prepared_acl_packet_now(handle)){
        log_info("l2cap_send_prepared_connectionless handle %u,, cid %u, cannot send", handle, cid);
        return BTSTACK_ACL_BUFFERS_FULL;
    }
    
    log_debug("l2cap_send_prepared_connectionless handle %u, cid %u", handle, cid);
    
    uint8_t *acl_buffer = hci_get_outgoing_packet_buffer();

    // 0 - Connection handle : PB=00 : BC=00 
    bt_store_16(acl_buffer, 0, handle | (0 << 12) | (0 << 14));
    // 2 - ACL length
    bt_store_16(acl_buffer, 2,  len + 4);
    // 4 - L2CAP packet length
    bt_store_16(acl_buffer, 4,  len + 0);
    // 6 - L2CAP channel DEST
    bt_store_16(acl_buffer, 6, cid);    
    // send
    int err = hci_send_acl_packet_buffer(len+8);
	/* l2cap_puts("l2cap_send_acl_exit\n"); */
        
    return err;
}

int le_l2cap_send_connectionless(uint16_t handle, uint16_t cid, uint8_t *data, uint16_t len){

    if (!hci_can_send_acl_packet_now(handle)){
        log_info("l2cap_send_connectionless cid %u, cannot send", cid);
        return BTSTACK_ACL_BUFFERS_FULL;
    }

    hci_reserve_packet_buffer();
    uint8_t *acl_buffer = hci_get_outgoing_packet_buffer();

    memcpy(&acl_buffer[8], data, len);

    return le_l2cap_send_prepared_connectionless(handle, cid, len);
}

static void l2cap_hci_event_handler(uint8_t packet_type, uint16_t cid, uint8_t *packet, uint16_t size)
{
    l2cap_puts("Layer - l2cap_hci_event_handler : ");
            
    /*-TODO-*/

}

static int l2cap_send_signaling_packet(hci_con_handle_t handle, L2CAP_SIGNALING_COMMANDS cmd, uint8_t identifier, ...){

    if (!hci_can_send_acl_packet_now(handle)){
        log_info("l2cap_send_signaling_packet, cannot send");
        return BTSTACK_ACL_BUFFERS_FULL;
    }
    
    // log_info("l2cap_send_signaling_packet type %u", cmd);
    hci_reserve_packet_buffer();
    uint8_t *acl_buffer = hci_get_outgoing_packet_buffer();
    va_list argptr;
    va_start(argptr, identifier);
    uint16_t len = l2cap_create_signaling_classic(acl_buffer, handle, cmd, identifier, argptr);
    va_end(argptr);
    // log_info("l2cap_send_signaling_packet con %u!", handle);
    return hci_send_acl_packet_buffer(len);
}

#ifdef ENABLE_BLE
static int l2cap_send_le_signaling_packet(hci_con_handle_t handle, L2CAP_SIGNALING_COMMANDS cmd, uint8_t identifier, ...){

    if (!hci_can_send_acl_packet_now(handle)){
        log_info("l2cap_send_signaling_packet, cannot send");
        return BTSTACK_ACL_BUFFERS_FULL;
    }
    
    // log_info("l2cap_send_signaling_packet type %u", cmd);
    hci_reserve_packet_buffer();
    uint8_t *acl_buffer = hci_get_outgoing_packet_buffer();
    va_list argptr;
    va_start(argptr, identifier);
    uint16_t len = l2cap_create_signaling_le(acl_buffer, handle, cmd, identifier, argptr);
    va_end(argptr);
    // log_info("l2cap_send_signaling_packet con %u!", handle);
    return hci_send_acl_packet_buffer(len);
}
#endif

// MARK: L2CAP_RUN
// process outstanding signaling tasks
static void l2cap_run(void){
    
    // log_info("l2cap_run: entered");

#if 0
    // check pending signaling responses
    while (signaling_responses_pending){
        
        hci_con_handle_t handle = signaling_responses[0].handle;
        
        if (!hci_can_send_acl_packet_now(handle)) break;

        uint8_t  sig_id = signaling_responses[0].sig_id;
        uint16_t infoType = signaling_responses[0].data;    // INFORMATION_REQUEST
        uint16_t result   = signaling_responses[0].data;    // CONNECTION_REQUEST, COMMAND_REJECT
        uint8_t  response_code = signaling_responses[0].code;

        // remove first item before sending (to avoid sending response mutliple times)
        signaling_responses_pending--;
        int i;
        for (i=0; i < signaling_responses_pending; i++){
            memcpy(&signaling_responses[i], &signaling_responses[i+1], sizeof(l2cap_signaling_response_t));
        }

        switch (response_code){
            case CONNECTION_REQUEST:
                l2cap_send_signaling_packet(handle, CONNECTION_RESPONSE, sig_id, 0, 0, result, 0);
                // also disconnect if result is 0x0003 - security blocked
                if (result == 0x0003){
                    hci_disconnect_security_block(handle);
                }
                break;
            case ECHO_REQUEST:
                l2cap_send_signaling_packet(handle, ECHO_RESPONSE, sig_id, 0, NULL);
                break;
            case INFORMATION_REQUEST:
                switch (infoType){
                    case 1: { // Connectionless MTU
                        uint16_t connectionless_mtu = hci_max_acl_data_packet_length();
                        l2cap_send_signaling_packet(handle, INFORMATION_RESPONSE, sig_id, infoType, 0, sizeof(connectionless_mtu), &connectionless_mtu);
                        break;
                    }
                    case 2: { // Extended Features Supported
                        // extended features request supported, features: fixed channels, unicast connectionless data reception
                        uint32_t features = 0x280;
                        l2cap_send_signaling_packet(handle, INFORMATION_RESPONSE, sig_id, infoType, 0, sizeof(features), &features);
                        break;
                    }
                    case 3: { // Fixed Channels Supported
                        uint8_t map[8];
                        memset(map, 0, 8);
                        map[0] = 0x06;  // L2CAP Signaling Channel (0x02) + Connectionless reception (0x04)
                        l2cap_send_signaling_packet(handle, INFORMATION_RESPONSE, sig_id, infoType, 0, sizeof(map), &map);
                        break;
                    }
                    default:
                        // all other types are not supported
                        l2cap_send_signaling_packet(handle, INFORMATION_RESPONSE, sig_id, infoType, 1, 0, NULL);
                        break;                        
                }
                break;
            case COMMAND_REJECT:
                l2cap_send_signaling_packet(handle, COMMAND_REJECT, sig_id, result, 0, NULL);
#ifdef ENABLE_BLE
            case COMMAND_REJECT_LE:
                l2cap_send_le_signaling_packet(handle, COMMAND_REJECT, sig_id, result, 0, NULL);
                break;
#endif
            default:
                // should not happen
                break;
        }
    }
    
    uint8_t  config_options[4];
    linked_list_iterator_t it;    
    linked_list_iterator_init(&it, &l2cap_channels);
    while (linked_list_iterator_has_next(&it)){

        l2cap_channel_t * channel = (l2cap_channel_t *) linked_list_iterator_next(&it);
        // log_info("l2cap_run: channel %p, state %u, var 0x%02x", channel, channel->state, channel->state_var);
        switch (channel->state){

            case L2CAP_STATE_WAIT_INCOMING_SECURITY_LEVEL_UPDATE:
            case L2CAP_STATE_WAIT_CLIENT_ACCEPT_OR_REJECT:
                if (!hci_can_send_acl_packet_now(channel->con_handle)) break;
                if (channel->state_var & L2CAP_CHANNEL_STATE_VAR_SEND_CONN_RESP_PEND) {
                    channelStateVarClearFlag(channel, L2CAP_CHANNEL_STATE_VAR_SEND_CONN_RESP_PEND);
                    l2cap_send_signaling_packet(channel->con_handle, CONNECTION_RESPONSE, channel->remote_sig_id, channel->local_cid, channel->remote_cid, 1, 0);
                }
                break;

            case L2CAP_STATE_WILL_SEND_CREATE_CONNECTION:
                if (!hci_can_send_command_packet_now()) break;
                // send connection request - set state first
                channel->state = L2CAP_STATE_WAIT_CONNECTION_COMPLETE;
                // BD_ADDR, Packet_Type, Page_Scan_Repetition_Mode, Reserved, Clock_Offset, Allow_Role_Switch
                hci_send_cmd(&hci_create_connection, channel->address, hci_usable_acl_packet_types(), 0, 0, 0, 1); 
                break;
                
            case L2CAP_STATE_WILL_SEND_CONNECTION_RESPONSE_DECLINE:
                if (!hci_can_send_acl_packet_now(channel->con_handle)) break;
                channel->state = L2CAP_STATE_INVALID;
                l2cap_send_signaling_packet(channel->con_handle, CONNECTION_RESPONSE, channel->remote_sig_id, channel->local_cid, channel->remote_cid, channel->reason, 0);
                // discard channel - l2cap_finialize_channel_close without sending l2cap close event
                l2cap_stop_rtx(channel);
                linked_list_iterator_remove(&it);
                btstack_memory_l2cap_channel_free(channel); 
                break;
                
            case L2CAP_STATE_WILL_SEND_CONNECTION_RESPONSE_ACCEPT:
                if (!hci_can_send_acl_packet_now(channel->con_handle)) break;
                channel->state = L2CAP_STATE_CONFIG;
                channelStateVarSetFlag(channel, L2CAP_CHANNEL_STATE_VAR_SEND_CONF_REQ);
                l2cap_send_signaling_packet(channel->con_handle, CONNECTION_RESPONSE, channel->remote_sig_id, channel->local_cid, channel->remote_cid, 0, 0);
                break;
                
            case L2CAP_STATE_WILL_SEND_CONNECTION_REQUEST:
                if (!hci_can_send_acl_packet_now(channel->con_handle)) break;
                // success, start l2cap handshake
                channel->local_sig_id = l2cap_next_sig_id();
                channel->state = L2CAP_STATE_WAIT_CONNECT_RSP;
                l2cap_send_signaling_packet( channel->con_handle, CONNECTION_REQUEST, channel->local_sig_id, channel->psm, channel->local_cid);
                l2cap_start_rtx(channel);
                break;
            
            case L2CAP_STATE_CONFIG:
                if (!hci_can_send_acl_packet_now(channel->con_handle)) break;
                if (channel->state_var & L2CAP_CHANNEL_STATE_VAR_SEND_CONF_RSP){
                    uint16_t flags = 0;
                    channelStateVarClearFlag(channel, L2CAP_CHANNEL_STATE_VAR_SEND_CONF_RSP);
                    if (channel->state_var & L2CAP_CHANNEL_STATE_VAR_SEND_CONF_RSP_CONT) {
                        flags = 1;
                    } else {
                        channelStateVarSetFlag(channel, L2CAP_CHANNEL_STATE_VAR_SENT_CONF_RSP);
                    }
                    if (channel->state_var & L2CAP_CHANNEL_STATE_VAR_SEND_CONF_RSP_INVALID){
                        l2cap_send_signaling_packet(channel->con_handle, CONFIGURE_RESPONSE, channel->remote_sig_id, channel->remote_cid, flags, L2CAP_CONF_RESULT_UNKNOWN_OPTIONS, 0, NULL);
                    } else if (channel->state_var & L2CAP_CHANNEL_STATE_VAR_SEND_CONF_RSP_MTU){
                        config_options[0] = 1; // MTU
                        config_options[1] = 2; // len param
                        little_endian_store_16( (uint8_t*)&config_options, 2, channel->remote_mtu);
                        l2cap_send_signaling_packet(channel->con_handle, CONFIGURE_RESPONSE, channel->remote_sig_id, channel->remote_cid, flags, 0, 4, &config_options);
                        channelStateVarClearFlag(channel,L2CAP_CHANNEL_STATE_VAR_SEND_CONF_RSP_MTU);
                    } else {
                        l2cap_send_signaling_packet(channel->con_handle, CONFIGURE_RESPONSE, channel->remote_sig_id, channel->remote_cid, flags, 0, 0, NULL);
                    }
                    channelStateVarClearFlag(channel, L2CAP_CHANNEL_STATE_VAR_SEND_CONF_RSP_CONT);
                }
                else if (channel->state_var & L2CAP_CHANNEL_STATE_VAR_SEND_CONF_REQ){
                    channelStateVarClearFlag(channel, L2CAP_CHANNEL_STATE_VAR_SEND_CONF_REQ);
                    channelStateVarSetFlag(channel, L2CAP_CHANNEL_STATE_VAR_SENT_CONF_REQ);
                    channel->local_sig_id = l2cap_next_sig_id();
                    config_options[0] = 1; // MTU
                    config_options[1] = 2; // len param
                    little_endian_store_16( (uint8_t*)&config_options, 2, channel->local_mtu);
                    l2cap_send_signaling_packet(channel->con_handle, CONFIGURE_REQUEST, channel->local_sig_id, channel->remote_cid, 0, 4, &config_options);
                    l2cap_start_rtx(channel);
                }
                if (l2cap_channel_ready_for_open(channel)){
                    channel->state = L2CAP_STATE_OPEN;
                    l2cap_emit_channel_opened(channel, 0);  // success
                }
                break;

            case L2CAP_STATE_WILL_SEND_DISCONNECT_RESPONSE:
                if (!hci_can_send_acl_packet_now(channel->con_handle)) break;
                channel->state = L2CAP_STATE_INVALID;
                l2cap_send_signaling_packet( channel->con_handle, DISCONNECTION_RESPONSE, channel->remote_sig_id, channel->local_cid, channel->remote_cid);   
                // we don't start an RTX timer for a disconnect - there's no point in closing the channel if the other side doesn't respond :)
                l2cap_finialize_channel_close(channel);  // -- remove from list
                break;
                
            case L2CAP_STATE_WILL_SEND_DISCONNECT_REQUEST:
                if (!hci_can_send_acl_packet_now(channel->con_handle)) break;
                channel->local_sig_id = l2cap_next_sig_id();
                channel->state = L2CAP_STATE_WAIT_DISCONNECT;
                l2cap_send_signaling_packet( channel->con_handle, DISCONNECTION_REQUEST, channel->local_sig_id, channel->remote_cid, channel->local_cid);   
                break;
            default:
                break;
        }
    }
#endif

#ifdef ENABLE_BLE
    linked_list_iterator_t it;    
    // send l2cap con paramter update if necessary
    hci_connections_get_iterator(&it);
    while(linked_list_iterator_has_next(&it)){
        hci_connection_t * connection = (hci_connection_t *) linked_list_iterator_next(&it);
        if (connection->address_type != BD_ADDR_TYPE_LE_PUBLIC && connection->address_type != BD_ADDR_TYPE_LE_RANDOM) continue;
        if (!hci_can_send_acl_packet_now(connection->con_handle)) continue;
        switch (connection->le_con_parameter_update_state){
            case CON_PARAMETER_UPDATE_SEND_REQUEST:
                connection->le_con_parameter_update_state = CON_PARAMETER_UPDATE_NONE;
                l2cap_send_le_signaling_packet(connection->con_handle, CONNECTION_PARAMETER_UPDATE_REQUEST, connection->le_con_param_update_identifier,
                                               connection->le_conn_interval_min, connection->le_conn_interval_max, connection->le_conn_latency, connection->le_supervision_timeout);
                break;
            case CON_PARAMETER_UPDATE_SEND_RESPONSE:
                connection->le_con_parameter_update_state = CON_PARAMETER_UPDATE_CHANGE_HCI_CON_PARAMETERS;
                l2cap_send_le_signaling_packet(connection->con_handle, CONNECTION_PARAMETER_UPDATE_RESPONSE, connection->le_con_param_update_identifier, 0);
                break;
            case CON_PARAMETER_UPDATE_DENY:
                connection->le_con_parameter_update_state = CON_PARAMETER_UPDATE_NONE;
                l2cap_send_le_signaling_packet(connection->con_handle, CONNECTION_PARAMETER_UPDATE_RESPONSE, connection->le_con_param_update_identifier, 1);
                break;
            default:
                break;
        }
    }
#endif

    // log_info("l2cap_run: exit");
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

/**
 * @brief Request an update of the connection parameter for a given LE connection
 * @param handle
 * @param conn_interval_min (unit: 1.25ms)
 * @param conn_interval_max (unit: 1.25ms)
 * @param conn_latency
 * @param supervision_timeout (unit: 10ms)
 * @returns 0 if ok
 */
int gap_request_connection_parameter_update(hci_con_handle_t con_handle, uint16_t conn_interval_min,
    uint16_t conn_interval_max, uint16_t conn_latency, uint16_t supervision_timeout){
    static u8 le_con_param_update_identifier = 0x1;


    hci_connection_t * connection = le_hci_connection_for_handle(con_handle);
    if (!connection) return ERROR_CODE_UNKNOWN_CONNECTION_IDENTIFIER;
    connection->le_conn_interval_min = conn_interval_min;
    connection->le_conn_interval_max = conn_interval_max;
    connection->le_conn_latency = conn_latency;
    connection->le_supervision_timeout = supervision_timeout;
    connection->le_con_parameter_update_state = CON_PARAMETER_UPDATE_SEND_REQUEST;
    connection->le_con_param_update_identifier = le_con_param_update_identifier;
    l2cap_run();

    if (le_con_param_update_identifier++ == 0)
        le_con_param_update_identifier = 0x1;
    return 0;
}

static void l2cap_dispatch_cid(uint16_t cid, uint8_t *packet, uint16_t size)
{
    // Get Channel ID
    uint16_t channel_id = READ_L2CAP_CHANNEL_ID(packet); 
    hci_con_handle_t handle = READ_ACL_CONNECTION_HANDLE(packet);

    switch(cid)
    {
        case L2CAP_CID_SIGNALING:
            l2cap_puts("L2CAP_CID_SIGNALING\n");
            /*-TODO-*/
            break;
        case L2CAP_CID_CONNECTIONLESS_CHANNEL:
            l2cap_puts("L2CAP_CID_CONNECTIONLESS_CHANNEL\n");
            if (fixed_channels[L2CAP_FIXED_CHANNEL_TABLE_INDEX_CONNECTIONLESS_CHANNEL].callback) 
            {
                (*fixed_channels[L2CAP_FIXED_CHANNEL_TABLE_INDEX_CONNECTIONLESS_CHANNEL].callback)(UCD_DATA_PACKET, handle, &packet[COMPLETE_L2CAP_HEADER], size-COMPLETE_L2CAP_HEADER);
            }
            break;
        default:
            if (((cid >= 4) && (cid <= 6))
                && ((cid >= 8) && (cid <= 0x3e)))
            {
                puts("L2CAP ACL-U link Reserved CID\n");
            }    
            break;
    }
}

static void le_l2cap_dispatch_cid(uint16_t cid, uint8_t *packet, uint16_t size)
{
    // Get Channel ID
    uint16_t channel_id = READ_L2CAP_CHANNEL_ID(packet); 
    hci_con_handle_t handle = READ_ACL_CONNECTION_HANDLE(packet);

    switch(cid)
    {
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
        case L2CAP_CID_SIGNALING_LE: {
            l2cap_puts("L2CAP_CID_SIGNALING_LE : ");
            switch (packet[8]){
                case CONNECTION_PARAMETER_UPDATE_RESPONSE: {
                    l2cap_puts(" - CONNECTION_PARAMETER_UPDATE_RESPONSE");
                    uint16_t result = READ_BT_16(packet, 12);
                    l2cap_emit_connection_parameter_update_response(handle, result);
                    break;
                }
                case CONNECTION_PARAMETER_UPDATE_REQUEST: {
                    l2cap_puts(" - CONNECTION_PARAMETER_UPDATE_REQUEST ");
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
                            l2cap_puts(" - CON_PARAMETER_UPDATE_SEND_RESPONSE\n");
                            l2cap_printf("Conn_Interval_Min : %x\n", connection->le_conn_interval_min);
                            l2cap_printf("Conn_Interval_Max : %x\n", connection->le_conn_interval_max);
                            l2cap_printf("Conn_Latency : %x\n", connection->le_conn_latency);
                            l2cap_printf("Supervision_Timeout : %x\n", connection->le_supervision_timeout);
                        } else {
                            l2cap_puts(" - CON_PARAMETER_UPDATE_DENY\n");
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
            
        default: 
            if (((cid >= 1) && (cid <= 3))
                && ((cid >= 7) && (cid <= 0x1f))
                && (cid == 0x3f)
                && ((cid >= 0x80) && (cid <= 0xfffff)))
            {
                puts("L2CAP LE-U link Reserved CID\n");
            }    
            break;
    }
}

static void l2cap_acl_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size ){
        
    // Get Channel ID
    uint16_t channel_id = READ_L2CAP_CHANNEL_ID(packet); 
    hci_con_handle_t handle = READ_ACL_CONNECTION_HANDLE(packet);

    l2cap_puts("Layer - l2cap_acl_handler: ");
    
    //ACL-U logical link for Classic
    l2cap_dispatch_cid(channel_id, packet, size);

#ifdef ENABLE_BLE
    //LE-U logical link for LE
    le_l2cap_dispatch_cid(channel_id, packet, size);
#endif
    //0x0040~0xffff Dynamically allocated
    switch (channel_id) {
        case 0:
            l2cap_puts("L2CAP Not Allowed CID\n");
            break;
        //Vendor command
        case 0xff:
            l2cap_puts("********my_acl_packet********\n");
            l2cap_buf(packet, size);
            break;
    }
}



// Bluetooth 4.0 - allows to register handler for Attribute Protocol and Security Manager Protocol
void l2cap_register_fixed_channel(btstack_packet_handler_t the_packet_handler, uint16_t channel_id) {
    int index = l2cap_fixed_channel_table_index_for_channel_id(channel_id);
    if (index < 0) return;
    fixed_channels[index].callback = the_packet_handler;
}

