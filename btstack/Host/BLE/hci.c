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
 *  hci.c
 *
 *  Created by Matthias Ringwald on 4/29/09.
 *
 */

#include "ble/btstack-config.h"

#include "ble/hci.h"
#include "ble/gap.h"

#include <stdarg.h>
#include <string.h>
#include <stdio.h>


#include "ble/btstack_memory.h"
#include "ble/debug.h"
#include "ble/hci_dump.h"

#include <linked_list.h>
#include <hci_cmds.h>
#include "bt_memory.h"

/************************HCI DEBUG CONTROL**************************/
#define HCI_DEBUG
#ifdef HCI_DEBUG
#define hci_puts     puts
#define hci_deg      printf

#else
#define hci_puts(...)     
#define hci_deg(...)      

#endif

#define HCI_CONNECTION_TIMEOUT_MS 10000

#ifdef USE_BLUETOOL
#include "../platforms/ios/src/bt_control_iphone.h"
#endif

/*static void hci_update_scan_enable(void);*/
static gap_security_level_t gap_security_level_for_connection(hci_connection_t * connection);
static void hci_connection_timeout_handler(timer_source_t *timer);
static void hci_connection_timestamp(hci_connection_t *connection);
static int  hci_power_control_on(void);
static void hci_power_control_off(void);
static void hci_state_reset(void);
#ifdef HAVE_BLE
// called from test/ble_client/advertising_data_parser.c
void le_handle_advertisement_report(uint8_t *packet, int size);
static void hci_remove_from_whitelist(bd_addr_type_t address_type, bd_addr_t address);
#endif

static void hci_emit_state();

// the STACK is here
#ifndef HAVE_MALLOC
static hci_stack_t   hci_stack_static;
#endif
static hci_stack_t * hci_stack sec(.btmem_highly_available) = NULL;

// test helper
static uint8_t disable_l2cap_timeouts = 0;

/**
 * create connection for given address
 *
 * @return connection OR NULL, if no memory left
 */
static hci_connection_t * create_connection_for_bd_addr_and_type(bd_addr_t addr, bd_addr_type_t addr_type)
{
    log_info("create_connection_for_addr %s, type %x", 
			bd_addr_to_str(addr), addr_type);

    hci_connection_t * conn = le_btstack_memory_hci_connection_get();
    if (!conn) {
		return NULL;
	}
    memset(conn, 0, sizeof(hci_connection_t));
    BD_ADDR_COPY(conn->address, addr);
    conn->address_type = addr_type;
    conn->con_handle = 0xffff;
    conn->authentication_flags = AUTH_FLAGS_NONE;
    conn->bonding_flags = 0;
    conn->requested_security_level = LEVEL_0;
//	linked_item_set_user(&conn->timeout.item, conn);
//	conn->timeout.process = hci_connection_timeout_handler;
	hci_connection_timestamp(conn);
    conn->acl_recombination_length = 0;
    conn->acl_recombination_pos = 0;
    conn->num_acl_packets_sent = 0;
    /*conn->num_sco_packets_sent = 0;*/
    conn->le_con_parameter_update_state = CON_PARAMETER_UPDATE_NONE;
    linked_list_add(&hci_stack->connections, (linked_item_t *) conn);
    return conn;
}


/**
 * get le connection parameter range
*
 * @return le connection parameter range struct
 */
void gap_le_get_connection_parameter_range(le_connection_parameter_range_t * range){
    *range = hci_stack->le_connection_parameter_range;
}

/**
 * set le connection parameter range
 *
 */

void gap_le_set_connection_parameter_range(le_connection_parameter_range_t * range){
    hci_stack->le_connection_parameter_range = *range;
}

/**
 * get hci connections iterator
 *
 * @return hci connections iterator
 */

void le_hci_connections_get_iterator(linked_list_iterator_t *it){
    linked_list_iterator_init(it, &hci_stack->connections);
}

/**
 * get connection for a given handle
 *
 * @return connection OR NULL, if not found
 */
hci_connection_t * le_hci_connection_for_handle(hci_con_handle_t con_handle)
{
    linked_list_iterator_t it;
    linked_list_iterator_init(&it, &hci_stack->connections);
    while (linked_list_iterator_has_next(&it)){
        hci_connection_t * item = (hci_connection_t *) linked_list_iterator_next(&it);
        if ( item->con_handle == con_handle ) {
            return item;
        }
    } 
    return NULL;
}

/**
 * get connection for given address
 *
 * @return connection OR NULL, if not found
 */
hci_connection_t * le_hci_connection_for_bd_addr_and_type(bd_addr_t  addr, bd_addr_type_t addr_type)
{
    linked_list_iterator_t it;
    linked_list_iterator_init(&it, &hci_stack->connections);
    while (linked_list_iterator_has_next(&it)){
        hci_connection_t * connection = (hci_connection_t *) linked_list_iterator_next(&it);
        if (connection->address_type != addr_type)  continue;
        if (memcmp(addr, connection->address, 6) != 0) continue;
        return connection;   
    } 
    return NULL;
}

static void hci_connection_timeout_handler(timer_source_t *timer)
{
	/*-TODO-*/

#if 0
    hci_connection_t * connection = (hci_connection_t *) linked_item_get_user(&timer->item);

    if (embedded_get_ticks() > connection->timestamp + embedded_ticks_for_ms(HCI_CONNECTION_TIMEOUT_MS)){
        // connections might be timed out
        hci_emit_l2cap_check_timeout(connection);
    }
    run_loop_set_timer(timer, HCI_CONNECTION_TIMEOUT_MS);
    run_loop_add_timer(timer);
#endif
}

static void hci_connection_timestamp(hci_connection_t *connection)
{
	/*-TODO-*/
    /*connection->timestamp = embedded_get_ticks();*/
}


inline static void connectionSetAuthenticationFlags(hci_connection_t * conn, hci_authentication_flags_t flags)
{
    conn->authentication_flags = (hci_authentication_flags_t)(conn->authentication_flags | flags);
}

inline static void connectionClearAuthenticationFlags(hci_connection_t * conn, hci_authentication_flags_t flags)
{
    conn->authentication_flags = (hci_authentication_flags_t)(conn->authentication_flags & ~flags);
}


/**
 * add authentication flags and reset timer
 * @note: assumes classic connection
 * @note: bd_addr is passed in as litle endian uint8_t * as it is called from parsing packets
 */
static void hci_add_connection_flags_for_flipped_bd_addr(uint8_t *bd_addr, hci_authentication_flags_t flags){
    bd_addr_t addr;
    bt_flip_addr(addr, bd_addr);
    hci_connection_t * conn = le_hci_connection_for_bd_addr_and_type(addr, BD_ADDR_TYPE_CLASSIC);
    if (conn) {
        connectionSetAuthenticationFlags(conn, flags);
        hci_connection_timestamp(conn);
    }
}

/*int  hci_authentication_active_for_handle(hci_con_handle_t handle){*/
    /*hci_connection_t * conn = hci_connection_for_handle(handle);*/
    /*if (!conn) return 0;*/
    /*if (conn->authentication_flags & LEGACY_PAIRING_ACTIVE) return 1;*/
    /*if (conn->authentication_flags & SSP_PAIRING_ACTIVE) return 1;*/
    /*return 0;*/
/*}*/

static void hci_drop_link_key_for_bd_addr(bd_addr_t addr)
{
    if (hci_stack->remote_device_db) {
        hci_stack->remote_device_db->delete_link_key(addr);
    }
}

/*int hci_is_le_connection(hci_connection_t * connection){
    return  connection->address_type == BD_ADDR_TYPE_LE_PUBLIC ||
    connection->address_type == BD_ADDR_TYPE_LE_RANDOM;
}*/


/**
 * count connections
 */
static int nr_hci_connections(void){
    int count = 0;
    linked_item_t *it;
    for (it = (linked_item_t *) hci_stack->connections; it ; it = it->next, count++);
    return count;
}

/** 
 * Dummy handler called by HCI
 */
static void dummy_handler(uint8_t packet_type, 
		uint8_t *packet, uint16_t size)
{

}

/*uint8_t hci_number_outgoing_packets(hci_con_handle_t handle){*/
    /*hci_connection_t * connection = hci_connection_for_handle(handle);*/
    /*if (!connection) {*/
        /*log_error("hci_number_outgoing_packets: connection for handle %u does not exist!", handle);*/
        /*return 0;*/
    /*}*/
    /*return connection->num_acl_packets_sent;*/
/*}*/

uint8_t hci_number_free_acl_slots_for_handle(hci_con_handle_t con_handle){
    
    int num_packets_sent_classic = 0;
    int num_packets_sent_le = 0;

    bd_addr_type_t address_type = BD_ADDR_TYPE_UNKNOWN;

    linked_item_t *it;
    for (it = (linked_item_t *) hci_stack->connections; it ; it = it->next){
        hci_connection_t * connection = (hci_connection_t *) it;
		num_packets_sent_le += connection->num_acl_packets_sent;
        // ignore connections that are not open, e.g., in state RECEIVED_DISCONNECTION_COMPLETE
        if (connection->con_handle == con_handle && connection->state == OPEN){
            address_type = connection->address_type;
        }
    }

    int free_slots_classic = hci_stack->acl_packets_total_num - num_packets_sent_classic;
    int free_slots_le = 0;

    if (free_slots_classic < 0){
        hci_deg("hci_number_free_acl_slots: outgoing classic packets (%u) > total classic packets (%u)", num_packets_sent_classic, hci_stack->acl_packets_total_num);
        return 0;
    }

    if (hci_stack->le_acl_packets_total_num){
        // if we have LE slots, they are used
        free_slots_le = hci_stack->le_acl_packets_total_num - num_packets_sent_le;
        if (free_slots_le < 0){
            hci_deg("hci_number_free_acl_slots: outgoing le packets (%u) > total le packets (%u)", num_packets_sent_le, hci_stack->le_acl_packets_total_num);
            return 0;
        }
    } else {
        // otherwise, classic slots are used for LE, too
        free_slots_classic -= num_packets_sent_le;
        if (free_slots_classic < 0){
            hci_deg("hci_number_free_acl_slots: outgoing classic + le packets (%u + %u) > total packets (%u)", num_packets_sent_classic, num_packets_sent_le, hci_stack->acl_packets_total_num);
            return 0;
        }
    }

    switch (address_type)
	{
        case BD_ADDR_TYPE_UNKNOWN:
            log_error("hci_number_free_acl_slots: handle 0x%04x not in connection list", con_handle);
            return 0;

        default:
           if (hci_stack->le_acl_packets_total_num){
               hci_deg("le free slots %x\n", free_slots_le);
               return free_slots_le;
           }
           return free_slots_classic; 
    }
}

// new functions replacing hci_can_send_packet_now[_using_packet_buffer]
int le_hci_can_send_command_packet_now(void){
	//TODO
	/* return 1; */
    if (hci_stack->hci_packet_buffer_reserved) {
        puts("le_hci_can_send_command_packet_now\n");
        return 0;   
    }

    // check for async hci transport implementations
    if (hci_stack->hci_transport->can_send_packet_now){
        if (!hci_stack->hci_transport->can_send_packet_now(HCI_COMMAND_DATA_PACKET)){
            return 0;
        }
    }

    return hci_stack->num_cmd_packets > 0;
}

int le_hci_can_send_prepared_acl_packet_now(hci_con_handle_t con_handle) {
	//TODO
	/* return 1; */
    // check for async hci transport implementations
    if (hci_stack->hci_transport->can_send_packet_now){
        if (!hci_stack->hci_transport->can_send_packet_now(HCI_ACL_DATA_PACKET)){
            return 0;
        }
    }
    return hci_number_free_acl_slots_for_handle(con_handle) > 0;
}

int le_hci_can_send_acl_packet_now(hci_con_handle_t con_handle){
	//TODO
	/* return 1; */
    if (hci_stack->hci_packet_buffer_reserved) {
        puts("le_hci_can_send_acl_packet_now\n");
        return 0;   
    }
    return le_hci_can_send_prepared_acl_packet_now(con_handle);
}

// used for internal checks in l2cap[-le].c
int le_hci_is_packet_buffer_reserved(void){
    return hci_stack->hci_packet_buffer_reserved;
}

// reserves outgoing packet buffer. @returns 1 if successful
int le_hci_reserve_packet_buffer(void){
    if (hci_stack->hci_packet_buffer_reserved) {
        log_error("hci_reserve_packet_buffer called but buffer already reserved");
        return 0;
    }
    hci_stack->hci_packet_buffer_reserved = 1;
    return 1;    
}

void le_hci_release_packet_buffer(void){
    hci_stack->hci_packet_buffer_reserved = 0;
}

// assumption: synchronous implementations don't provide can_send_packet_now as they don't keep the buffer after the call
int hci_transport_synchronous(void){
    return hci_stack->hci_transport->can_send_packet_now == NULL;
}

uint16_t hci_max_acl_le_data_packet_length(void){
    return hci_stack->le_data_packets_length > 0 ? hci_stack->le_data_packets_length : hci_stack->acl_data_packet_length;
}

static int hci_send_acl_packet_fragments(hci_connection_t *connection)
{
    // log_info("hci_send_acl_packet_fragments  %u/%u (con 0x%04x)", hci_stack->acl_fragmentation_pos, hci_stack->acl_fragmentation_total_size, connection->con_handle);

    // max ACL data packet length depends on connection type (LE vs. Classic) and available buffers
    uint16_t max_acl_data_packet_length = hci_stack->acl_data_packet_length;
    if (/*hci_is_le_connection(connection) && */hci_stack->le_data_packets_length > 0){
        max_acl_data_packet_length = hci_stack->le_data_packets_length;
    }
	/* puts("hci_send_acl\n"); */

    // testing: reduce buffer to minimum
    // max_acl_data_packet_length = 52;

    int err;
    // multiple packets could be send on a synchronous HCI transport
    while (1){

        // get current data
        const uint16_t acl_header_pos = hci_stack->acl_fragmentation_pos - 4;
        int current_acl_data_packet_length = hci_stack->acl_fragmentation_total_size - hci_stack->acl_fragmentation_pos;
        int more_fragments = 0;

        // if ACL packet is larger than Bluetooth packet buffer, only send max_acl_data_packet_length
        if (current_acl_data_packet_length > max_acl_data_packet_length){
            more_fragments = 1;
            current_acl_data_packet_length = max_acl_data_packet_length;
        }

        // copy handle_and_flags if not first fragment and update packet boundary flags to be 01 (continuing fragmnent)
        if (acl_header_pos > 0){
            uint16_t handle_and_flags = READ_BT_16(hci_stack->hci_packet_buffer, 0);
            handle_and_flags = (handle_and_flags & 0xcfff) | (1 << 12);
            bt_store_16(hci_stack->hci_packet_buffer, acl_header_pos, handle_and_flags);
        }

        // update header len
        bt_store_16(hci_stack->hci_packet_buffer, acl_header_pos + 2, current_acl_data_packet_length);

        // count packet
        connection->num_acl_packets_sent++;

        // send packet
        uint8_t * packet = &hci_stack->hci_packet_buffer[acl_header_pos];
        const int size = current_acl_data_packet_length + 4;
        err = hci_stack->hci_transport->send_packet(HCI_ACL_DATA_PACKET, packet, size);

        puts("acl more ");
        // done yet?
        if (!more_fragments) break;

        // update start of next fragment to send
        hci_stack->acl_fragmentation_pos += current_acl_data_packet_length;

        // can send more?
        if (!le_hci_can_send_prepared_acl_packet_now(connection->con_handle))
		   	return err;
    }

    puts("done!\n");
    // done    
    hci_stack->acl_fragmentation_pos = 0;
    hci_stack->acl_fragmentation_total_size = 0;

    // release buffer now for synchronous transport
    if (hci_transport_synchronous()){
        le_hci_release_packet_buffer();
        // notify upper stack that iit might be possible to send again
        uint8_t event[] = { DAEMON_EVENT_HCI_PACKET_SENT, 0};
        hci_stack->packet_handler(HCI_EVENT_PACKET, 
				&event[0], sizeof(event));
    }
	/* puts("exit2\n"); */

    return err;
}

// pre: caller has reserved the packet buffer
int le_hci_send_acl_packet_buffer(int size){
	/* puts("le_send_acl\n"); */

    // log_info("hci_send_acl_packet_buffer size %u", size);

    if (!hci_stack->hci_packet_buffer_reserved) {
        return 0;
    }

    uint8_t * packet = hci_stack->hci_packet_buffer;
    hci_con_handle_t con_handle = READ_ACL_CONNECTION_HANDLE(packet);

    // check for free places on Bluetooth module
    if (!le_hci_can_send_prepared_acl_packet_now(con_handle)) {
        le_hci_release_packet_buffer();
        return BTSTACK_ACL_BUFFERS_FULL;
    }

    hci_connection_t *connection = le_hci_connection_for_handle( con_handle);
    if (!connection) {
        le_hci_release_packet_buffer();
        return 0;
    }
    hci_connection_timestamp(connection);
    

    // setup data
    hci_stack->acl_fragmentation_total_size = size;
    hci_stack->acl_fragmentation_pos = 4;   // start of L2CAP packet

    return hci_send_acl_packet_fragments(connection);
}


static void acl_handler(uint8_t *packet, int size)
{
	hci_con_handle_t con_handle = READ_ACL_CONNECTION_HANDLE(packet);
	hci_connection_t *conn      = le_hci_connection_for_handle(con_handle);
	uint8_t  acl_flags          = READ_ACL_FLAGS(packet);
	uint16_t acl_length         = READ_ACL_LENGTH(packet);

	// ignore non-registered handle
	ASSERT(conn != NULL, "%s - %x\n", __func__, con_handle);

	if (!conn){
		return;
	}

	// assert packet is complete    
	if (acl_length + 4 != size){
		return;
	}

	// update idle timestamp
	hci_connection_timestamp(conn);

	// handle different packet types
	switch (acl_flags & 0x03) 
	{
		case 0x01: // continuation fragment

			// sanity checks
			if (conn->acl_recombination_pos == 0) {
				return;
			}
			if (conn->acl_recombination_pos + acl_length > 4 + HCI_ACL_BUFFER_SIZE){
				conn->acl_recombination_pos = 0;
				return;
			}

			// append fragment payload (header already stored)
			memcpy(&conn->acl_recombination_buffer[HCI_INCOMING_PRE_BUFFER_SIZE + conn->acl_recombination_pos], &packet[4], acl_length );
			conn->acl_recombination_pos += acl_length;

			// log_error( "ACL Cont Fragment: acl_len %u, combined_len %u, l2cap_len %u", acl_length,
			//        conn->acl_recombination_pos, conn->acl_recombination_length);  

			// forward complete L2CAP packet if complete. 
			if (conn->acl_recombination_pos >= conn->acl_recombination_length + 4 + 4){ // pos already incl. ACL header

				hci_stack->packet_handler(HCI_ACL_DATA_PACKET,
						&conn->acl_recombination_buffer[HCI_INCOMING_PRE_BUFFER_SIZE],
						conn->acl_recombination_pos);
				// reset recombination buffer
				conn->acl_recombination_length = 0;
				conn->acl_recombination_pos = 0;
			}
			break;

		case 0x02: 
			{ // first fragment

				// sanity check
				if (conn->acl_recombination_pos) {
					conn->acl_recombination_pos = 0;
				}

				// peek into L2CAP packet!
				uint16_t l2cap_length = READ_L2CAP_LENGTH( packet );

				// log_info( "ACL First Fragment: acl_len %u, l2cap_len %u", acl_length, l2cap_length);

				// compare fragment size to L2CAP packet size
				if (acl_length >= l2cap_length + 4){

					// forward fragment as L2CAP packet
					hci_stack->packet_handler(HCI_ACL_DATA_PACKET, packet, acl_length + 4);
				} else {

					if (acl_length > HCI_ACL_BUFFER_SIZE){
						return;
					}

					// store first fragment and tweak acl length for complete package
					memcpy(&conn->acl_recombination_buffer[HCI_INCOMING_PRE_BUFFER_SIZE], packet, acl_length + 4);
					conn->acl_recombination_pos    = acl_length + 4;
					conn->acl_recombination_length = l2cap_length;
					bt_store_16(conn->acl_recombination_buffer, HCI_INCOMING_PRE_BUFFER_SIZE + 2, l2cap_length +4);
				}
			}
			break;

		default:
			ASSERT(0, "%s\n", "acl_flags error\n");
			return;
	}

	// execute main loop
	le_hci_run();
}

static void hci_shutdown_connection(hci_connection_t *conn){
    
    /* printf_buf(0x0, 0x10); */
    /* printf("conn->timeout %x\n", &conn->timeout); */
    /* printf("conn->timeout.entry %x\n", &conn->timeout.entry); */
    //bug fix : no match register
    /* sys_timer_remove(&conn->timeout); */
    
    /* printf_buf(0x0, 0x10); */
    linked_list_remove(&hci_stack->connections, (linked_item_t *) conn);
    le_btstack_memory_hci_connection_free( conn );
    
    // now it's gone
    le_hci_emit_nr_connections_changed();
}


uint8_t* le_hci_get_outgoing_packet_buffer(void){
    // hci packet buffer is >= acl data packet length
    return hci_stack->hci_packet_buffer;
}

uint16_t le_hci_max_acl_data_packet_length(void){
    return hci_stack->acl_data_packet_length;
}

int hci_non_flushable_packet_boundary_flag_supported(void){
    // No. 54, byte 6, bit 6
    return (hci_stack->local_supported_features[6] & (1 << 6)) != 0;
}

int hci_ssp_supported(void){
    // No. 51, byte 6, bit 3
    return (hci_stack->local_supported_features[6] & (1 << 3)) != 0;
}

int hci_classic_supported(void){
    // No. 37, byte 4, bit 5, = No BR/EDR Support
    return (hci_stack->local_supported_features[4] & (1 << 5)) == 0;
}


// get addr type and address used in advertisement packets
void hci_le_advertisement_address(uint8_t * addr_type, bd_addr_t  addr){
    *addr_type = hci_stack->adv_addr_type;
    /* puts(__func__); */
    if (hci_stack->adv_addr_type){
        memcpy(addr, hci_stack->adv_address, 6);
        /* puts("****random addr \n"); */
    } else 
    {
        memcpy(addr, hci_stack->local_bd_addr, 6);
        /* puts("****public\n"); */
    }
    /* printf_buf(addr ,6); */
}

void le_handle_advertisement_report(uint8_t *packet, int size)
{
    int i;
    int offset = 3;
    uint8_t event[12 + LE_ADVERTISING_DATA_SIZE]; // use upper bound to avoid var size automatic var
    int num_reports = packet[offset];
    offset += 1;

    for (i=0; i<num_reports;i++)
	{
        uint8_t data_length = packet[offset + 8];
        uint8_t event_size = 10 + data_length;
        int pos = 0;
        event[pos++] = GAP_LE_ADVERTISING_REPORT;
        event[pos++] = event_size;
        memcpy(&event[pos], &packet[offset], 1+1+6); // event type + address type + address
        offset += 8;
        pos += 8;
        event[pos++] = packet[offset + 1 + data_length]; // rssi
        event[pos++] = packet[offset++]; //data_length;
        memcpy(&event[pos], &packet[offset], data_length);
        pos += data_length;
        offset += data_length + 1; // rssi
        hci_stack->packet_handler(HCI_EVENT_PACKET, event, pos);
    }
}


static void hci_initializing_next_state(){
    hci_stack->substate = (hci_substate_t )( ((int) hci_stack->substate) + 1);
	/* puts("next_substate\n"); */
}

#ifdef BT16
static const u8 adv_ind_data[] = {
	0x02, 0x01, 0x06,
	0x05, 0x09, 'b','t', '1', '6',
	//0x05, 0x12, 0x80, 0x02, 0x80, 0x02,
	0x04, 0x0d, 0x00, 0x05, 0x10,
	0x03, 0x03, 0x0d, 0x18,
};
#endif

#ifdef BR16
static const u8 adv_ind_data[] = {
	0x02, 0x01, 0x06,
	0x09, 0x09, 'b','r', '1', '6', '-','4', '.', '2',
	//0x05, 0x12, 0x80, 0x02, 0x80, 0x02,
	0x04, 0x0d, 0x00, 0x05, 0x10,
	0x03, 0x03, 0x0d, 0x18,
};
#endif

#ifdef BR17
static const u8 adv_ind_data[] = {
	0x02, 0x01, 0x06,
	0x09, 0x09, 'b','r', '1', '7', '-','4', '.', '2',
	//0x05, 0x12, 0x80, 0x02, 0x80, 0x02,
	0x04, 0x0d, 0x00, 0x05, 0x10,
	0x03, 0x03, 0x0d, 0x18,
};
#endif

//len, type, Manufacturer Specific data
static const u8 scan_rsp_data[] = {
	30,  0xff, 0x57, 0x01, 0x00, 0x7c, 0x5b, 0x5d, 0x17,
	0x0e, 0x02, 0xbd, 0xd7, 0x0c, 0xbb, 0x8a, 0xcc, 0xc3,
	0x3c, 0xf2, 0x86, 0x0f, 0x00, 0x00, 0x00, 0x88, 0x0f,
	0x10, 0x53, 0x49, 0x56,
};

struct resolving_list_parameter{
    u8 peer_identity_address_type;  
    u8 peer_identity_address[6];
    u8 peer_irk[16];
    u8 local_irk[16];
};


struct resolving_list_parameter rpa[] = {
   [0] = { 
       .peer_identity_address_type = 0,
       .peer_identity_address = {0x46, 0x01, 0x70, 0xAC, 0xF5, 0xBC},  //LSB->MSB
       .peer_irk = {0x74, 0x01, 0x90, 0x4A, 0xF7, 0x35, 0xA4, 0x38, 0xD3, 0x20, 0x9A, 0xCD, 0x48, 0xC0, 0x54, 0x40},
       .local_irk = {0xE6, 0xEA, 0xEE, 0x60, 0x31, 0x7B, 0xFC, 0xA2, 0x3F, 0xA5, 0x79, 0x59, 0xE7, 0x41, 0xCF, 0xC7},
   },
};

// assumption: hci_can_send_command_packet_now() == true
static void hci_initializing_run()
{
    puts("hci init run : ");
    switch (hci_stack->substate)
	{
        case HCI_INIT_SEND_RESET:
			puts("HCI_INIT_SEND_RESET\n");
            hci_state_reset();
            /*-TODO-*/
            hci_stack->substate = HCI_INIT_W4_SEND_RESET;
            le_hci_send_cmd(&hci_reset);
            break;
		case HCI_INIT_READ_BD_ADDR:
			puts("HCI_INIT_READ_BD_ADDR\n");
            hci_stack->substate = HCI_INIT_W4_READ_BD_ADDR;
			le_hci_send_cmd(&hci_read_bd_addr);
			break;
        // LE INIT
        case HCI_INIT_LE_READ_BUFFER_SIZE:
			puts("HCI_INIT_LE_READ_BUFFER_SIZE\n");
            hci_stack->substate = HCI_INIT_W4_LE_READ_BUFFER_SIZE;
            le_hci_send_cmd(&hci_le_read_buffer_size);
            break;
        case HCI_INIT_WRITE_LE_HOST_SUPPORTED:
			puts("HCI_INIT_WRITE_LE_HOST_SUPPORTED\n");
            // LE Supported Host = 1, Simultaneous Host = 0
            hci_stack->substate = HCI_INIT_W4_WRITE_LE_HOST_SUPPORTED;
            le_hci_send_cmd(&hci_write_le_host_supported, 1, 0);
            break;
        case HCI_INIT_READ_WHITE_LIST_SIZE:
			puts("HCI_INIT_LE_SET_SCAN_PARAMETERS\n");
            hci_stack->substate = HCI_INIT_W4_READ_WHITE_LIST_SIZE;
            le_hci_send_cmd(&hci_le_read_white_list_size);
            break;

        case HCI_INIT_LE_SET_ADV_PARAMETERS:
                puts("HCI_INIT_LE_SET_ADV_PARAMETERS\n");
                hci_stack->substate = HCI_INIT_W4_LE_SET_ADV_PARAMETERS;
                le_hci_send_cmd(&hci_le_set_advertising_parameters,
                    0x0320, 0x0320, 
                    0x00, 0x00, 
                    0x0, rpa[0].peer_identity_address, 
                    0x7, 0x0);
                break;
        case HCI_INIT_LE_SET_ADV_DATA:
            puts("HCI_INIT_LE_SET_ADV_DATA\n");
            hci_stack->substate = HCI_INIT_W4_LE_SET_ADV_DATA;
            le_hci_send_cmd(&hci_le_set_advertising_data, sizeof(adv_ind_data),sizeof(adv_ind_data),  adv_ind_data);
                break;
        case HCI_INIT_LE_SET_RSP_DATA:
            puts("HCI_INIT_LE_SET_RSP_DATA\n");
            hci_stack->substate = HCI_INIT_W4_LE_SET_RSP_DATA;
            le_hci_send_cmd(&hci_le_set_scan_response_data, sizeof(scan_rsp_data), sizeof(scan_rsp_data), scan_rsp_data);
            break;
        //privacy 
        /* case HCI_INIT_LE_READ_RESOLVING_LIST_SIZE: */
            /* puts("HCI_INIT_LE_READ_RESOLVING_LIST_SIZE\n"); */
            /* hci_stack->substate = HCI_INIT_W4_LE_READ_RESOLVING_LIST_SIZE; */
            /* le_hci_send_cmd(&hci_le_read_resolving_list_size); */
            /* break; */
        /* case HCI_INIT_LE_ADD_DEVICE_TO_RESOLVING_LIST: */
            /* puts("HCI_INIT_LE_ADD_DEVICE_TO_RESOLVING_LIST\n"); */
            /* hci_stack->substate = HCI_INIT_W4_LE_ADD_DEVICE_TO_RESOLVING_LIST; */
            /* le_hci_send_cmd(&hci_le_add_device_to_resolving_list,  */
                    /* rpa[0].peer_identity_address_type, */
                    /* rpa[0].peer_identity_address, */
                    /* rpa[0].peer_irk, */
                    /* rpa[0].local_irk); */
            /* break; */
        /* case HCI_INIT_LE_SET_RANDOM_PRIVATE_ADDRESS_TIMEOUT: */
            /* puts("HCI_INIT_LE_SET_RANDOM_PRIVATE_ADDRESS_TIMEOUT\n"); */
            /* hci_stack->substate = HCI_INIT_W4_LE_SET_RANDOM_PRIVATE_ADDRESS_TIMEOUT; */
            /* le_hci_send_cmd(&hci_le_set_resolvable_private_address_timeout, 0x5); */
            /* break; */
        /* case HCI_INIT_LE_ADDRESS_RESOLUTION_ENABLE: */
            /* puts("HCI_INIT_LE_ADDRESS_RESOLUTION_ENABLE\n"); */
            /* hci_stack->substate = HCI_INIT_W4_LE_ADDRESS_RESOLUTION_ENABLE; */
            /* le_hci_send_cmd(&hci_le_set_address_resolution_enable, 1); */
            /* break; */

        case HCI_INIT_LE_SET_ADV_EN:
			puts("HCI_INIT_LE_SET_ADV_EN\n");
            hci_stack->substate = HCI_INIT_W4_LE_SET_ADV_EN;
            le_hci_send_cmd(&hci_le_set_advertise_enable, 1);
            break;
        // DONE
        case HCI_INIT_DONE:
            // done.
			puts("HCI_STATE_WORKING\n");
            hci_stack->state = HCI_STATE_WORKING;
            hci_emit_state();
            return;
        default:
            return;
    }
}

static void hci_initializing_event_handler(uint8_t * packet, uint16_t size)
{
    uint8_t command_completed = 0;


    if (packet[0] == HCI_EVENT_COMMAND_COMPLETE){
        uint16_t opcode = READ_BT_16(packet,3);
        if (opcode == hci_stack->last_cmd_opcode){
            command_completed = 1;
        }
        puts("command_completed\n");
    }
    if (packet[0] == HCI_EVENT_COMMAND_STATUS){
        uint8_t  status = packet[2];
        uint16_t opcode = READ_BT_16(packet,4);
        if (opcode == hci_stack->last_cmd_opcode){
            if (status){
                command_completed = 1;
            }
        }
        puts("command_status\n");
    }


    if (!command_completed) return;

    hci_initializing_next_state();
}

// avoid huge local variables
static void event_handler(uint8_t *packet, int size)
{
	bd_addr_t addr;
	bd_addr_type_t addr_type;
	uint8_t link_type;
	hci_con_handle_t handle;
	hci_connection_t * conn;
	int i;
	uint16_t event_length = packet[1];

	// assert packet is complete
	if (size != event_length + 2){
		return;
	}

	switch (packet[0]) {
		case HCI_EVENT_COMMAND_COMPLETE:
			// get num cmd packets
			hci_stack->num_cmd_packets = packet[2];

            //data length extension begin
			if (COMMAND_COMPLETE_EVENT(packet, hci_le_read_suggested_default_data_length)){

                    printf("suggestedMaxTxOctets: %04x\n",  READ_BT_16(packet, 6));
                    printf("suggestedMaxTxTime: %04x\n",    READ_BT_16(packet, 8));
            }
			if (COMMAND_COMPLETE_EVENT(packet, hci_le_read_maximum_data_length)){

                    printf("supportedMaxTxOctets: %04x\n",  READ_BT_16(packet, 6));
                    printf("supportedMaxTxTime: %04x\n",    READ_BT_16(packet, 8));
                    printf("supportedMaxRxOctets: %04x\n",  READ_BT_16(packet, 10));
                    printf("supportedMaxRxTime: %04x\n",    READ_BT_16(packet, 12));
            }
            //data length extension end
            //
			if (COMMAND_COMPLETE_EVENT(packet, hci_le_read_buffer_size)){
				hci_stack->le_data_packets_length = READ_BT_16(packet, 6);
				hci_stack->le_acl_packets_total_num  = packet[8];
                printf("acl pkt length %x\n", hci_stack->le_data_packets_length);
                printf("acl pkt num %x\n", hci_stack->le_acl_packets_total_num);
				// determine usable ACL payload size
				if (HCI_ACL_PAYLOAD_SIZE < hci_stack->le_data_packets_length){
					hci_stack->le_data_packets_length = HCI_ACL_PAYLOAD_SIZE;
				}
                if (hci_stack->le_data_packets_length < 27)
                {
                    //spec 4.2 Vol 2 Part E
                    puts("Host shall not fragment HCI ACL Data Packets on an LE-U logial link\n");
                }
			}
            if (COMMAND_COMPLETE_EVENT(packet, hci_le_read_white_list_size)){
                hci_stack->le_whitelist_capacity = READ_BT_16(packet, 6);
                printf("le_whitelist_capacity %x\n", hci_stack->le_whitelist_capacity);
                log_info("hci_le_read_white_list_size: size %u", hci_stack->le_whitelist_capacity);
            }   
			if (COMMAND_COMPLETE_EVENT(packet, hci_read_bd_addr)){
				bt_flip_addr(hci_stack->local_bd_addr, 
						&packet[OFFSET_OF_DATA_IN_COMMAND_COMPLETE+1]);
                puts("hci_read_bd_addr");printf_buf(hci_stack->local_bd_addr, 6);
			}
			if (COMMAND_COMPLETE_EVENT(packet, hci_le_read_resolving_list_size)){
                hci_stack->le_resolvinglist_capacity = READ_BT_16(packet, 6);
                printf("le_resolvinglist_capacit %x\n", hci_stack->le_resolvinglist_capacity);
            }
			break;

		case HCI_EVENT_COMMAND_STATUS:
			// get num cmd packets
			hci_stack->num_cmd_packets = packet[3];
			break;

		case HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS:
			{
				int offset = 3;

				for (i=0; i<packet[2];i++)
				{
					handle = READ_BT_16(packet, offset);
					offset += 2;
					uint16_t num_packets = READ_BT_16(packet, offset);
					offset += 2;
                    printf("acl pkt complete\n", num_packets);

					conn = le_hci_connection_for_handle(handle);
					if (!conn){
						continue;
					}

					if (conn->num_acl_packets_sent >= num_packets){
						conn->num_acl_packets_sent -= num_packets;
					} else {
						conn->num_acl_packets_sent = 0;
					}
                    printf("acl pkt remain\n", conn->num_acl_packets_sent);
				}
			}
			break;

		case HCI_EVENT_ENCRYPTION_CHANGE:
			handle = READ_BT_16(packet, 3);
			conn = le_hci_connection_for_handle(handle);
			if (!conn) break;
			if (packet[2] == 0) {
				if (packet[5]){
					conn->authentication_flags |= CONNECTION_ENCRYPTED;
				} else {
					conn->authentication_flags &= ~CONNECTION_ENCRYPTED;
				}
			}
			hci_emit_security_level(handle, gap_security_level_for_connection(conn));
			break;

		case HCI_EVENT_AUTHENTICATION_COMPLETE_EVENT:
			handle = READ_BT_16(packet, 3);
			conn = le_hci_connection_for_handle(handle);
			if (!conn) break;

			// dedicated bonding: send result and disconnect
			if (conn->bonding_flags & BONDING_DEDICATED){
				conn->bonding_flags &= ~BONDING_DEDICATED;
				conn->bonding_flags |= BONDING_DISCONNECT_DEDICATED_DONE;
				conn->bonding_status = packet[2];
				break;
			}

			if (packet[2] == 0 && 
					gap_security_level_for_link_key_type(conn->link_key_type) >= conn->requested_security_level){
				// link key sufficient for requested security
				conn->bonding_flags |= BONDING_SEND_ENCRYPTION_REQUEST;
				break;
			}
			// not enough
			hci_emit_security_level(handle, gap_security_level_for_connection(conn));
			break;

		case HCI_EVENT_DISCONNECTION_COMPLETE:
			if (packet[2]) break;   // status != 0
			handle = READ_BT_16(packet, 3);
			printf("hci_event_disconnect: %x\n", handle);
			conn = le_hci_connection_for_handle(handle);
			if (!conn) break;       // no conn struct anymore
			conn->state = RECEIVED_DISCONNECTION_COMPLETE;
			break;

		case HCI_EVENT_LE_META:
			switch (packet[2])
			{
                case HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE:
					puts("le_data_length_change\n");
                    printf("connEffectiveMaxTxOctets: %04x\n",    packet[5]<<8|packet[4]);
                    printf("connEffectiveMaxTxTime:   %04x\n",    packet[7]<<8|packet[6]);
                    printf("connEffectiveMaxRxOctets: %04x\n",    packet[9]<<8|packet[8]);
                    printf("connEffectiveMaxRxTime:   %04x\n",    packet[11]<<8|packet[10]);
                    break;
				case HCI_SUBEVENT_LE_ADVERTISING_REPORT:
					puts("advertising report received");
					if (hci_stack->le_scanning_state != LE_SCANNING)
					   	break;
					le_handle_advertisement_report(packet, size);
					break;

                case HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE_EVENT:
					puts("le_enhanced_connection_complete\n");
                    printf("interval: %04x\n",    packet[27]<<8|packet[26]);
                    printf("latency : %04x\n",    packet[29]<<8|packet[28]);
                    printf("timeout : %04x\n",    packet[31]<<8|packet[30]);
                    puts("local RPA : "); printf_buf(&packet[14], 6);
                    bt_flip_addr(hci_stack->adv_address, &packet[14]);
                    puts("peer RPA : "); printf_buf(&packet[20], 6);
                    puts("\n");
				case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                    //Status error
					// Connection management
					bt_flip_addr(addr, &packet[8]);
					addr_type = (bd_addr_type_t)packet[7];

					// LE connections are auto-accepted, so just create a connection if there isn't one already
					conn = le_hci_connection_for_bd_addr_and_type(addr, addr_type);
					// if auto-connect, remove from whitelist in both roles
                    if (hci_stack->le_connecting_state == LE_CONNECTING_WHITELIST){
                        hci_remove_from_whitelist(addr_type, addr);  
                    }
					if (packet[3]){
                        // outgoing connection establishment is done
                        hci_stack->le_connecting_state = LE_CONNECTING_IDLE;
						if (conn){
							// outgoing connection failed, remove entry
							linked_list_remove(&hci_stack->connections, (linked_item_t *) conn);
							le_btstack_memory_hci_connection_free( conn );
						}
						// if authentication error, also delete link key
						if (packet[3] == 0x05) {
							hci_drop_link_key_for_bd_addr(addr);
						}
                        //DIRECTED_ADVERTISING_TIMEOUT
                        if (packet[3] == 0x3C) 
                        {
                        }
						break;
					}
					puts("le_connection_complete\n");
					// on success, both hosts receive connection complete event
                    if (packet[6] == HCI_ROLE_MASTER){
                        // if we're master, it was an outgoing connection and we're done with it
                        hci_stack->le_connecting_state = LE_CONNECTING_IDLE;
                    } else {
                        // if we're slave, it was an incoming connection, advertisements have stopped
                        hci_stack->le_advertisements_active = 0;
                    }
					if (!conn){
						conn = create_connection_for_bd_addr_and_type(addr, addr_type);
					}
					if (!conn){
						// no memory
						ASSERT(0, "%s\n", "no memory");
						break;
					}

					conn->state = OPEN;
					conn->con_handle = READ_BT_16(packet, 4);

					// TODO: store - role, peer address type, conn_interval, conn_latency, supervision timeout, master clock

					// restart timer
					// run_loop_set_timer(&conn->timeout, HCI_CONNECTION_TIMEOUT_MS);
					// run_loop_add_timer(&conn->timeout);

					le_hci_emit_nr_connections_changed();
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}

	// handle BT initialization
	if (hci_stack->state == HCI_STATE_INITIALIZING){
		hci_initializing_event_handler(packet, size);
	}
#if 0 
	// help with BT sleep
	if (hci_stack->state == HCI_STATE_FALLING_ASLEEP
			&& hci_stack->substate == HCI_FALLING_ASLEEP_W4_WRITE_SCAN_ENABLE
			&& COMMAND_COMPLETE_EVENT(packet, hci_write_scan_enable)){
		hci_initializing_next_state();
	}
#endif

	// notify upper stack
	hci_stack->packet_handler(HCI_EVENT_PACKET, packet, size);

	// moved here to give upper stack a chance to close down everything with hci_connection_t intact
	if (packet[0] == HCI_EVENT_DISCONNECTION_COMPLETE){
		if (!packet[2]){
			handle = READ_BT_16(packet, 3);
			hci_connection_t * conn = le_hci_connection_for_handle(handle);

            /* printf("conn handle %x\n", handle); */
			if (conn) {
                /* printf("conn->addr %02x - %02x - %02x - %02x - %02x - %02x \n", conn->address[0], conn->address[1],conn->address[2],conn->address[3],conn->address[4],conn->address[5]); */
				/*uint8_t status = conn->bonding_status;
				  uint16_t flags = conn->bonding_flags;
				bd_addr_t bd_address;
				memcpy(&bd_address, conn->address, 6);*/
                /* printf_buf(0x0, 0x10); */
				hci_shutdown_connection(conn);
#if 0
				// connection struct is gone, don't access anymore
				if (flags & BONDING_EMIT_COMPLETE_ON_DISCONNECT){
					hci_emit_dedicated_bonding_result(bd_address, status);
				}
#endif
			}
		}
	}

	// execute main loop
	le_hci_run();
}


static void packet_handler(uint8_t packet_type, uint8_t *packet, uint16_t size)
{
    /* puts("--HCI PH "); */
    switch (packet_type) 
	{
        case HCI_EVENT_PACKET:
            event_handler(packet, size);
            break;
        case HCI_ACL_DATA_PACKET:
            /* puts("HCI_ACL_DATA_PACKET\n"); */
            acl_handler(packet, size);
            break;
        default:
            break;
    }
    /* puts("\nHCI exit "); */
}

/** Register HCI packet handlers */
void le_hci_register_packet_handler(void (*handler)(uint8_t packet_type, uint8_t *packet, uint16_t size)){
    hci_stack->packet_handler = handler;
}

static void hci_state_reset()
{
    // no connections yet
    hci_stack->connections = NULL;

    // keep discoverable/connectable as this has been requested by the client(s)
    // hci_stack->discoverable = 0;
    // hci_stack->connectable = 0;
    // hci_stack->bondable = 1;

    // buffer is free
    hci_stack->hci_packet_buffer_reserved = 0;

    // no pending cmds
    hci_stack->decline_reason = 0;
    /*hci_stack->new_scan_enable_value = 0xff;*/
    
    // LE
    hci_stack->adv_addr_type = 0;
    memset(hci_stack->adv_address, 0, 6);
    hci_stack->le_scanning_state = LE_SCAN_IDLE;
    hci_stack->le_scan_type = 0xff; 
    hci_stack->le_connecting_state = LE_CONNECTING_IDLE;
    hci_stack->le_whitelist = 0;
    hci_stack->le_whitelist_capacity = 0;
    hci_stack->le_connection_parameter_range.le_conn_interval_min =          6; 
    hci_stack->le_connection_parameter_range.le_conn_interval_max =       3200;
    hci_stack->le_connection_parameter_range.le_conn_latency_min =           0;
    hci_stack->le_connection_parameter_range.le_conn_latency_max =         500;
    hci_stack->le_connection_parameter_range.le_supervision_timeout_min =   10;
    hci_stack->le_connection_parameter_range.le_supervision_timeout_max = 3200;
}

void le_hci_init(hci_transport_t *transport, void *config, bt_control_t *control, remote_device_db_t const* remote_device_db)
{
#ifdef HAVE_MALLOC
    if (!hci_stack) {
        /* hci_stack = (hci_stack_t*) malloc(sizeof(hci_stack_t)); */
        hci_stack = (hci_stack_t*) bt_malloc(BTMEM_HIGHLY_AVAILABLE, sizeof(hci_stack_t));
    }
#else
    hci_stack = &hci_stack_static;
#endif
    memset(hci_stack, 0, sizeof(hci_stack_t));

    // reference to use transport layer implementation
    hci_stack->hci_transport = transport;
    
    // references to used control implementation
    hci_stack->control = control;
    
    // reference to used config
    hci_stack->config = config;
    
    // higher level handler
    hci_stack->packet_handler = dummy_handler;

    // store and open remote device db
    hci_stack->remote_device_db = remote_device_db;
    if (hci_stack->remote_device_db) {
        hci_stack->remote_device_db->open();
    }
    
    // max acl payload size defined in config.h
    hci_stack->acl_data_packet_length = HCI_ACL_PAYLOAD_SIZE;
    
    // register packet handlers with transport
    transport->register_packet_handler(&packet_handler);

    hci_stack->state = HCI_STATE_OFF;

    // class of device
    hci_stack->class_of_device = 0x007a020c; // Smartphone 

    // bondable by default
    hci_stack->bondable = 1;

    // Secure Simple Pairing default: enable, no I/O capabilities, general bonding, mitm not required, auto accept 
    /*hci_stack->ssp_enable = 1;
    hci_stack->ssp_io_capability = SSP_IO_CAPABILITY_NO_INPUT_NO_OUTPUT;
    hci_stack->ssp_authentication_requirement = SSP_IO_AUTHREQ_MITM_PROTECTION_NOT_REQUIRED_GENERAL_BONDING;
    hci_stack->ssp_auto_accept = 1;*/

    hci_state_reset();
}
#if 0
void hci_close(){
    // close remote device db
    if (hci_stack->remote_device_db) {
        hci_stack->remote_device_db->close();
    }
    while (hci_stack->connections) {
        // cancel all l2cap connections
        hci_emit_disconnection_complete(((hci_connection_t *) hci_stack->connections)->con_handle, 0x16); // terminated by local host
        hci_shutdown_connection((hci_connection_t *) hci_stack->connections);
    }
    hci_power_control(HCI_POWER_OFF);
    
#ifdef HAVE_MALLOC
    free(hci_stack);
#endif
    hci_stack = NULL;
}
#endif

void le_hci_set_class_of_device(uint32_t class_of_device){
    hci_stack->class_of_device = class_of_device;
}

// Set Public BD ADDR - passed on to Bluetooth chipset if supported in bt_control_h
void le_hci_set_bd_addr(bd_addr_t addr){
    memcpy(hci_stack->custom_bd_addr, addr, 6);
    hci_stack->custom_bd_addr_set = 1;
}

void le_hci_disable_l2cap_timeout_check(){
    disable_l2cap_timeouts = 1;
}
// State-Module-Driver overview
// state                    module  low-level 
// HCI_STATE_OFF             off      close
// HCI_STATE_INITIALIZING,   on       open
// HCI_STATE_WORKING,        on       open
// HCI_STATE_HALTING,        on       open
// HCI_STATE_SLEEPING,    off/sleep   close
// HCI_STATE_FALLING_ASLEEP  on       open

static int hci_power_control_on(void){
    
    // power on
    int err = 0;
    if (hci_stack->control && hci_stack->control->on){
        err = (*hci_stack->control->on)(hci_stack->config);
    }
    if (err){
        log_error( "POWER_ON failed");
        le_hci_emit_hci_open_failed();
        return err;
    }
    
    // open low-level device
    err = hci_stack->hci_transport->open(hci_stack->config);
    if (err){
        log_error( "HCI_INIT failed, turning Bluetooth off again");
        if (hci_stack->control && hci_stack->control->off){
            (*hci_stack->control->off)(hci_stack->config);
        }
        le_hci_emit_hci_open_failed();
        return err;
    }
    return 0;
}
#if 0

static void hci_power_control_off(void){
    
    log_info("hci_power_control_off");

    // close low-level device
    hci_stack->hci_transport->close(hci_stack->config);

    log_info("hci_power_control_off - hci_transport closed");
    
    // power off
    if (hci_stack->control && hci_stack->control->off){
        (*hci_stack->control->off)(hci_stack->config);
    }
    
    log_info("hci_power_control_off - control closed");

    hci_stack->state = HCI_STATE_OFF;
}

static void hci_power_control_sleep(void){
    
    log_info("hci_power_control_sleep");
    
#if 0
    // don't close serial port during sleep
    
    // close low-level device
    hci_stack->hci_transport->close(hci_stack->config);
#endif
    
    // sleep mode
    if (hci_stack->control && hci_stack->control->sleep){
        (*hci_stack->control->sleep)(hci_stack->config);
    }
    
    hci_stack->state = HCI_STATE_SLEEPING;
}

static int hci_power_control_wake(void){
    
    log_info("hci_power_control_wake");

    // wake on
    if (hci_stack->control && hci_stack->control->wake){
        (*hci_stack->control->wake)(hci_stack->config);
    }
    
#if 0
    // open low-level device
    int err = hci_stack->hci_transport->open(hci_stack->config);
    if (err){
        log_error( "HCI_INIT failed, turning Bluetooth off again");
        if (hci_stack->control && hci_stack->control->off){
            (*hci_stack->control->off)(hci_stack->config);
        }
        le_hci_emit_hci_open_failed();
        return err;
    }
#endif
    
    return 0;
}
#endif

static void hci_power_transition_to_initializing(void){
    // set up state machine
    hci_stack->num_cmd_packets = 1; // assume that one cmd can be sent
    hci_stack->hci_packet_buffer_reserved = 0;
    hci_stack->state = HCI_STATE_INITIALIZING;
    hci_stack->substate = HCI_INIT_SEND_RESET;
}

int le_hci_power_control(HCI_POWER_MODE power_mode)
{
    int err = 0;

    switch (hci_stack->state)
	{
        case HCI_STATE_OFF:
            switch (power_mode)
			{
                case HCI_POWER_ON:
                    err = hci_power_control_on();
                    if (err) {
                        return err;
                    }
                    hci_power_transition_to_initializing();
                    break;
                case HCI_POWER_OFF:
                    // do nothing
                    break;  
                case HCI_POWER_SLEEP:
                    // do nothing (with SLEEP == OFF)
                    break;
            }
            break;
            
#if 0
        case HCI_STATE_INITIALIZING:
            switch (power_mode){
                case HCI_POWER_ON:
                    // do nothing
                    break;
                case HCI_POWER_OFF:
                    // no connections yet, just turn it off
                    hci_power_control_off();
                    break;  
                case HCI_POWER_SLEEP:
                    // no connections yet, just turn it off
                    hci_power_control_sleep();
                    break;
            }
            break;
            
        case HCI_STATE_WORKING:
            switch (power_mode){
                case HCI_POWER_ON:
                    // do nothing
                    break;
                case HCI_POWER_OFF:
                    // see hci_run
                    hci_stack->state = HCI_STATE_HALTING;
                    break;  
                case HCI_POWER_SLEEP:
                    // see hci_run
                    hci_stack->state = HCI_STATE_FALLING_ASLEEP;
                    hci_stack->substate = HCI_FALLING_ASLEEP_DISCONNECT;
                    break;
            }
            break;
            
        case HCI_STATE_HALTING:
            switch (power_mode){
                case HCI_POWER_ON:
                    hci_power_transition_to_initializing();
                    break;
                case HCI_POWER_OFF:
                    // do nothing
                    break;  
                case HCI_POWER_SLEEP:
                    // see hci_run
                    hci_stack->state = HCI_STATE_FALLING_ASLEEP;
                    hci_stack->substate = HCI_FALLING_ASLEEP_DISCONNECT;
                    break;
            }
            break;
            
        case HCI_STATE_FALLING_ASLEEP:
            switch (power_mode){
                case HCI_POWER_ON:

#if defined(USE_POWERMANAGEMENT) && defined(USE_BLUETOOL)
                    // nothing to do, if H4 supports power management
                    if (bt_control_iphone_power_management_enabled()){
                        hci_stack->state = HCI_STATE_INITIALIZING;
                        hci_stack->substate = HCI_INIT_WRITE_SCAN_ENABLE;   // init after sleep
                        break;
                    }
#endif
                    hci_power_transition_to_initializing();
                    break;
                case HCI_POWER_OFF:
                    // see hci_run
                    hci_stack->state = HCI_STATE_HALTING;
                    break;  
                case HCI_POWER_SLEEP:
                    // do nothing
                    break;
            }
            break;
            
        case HCI_STATE_SLEEPING:
            switch (power_mode){
                case HCI_POWER_ON:
                    
#if defined(USE_POWERMANAGEMENT) && defined(USE_BLUETOOL)
                    // nothing to do, if H4 supports power management
                    if (bt_control_iphone_power_management_enabled()){
                        hci_stack->state = HCI_STATE_INITIALIZING;
                        hci_stack->substate = HCI_INIT_AFTER_SLEEP;
                        hci_update_scan_enable();
                        break;
                    }
#endif
                    err = hci_power_control_wake();
                    if (err) return err;
                    hci_power_transition_to_initializing();
                    break;
                case HCI_POWER_OFF:
                    hci_stack->state = HCI_STATE_HALTING;
                    break;  
                case HCI_POWER_SLEEP:
                    // do nothing
                    break;
            }
            break;
#endif
    }

    // create internal event
	hci_emit_state();
    
	// trigger next/first action
	le_hci_run();
	
    return 0;
}


void le_hci_local_bd_addr(bd_addr_t address_buffer){
    memcpy(address_buffer, hci_stack->local_bd_addr, 6);
}

void le_hci_run(void)
{

	// log_info("hci_run: entered");
    /* hci_puts("hci_run: entered"); */
	hci_connection_t * connection;
	linked_item_t * it;

	// send continuation fragments first, as they block the prepared packet buffer
	if (hci_stack->acl_fragmentation_total_size > 0) {
		hci_con_handle_t con_handle = READ_ACL_CONNECTION_HANDLE(hci_stack->hci_packet_buffer);
		if (le_hci_can_send_prepared_acl_packet_now(con_handle)){
			hci_connection_t *connection = le_hci_connection_for_handle(con_handle);
			if (connection) {
				hci_send_acl_packet_fragments(connection);
				return;
			} 
			// connection gone -> discard further fragments
			hci_stack->acl_fragmentation_total_size = 0;
			hci_stack->acl_fragmentation_pos = 0;
		}
	}

	if (!le_hci_can_send_command_packet_now()) {
        puts("le_hci_run - le_hci_can_send_command_packet_now\n");
        return;   
    }

	// handle le scan
	if (hci_stack->state == HCI_STATE_WORKING){
		switch(hci_stack->le_scanning_state){
			case LE_START_SCAN:
				hci_stack->le_scanning_state = LE_SCANNING;
				le_hci_send_cmd(&hci_le_set_scan_enable, 1, 0);
				return;

			case LE_STOP_SCAN:
				hci_stack->le_scanning_state = LE_SCAN_IDLE;
				le_hci_send_cmd(&hci_le_set_scan_enable, 0, 0);
				return;
			default:
				break;
		}
		if (hci_stack->le_scan_type != 0xff){
			// defaults: active scanning, accept all advertisement packets
			int scan_type = hci_stack->le_scan_type;
			hci_stack->le_scan_type = 0xff;
			le_hci_send_cmd(&hci_le_set_scan_parameters, scan_type, 
					hci_stack->le_scan_interval, 
					hci_stack->le_scan_window,
					hci_stack->adv_addr_type,
					0);
			return;
		}
//
        // LE Whitelist Management
        //

        // check if whitelist needs modification
        linked_list_iterator_t lit;
        int modification_pending = 0;
        linked_list_iterator_init(&lit, &hci_stack->le_whitelist);
        while (linked_list_iterator_has_next(&lit)){
            whitelist_entry_t * entry = (whitelist_entry_t*) linked_list_iterator_next(&lit);
            if (entry->state & (LE_WHITELIST_REMOVE_FROM_CONTROLLER | LE_WHITELIST_ADD_TO_CONTROLLER)){
                modification_pending = 1;
                break;
            }
        }

        if (modification_pending){
            // stop connnecting if modification pending
            if (hci_stack->le_connecting_state != LE_CONNECTING_IDLE){
                le_hci_send_cmd(&hci_le_create_connection_cancel);
                return;
            }

            // add/remove entries
            linked_list_iterator_init(&lit, &hci_stack->le_whitelist);
            while (linked_list_iterator_has_next(&lit)){
                whitelist_entry_t * entry = (whitelist_entry_t*) linked_list_iterator_next(&lit);
                if (entry->state & LE_WHITELIST_ADD_TO_CONTROLLER){
                    entry->state = LE_WHITELIST_ON_CONTROLLER;
                    le_hci_send_cmd(&hci_le_add_device_to_white_list, entry->address_type, entry->address);
                    return;

                }
                if (entry->state & LE_WHITELIST_REMOVE_FROM_CONTROLLER){
                    bd_addr_t address;
                    bd_addr_type_t address_type = entry->address_type;                    
                    memcpy(address, entry->address, 6);
                    linked_list_remove(&hci_stack->le_whitelist, (linked_item_t *) entry);
                    btstack_memory_whitelist_entry_free(entry);
                    le_hci_send_cmd(&hci_le_remove_device_from_white_list, address_type, address);
                    return;
                }
            }
        }

        // start connecting
        if ( hci_stack->le_connecting_state == LE_CONNECTING_IDLE && 
            !linked_list_empty(&hci_stack->le_whitelist)){
            bd_addr_t null_addr;
            memset(null_addr, 0, 6);
            le_hci_send_cmd(&hci_le_create_connection,
                 0x0060,    // scan interval: 60 ms
                 0x0030,    // scan interval: 30 ms
                 1,         // use whitelist
                 0,         // peer address type
                 null_addr,      // peer bd addr
                 hci_stack->adv_addr_type, // our addr type:
                 0x0008,    // conn interval min
                 0x0018,    // conn interval max
                 0,         // conn latency
                 0x0048,    // supervision timeout
                 0x0001,    // min ce length
                 0x0001     // max ce length
                 );
            return;
        }
	}


	// send pending HCI commands
	for (it = (linked_item_t *) hci_stack->connections; it ; it = it->next){
		connection = (hci_connection_t *) it;

		switch(connection->state){
			case SEND_CREATE_CONNECTION:
				log_info("sending hci_le_create_connection");
				le_hci_send_cmd(&hci_le_create_connection,
						0x0060,    // scan interval: 60 ms
						0x0030,    // scan interval: 30 ms
						0,         // don't use whitelist
						connection->address_type, // peer address type
						connection->address,      // peer bd addr
						hci_stack->adv_addr_type, // our addr type:
						0x0008,    // conn interval min
						0x0018,    // conn interval max
						0,         // conn latency
						0x0048,    // supervision timeout
						0x0001,    // min ce length
						0x0001     // max ce length
						);
				connection->state = SENT_CREATE_CONNECTION;
				return;

			case SEND_CANCEL_CONNECTION:
				connection->state = SENT_CANCEL_CONNECTION;
				le_hci_send_cmd(&hci_le_create_connection_cancel);
				return;

			case SEND_DISCONNECT:
				connection->state = SENT_DISCONNECT;
				le_hci_send_cmd(&hci_disconnect, connection->con_handle, 0x13); // remote closed connection
				return;

			default:
				break;
		}
	}

	switch (hci_stack->state){
		case HCI_STATE_INITIALIZING:
			hci_initializing_run();
			break;
#if 0 
		case HCI_STATE_HALTING:

			log_info("HCI_STATE_HALTING");
			// close all open connections
			connection =  (hci_connection_t *) hci_stack->connections;
			if (connection){

				// send disconnect
                if (!le_hci_can_send_command_packet_now()) {
                    puts("le_hci_run2 - le_hci_can_send_command_packet_now\n");
                    return;   
                }

				log_info("HCI_STATE_HALTING, connection %p, handle %u", connection, (uint16_t)connection->con_handle);
				hci_send_cmd(&hci_disconnect, connection->con_handle, 0x13);  // remote closed connection

				// send disconnected event right away - causes higher layer connections to get closed, too.
				hci_shutdown_connection(connection);
				return;
			}
			log_info("HCI_STATE_HALTING, calling off");

			// switch mode
			hci_power_control_off();

			log_info("HCI_STATE_HALTING, emitting state");
			hci_emit_state();
			log_info("HCI_STATE_HALTING, done");
			break;

		case HCI_STATE_FALLING_ASLEEP:
			switch(hci_stack->substate) {
				case HCI_FALLING_ASLEEP_DISCONNECT:
					log_info("HCI_STATE_FALLING_ASLEEP");
					// close all open connections
					connection =  (hci_connection_t *) hci_stack->connections;

#if defined(USE_POWERMANAGEMENT) && defined(USE_BLUETOOL)
					// don't close connections, if H4 supports power management
					if (bt_control_iphone_power_management_enabled()){
						connection = NULL;
					}
#endif
					if (connection){

						// send disconnect
                        if (!le_hci_can_send_command_packet_now()) {
                            puts("le_hci_run3 - le_hci_can_send_command_packet_now\n");
                            return;   
                        }

						log_info("HCI_STATE_FALLING_ASLEEP, connection %p, handle %u", connection, (uint16_t)connection->con_handle);
						hci_send_cmd(&hci_disconnect, connection->con_handle, 0x13);  // remote closed connection

						// send disconnected event right away - causes higher layer connections to get closed, too.
						hci_shutdown_connection(connection);
						return;
					}

					if (hci_classic_supported()){
						// disable page and inquiry scan
                        if (!le_hci_can_send_command_packet_now()) {
                            puts("le_hci_run4 - le_hci_can_send_command_packet_now\n");
                            return;   
                        }

						log_info("HCI_STATE_HALTING, disabling inq scans");
						hci_send_cmd(&hci_write_scan_enable, hci_stack->connectable << 1); // drop inquiry scan but keep page scan

						// continue in next sub state
						hci_stack->substate = HCI_FALLING_ASLEEP_W4_WRITE_SCAN_ENABLE;
						break;
					}
					// fall through for ble-only chips

				case HCI_FALLING_ASLEEP_COMPLETE:
					log_info("HCI_STATE_HALTING, calling sleep");
#if defined(USE_POWERMANAGEMENT) && defined(USE_BLUETOOL)
					// don't actually go to sleep, if H4 supports power management
					if (bt_control_iphone_power_management_enabled()){
						// SLEEP MODE reached
						hci_stack->state = HCI_STATE_SLEEPING; 
						hci_emit_state();
						break;
					}
#endif
					// switch mode
					hci_power_control_sleep();  // changes hci_stack->state to SLEEP
					hci_emit_state();
					break;
				default:
					break;
			}
			break;
#endif                

		default:
			break;
	}
    /* puts("debug2\n"); */
}

int le_hci_send_cmd_packet(uint8_t *packet, int size)
{
    bd_addr_t addr;
    hci_connection_t * conn;
    
    if (IS_COMMAND(packet, hci_le_set_advertising_parameters)){
        hci_stack->adv_addr_type = packet[8] ? 1 : 0;
    }
    if (IS_COMMAND(packet, hci_le_set_random_address)){
        bt_flip_addr(hci_stack->adv_address, &packet[3]);
    }
    if (IS_COMMAND(packet, hci_le_set_advertise_enable)){
        hci_stack->le_advertisements_active = packet[3];
    }
    if (IS_COMMAND(packet, hci_le_create_connection)){
        // white list used?
        uint8_t initiator_filter_policy = packet[7];
        switch (initiator_filter_policy){
            case 0:
                // whitelist not used
                hci_stack->le_connecting_state = LE_CONNECTING_DIRECT;
                break;
            case 1:
                hci_stack->le_connecting_state = LE_CONNECTING_WHITELIST;
                break;
            default:
                log_error("Invalid initiator_filter_policy in LE Create Connection %u", initiator_filter_policy);
                break;
        }
    }
    if (IS_COMMAND(packet, hci_le_create_connection_cancel)){
        hci_stack->le_connecting_state = LE_CONNECTING_IDLE;
    }

    hci_stack->num_cmd_packets--;

    int err = hci_stack->hci_transport->send_packet(HCI_COMMAND_DATA_PACKET, packet, size);

    // release packet buffer for synchronous transport implementations    
    if (hci_transport_synchronous() && (packet == hci_stack->hci_packet_buffer)){
        hci_stack->hci_packet_buffer_reserved = 0;
    }

    return err;
}

#if 0
// disconnect because of security block
void hci_disconnect_security_block(hci_con_handle_t con_handle){
    hci_connection_t * connection = le_hci_connection_for_handle(con_handle);
    if (!connection) return;
    connection->bonding_flags |= BONDING_DISCONNECT_SECURITY_BLOCK;
}
#endif

/**
 * pre: numcmds >= 0 - it's allowed to send a command to the controller
 */
int le_hci_send_cmd(const hci_cmd_t *cmd, ...)
{
    if (!le_hci_can_send_command_packet_now()){ 
        hci_deg("hci_send_cmd called but cannot send packet now");
        return 0;
    }

    // for HCI INITIALIZATION
    // log_info("hci_send_cmd: opcode %04x", cmd->opcode);
    /* printf("hci_send_cmd: ogf %04x- ocf %04x\n", cmd->opcode>>10, cmd->opcode & 0x3ff); */
    
    hci_stack->last_cmd_opcode = cmd->opcode;

    le_hci_reserve_packet_buffer();
    uint8_t * packet = hci_stack->hci_packet_buffer;

    va_list argptr;
    va_start(argptr, cmd);
    uint16_t size = le_hci_create_cmd_internal(packet, cmd, argptr);
    va_end(argptr);

    return le_hci_send_cmd_packet(packet, size);
}

// Create various non-HCI events. 
// TODO: generalize, use table similar to hci_create_command
static void hci_emit_state()
{
    log_info("BTSTACK_EVENT_STATE %u", hci_stack->state);
    hci_deg("BTSTACK_EVENT_STATE %x", hci_stack->state);
    uint8_t event[3];
    event[0] = BTSTACK_EVENT_STATE;
    event[1] = sizeof(event) - 2;
    event[2] = hci_stack->state;
    hci_stack->packet_handler(HCI_EVENT_PACKET, event, sizeof(event));
}


void hci_emit_le_connection_complete(uint8_t address_type, bd_addr_t address, uint16_t conn_handle, uint8_t status){
    uint8_t event[21];
    event[0] = HCI_EVENT_LE_META;
    event[1] = sizeof(event) - 2;
    event[2] = HCI_SUBEVENT_LE_CONNECTION_COMPLETE;
    event[3] = status;
    bt_store_16(event, 4, conn_handle);
    event[6] = 0; // TODO: role
    event[7] = address_type;
    bt_flip_addr(&event[8], address);
    bt_store_16(event, 14, 0); // interval
    bt_store_16(event, 16, 0); // latency
    bt_store_16(event, 18, 0); // supervision timeout
    event[20] = 0; // master clock accuracy
    hci_stack->packet_handler(HCI_EVENT_PACKET, event, sizeof(event));
}

void le_hci_emit_disconnection_complete(uint16_t handle, uint8_t reason){
    uint8_t event[6];
    event[0] = HCI_EVENT_DISCONNECTION_COMPLETE;
    event[1] = sizeof(event) - 2;
    event[2] = 0; // status = OK
    bt_store_16(event, 3, handle);
    event[5] = reason;
    hci_stack->packet_handler(HCI_EVENT_PACKET, event, sizeof(event));
}

void le_hci_emit_l2cap_check_timeout(hci_connection_t *conn){
    if (disable_l2cap_timeouts) return;
    log_info("L2CAP_EVENT_TIMEOUT_CHECK");
    uint8_t event[4];
    event[0] = L2CAP_EVENT_TIMEOUT_CHECK;
    event[1] = sizeof(event) - 2;
    bt_store_16(event, 2, conn->con_handle);
    hci_stack->packet_handler(HCI_EVENT_PACKET, event, sizeof(event));
}

void le_hci_emit_nr_connections_changed(){
    log_info("BTSTACK_EVENT_NR_CONNECTIONS_CHANGED %u", nr_hci_connections());
    /* hci_deg("BTSTACK_EVENT_NR_CONNECTIONS_CHANGED %x", nr_hci_connections()); */
    uint8_t event[3];
    event[0] = BTSTACK_EVENT_NR_CONNECTIONS_CHANGED;
    event[1] = sizeof(event) - 2;
    event[2] = nr_hci_connections();
    hci_stack->packet_handler(HCI_EVENT_PACKET, event, sizeof(event));
}

void le_hci_emit_hci_open_failed(){
    log_info("BTSTACK_EVENT_POWERON_FAILED");
    uint8_t event[2];
    event[0] = BTSTACK_EVENT_POWERON_FAILED;
    event[1] = sizeof(event) - 2;
    hci_stack->packet_handler(HCI_EVENT_PACKET, event, sizeof(event));
}

#if 0

void hci_emit_remote_name_cached(bd_addr_t addr, device_name_t *name){
    uint8_t event[2+1+6+248+1]; // +1 for \0 in log_info
    event[0] = BTSTACK_EVENT_REMOTE_NAME_CACHED;
    event[1] = sizeof(event) - 2 - 1;
    event[2] = 0;   // just to be compatible with HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE
    bt_flip_addr(&event[3], addr);
    memcpy(&event[9], name, 248);
    
    event[9+248] = 0;   // assert \0 for log_info
    log_info("BTSTACK_EVENT_REMOTE_NAME_CACHED %s = '%s'", bd_addr_to_str(addr), &event[9]);

    hci_stack->packet_handler(HCI_EVENT_PACKET, event, sizeof(event)-1);
}
#endif


void hci_emit_security_level(hci_con_handle_t con_handle, gap_security_level_t level){
    log_info("hci_emit_security_level %u for handle %x", level, con_handle);
    uint8_t event[5];
    int pos = 0;
    event[pos++] = GAP_SECURITY_LEVEL;
    event[pos++] = sizeof(event) - 2;
    bt_store_16(event, 2, con_handle);
    pos += 2;
    event[pos++] = level;
    hci_stack->packet_handler(HCI_EVENT_PACKET, event, sizeof(event));
}

#if 0
void hci_emit_dedicated_bonding_result(bd_addr_t address, uint8_t status){
    log_info("hci_emit_dedicated_bonding_result %u ", status);
    uint8_t event[9];
    int pos = 0;
    event[pos++] = GAP_DEDICATED_BONDING_COMPLETED;
    event[pos++] = sizeof(event) - 2;
    event[pos++] = status;
    bt_flip_addr( &event[pos], address);
    pos += 6;
    hci_stack->packet_handler(HCI_EVENT_PACKET, event, sizeof(event));
}
#endif


// GAP API
/**
 * @bbrief enable/disable bonding. default is enabled
 * @praram enabled
 */
void gap_set_bondable_mode(int enable){
    hci_stack->bondable = enable ? 1 : 0;
}

/**
 * @brief map link keys to security levels
 */
gap_security_level_t gap_security_level_for_link_key_type(link_key_type_t link_key_type){
    switch (link_key_type){
        case AUTHENTICATED_COMBINATION_KEY_GENERATED_FROM_P256:
            return LEVEL_4;
        case COMBINATION_KEY:
        case AUTHENTICATED_COMBINATION_KEY_GENERATED_FROM_P192:
            return LEVEL_3;
        default:
            return LEVEL_2;
    }
}

static gap_security_level_t gap_security_level_for_connection(hci_connection_t * connection){
    if (!connection) return LEVEL_0;
    if ((connection->authentication_flags & CONNECTION_ENCRYPTED) == 0) return LEVEL_0;
    return gap_security_level_for_link_key_type(connection->link_key_type);
}    


int gap_mitm_protection_required_for_security_level(gap_security_level_t level){
    return level > LEVEL_2;
}

/**
 * @brief get current security level
 */
gap_security_level_t gap_security_level(hci_con_handle_t con_handle){
    hci_connection_t * connection = le_hci_connection_for_handle(con_handle);
    if (!connection) return LEVEL_0;
    return gap_security_level_for_connection(connection);
}

/**
 * @brief request connection to device to
 * @result GAP_AUTHENTICATION_RESULT
 */
void gap_request_security_level(hci_con_handle_t con_handle, gap_security_level_t requested_level)
{
    hci_connection_t * connection = le_hci_connection_for_handle(con_handle);
    if (!connection){
        hci_emit_security_level(con_handle, LEVEL_0);
        return;
    }
    gap_security_level_t current_level = gap_security_level(con_handle);
    log_info("gap_request_security_level %u, current level %u", requested_level, current_level);
    if (current_level >= requested_level){
        hci_emit_security_level(con_handle, current_level);
        return;
    }

    connection->requested_security_level = requested_level;

    // sending encryption request without a link key results in an error. 
    // TODO: figure out how to use it properly

    // would enabling ecnryption suffice (>= LEVEL_2)?
    if (hci_stack->remote_device_db){
        link_key_type_t link_key_type;
        link_key_t      link_key;
        if (hci_stack->remote_device_db->get_link_key( &connection->address, &link_key, &link_key_type)){
            if (gap_security_level_for_link_key_type(link_key_type) >= requested_level){
                connection->bonding_flags |= BONDING_SEND_ENCRYPTION_REQUEST;
                return;
            }
        }
    }

    // try to authenticate connection
    connection->bonding_flags |= BONDING_SEND_AUTHENTICATE_REQUEST;
    le_hci_run();
}

/**
 * @brief start dedicated bonding with device. disconnect after bonding
 * @param device
 * @param request MITM protection
 * @result GAP_DEDICATED_BONDING_COMPLETE
 */
int gap_dedicated_bonding(bd_addr_t device, int mitm_protection_required){

    // create connection state machine
    hci_connection_t * connection = create_connection_for_bd_addr_and_type(device, BD_ADDR_TYPE_CLASSIC);

    if (!connection){
        return BTSTACK_MEMORY_ALLOC_FAILED;
    }

    // delete linkn key
    hci_drop_link_key_for_bd_addr(device);

    // configure LEVEL_2/3, dedicated bonding
    connection->state = SEND_CREATE_CONNECTION;    
    connection->requested_security_level = mitm_protection_required ? LEVEL_3 : LEVEL_2;
    log_info("gap_dedicated_bonding, mitm %u -> level %u", mitm_protection_required, connection->requested_security_level);
    connection->bonding_flags = BONDING_DEDICATED;

    // wait for GAP Security Result and send GAP Dedicated Bonding complete

    // handle: connnection failure (connection complete != ok)
    // handle: authentication failure
    // handle: disconnect on done

    le_hci_run();

    return 0;
}

void gap_set_local_name(const char * local_name){
    hci_stack->local_name = local_name;
}

le_command_status_t le_central_start_scan(){
    if (hci_stack->le_scanning_state == LE_SCANNING) return BLE_PERIPHERAL_OK;
    hci_stack->le_scanning_state = LE_START_SCAN;
    le_hci_run();
    return BLE_PERIPHERAL_OK;
}

le_command_status_t le_central_stop_scan(){
    if ( hci_stack->le_scanning_state == LE_SCAN_IDLE) return BLE_PERIPHERAL_OK;
    hci_stack->le_scanning_state = LE_STOP_SCAN;
    le_hci_run();
    return BLE_PERIPHERAL_OK;
}

void le_central_set_scan_parameters(uint8_t scan_type, uint16_t scan_interval, uint16_t scan_window){
    hci_stack->le_scan_type     = scan_type;
    hci_stack->le_scan_interval = scan_interval;
    hci_stack->le_scan_window   = scan_window;
    le_hci_run();
}

le_command_status_t le_central_connect(bd_addr_t  addr, bd_addr_type_t addr_type)
{
    hci_connection_t * conn = le_hci_connection_for_bd_addr_and_type(addr, addr_type);

    if (!conn){
        conn = create_connection_for_bd_addr_and_type(addr, addr_type);
        if (!conn){
            // notify client that alloc failed
            hci_emit_le_connection_complete(addr_type, addr, 0, BTSTACK_MEMORY_ALLOC_FAILED);
            return BLE_PERIPHERAL_NOT_CONNECTED; // don't sent packet to controller
        }
        conn->state = SEND_CREATE_CONNECTION;
        le_hci_run();
        return BLE_PERIPHERAL_OK;
    }
    
    if (conn->state == SEND_CREATE_CONNECTION ||
        conn->state == SENT_CREATE_CONNECTION) {
        hci_emit_le_connection_complete(conn->address_type, conn->address, 0, ERROR_CODE_COMMAND_DISALLOWED);
        return BLE_PERIPHERAL_IN_WRONG_STATE;
    }
    
    hci_emit_le_connection_complete(conn->address_type, conn->address, conn->con_handle, 0);
    le_hci_run();

    return BLE_PERIPHERAL_OK;
}

// @assumption: only a single outgoing LE Connection exists
static hci_connection_t * le_central_get_outgoing_connection()
{
    linked_item_t *it;

    for (it = (linked_item_t *) hci_stack->connections; it ; it = it->next){
        hci_connection_t * conn = (hci_connection_t *) it;
        /*if (!hci_is_le_connection(conn)) continue;*/
        switch (conn->state){
            case SEND_CREATE_CONNECTION:
            case SENT_CREATE_CONNECTION:
                return conn;
            default:
                break;
        }
    }
    return NULL;
}

le_command_status_t le_central_connect_cancel()
{
    hci_connection_t * conn = le_central_get_outgoing_connection();
    switch (conn->state){
        case SEND_CREATE_CONNECTION:
            // skip sending create connection and emit event instead
            hci_emit_le_connection_complete(conn->address_type, conn->address, 0, ERROR_CODE_UNKNOWN_CONNECTION_IDENTIFIER);
            linked_list_remove(&hci_stack->connections, (linked_item_t *) conn);
            le_btstack_memory_hci_connection_free( conn );
            break;            
        case SENT_CREATE_CONNECTION:
            // request to send cancel connection
            conn->state = SEND_CANCEL_CONNECTION;
            le_hci_run();
            break;
        default:
            break;
    }
    return BLE_PERIPHERAL_OK;
}

/**
 * @brief Updates the connection parameters for a given LE connection
 * @param handle
 * @param conn_interval_min (unit: 1.25ms)
 * @param conn_interval_max (unit: 1.25ms)
 * @param conn_latency
 * @param supervision_timeout (unit: 10ms)
 * @returns 0 if ok
 */
int gap_update_connection_parameters(hci_con_handle_t con_handle, uint16_t conn_interval_min,
    uint16_t conn_interval_max, uint16_t conn_latency, uint16_t supervision_timeout){
    hci_connection_t * connection = le_hci_connection_for_handle(con_handle);
    if (!connection) return ERROR_CODE_UNKNOWN_CONNECTION_IDENTIFIER;
    connection->le_conn_interval_min = conn_interval_min;
    connection->le_conn_interval_max = conn_interval_max;
    connection->le_conn_latency = conn_latency;
    connection->le_supervision_timeout = supervision_timeout;
    connection->le_con_parameter_update_state = CON_PARAMETER_UPDATE_CHANGE_HCI_CON_PARAMETERS;
    le_hci_run();
    return 0;
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
    hci_connection_t * connection = le_hci_connection_for_handle(con_handle);
    if (!connection) return ERROR_CODE_UNKNOWN_CONNECTION_IDENTIFIER;
    connection->le_conn_interval_min = conn_interval_min;
    connection->le_conn_interval_max = conn_interval_max;
    connection->le_conn_latency = conn_latency;
    connection->le_supervision_timeout = supervision_timeout;
    connection->le_con_parameter_update_state = CON_PARAMETER_UPDATE_SEND_REQUEST;
    le_hci_run();
    return 0;
}

/**
 * @brief Set Advertisement Data
 * @param advertising_data_length
 * @param advertising_data (max 31 octets)
 * @note data is not copied, pointer has to stay valid
 */
void gap_advertisements_set_data(uint8_t advertising_data_length, uint8_t * advertising_data){
    hci_stack->le_advertisements_data_len = advertising_data_length;
    hci_stack->le_advertisements_data = advertising_data;
    hci_stack->le_advertisements_todo |= LE_ADVERTISEMENT_TASKS_SET_DATA;
    // disable advertisements before setting data
    if (hci_stack->le_advertisements_active){
        hci_stack->le_advertisements_todo |= LE_ADVERTISEMENT_TASKS_DISABLE | LE_ADVERTISEMENT_TASKS_ENABLE;
    }
    le_hci_run();
}

/**
 * @brief Set Advertisement Parameters
 * @param adv_int_min
 * @param adv_int_max
 * @param adv_type
 * @param own_address_type
 * @param direct_address_type
 * @param direct_address
 * @param channel_map
 * @param filter_policy
 *
 * @note internal use. use gap_advertisements_set_params from gap_le.h instead.
 */
 void hci_le_advertisements_set_params(uint16_t adv_int_min, uint16_t adv_int_max, uint8_t adv_type,
    uint8_t own_address_type, uint8_t direct_address_typ, bd_addr_t direct_address,
    uint8_t channel_map, uint8_t filter_policy) {

    hci_stack->le_advertisements_interval_min = adv_int_min;
    hci_stack->le_advertisements_interval_max = adv_int_max;
    hci_stack->le_advertisements_type = adv_type;
    hci_stack->le_advertisements_own_address_type = own_address_type;
    hci_stack->le_advertisements_direct_address_type = direct_address_typ;
    hci_stack->le_advertisements_channel_map = channel_map;
    hci_stack->le_advertisements_filter_policy = filter_policy;
    memcpy(hci_stack->le_advertisements_direct_address, direct_address, 6);

    hci_stack->le_advertisements_todo |= LE_ADVERTISEMENT_TASKS_SET_PARAMS;
    // disable advertisements before changing params
    if (hci_stack->le_advertisements_active){
        hci_stack->le_advertisements_todo |= LE_ADVERTISEMENT_TASKS_DISABLE | LE_ADVERTISEMENT_TASKS_ENABLE;
    }
    le_hci_run();
 }

/**
 * @brief Enable/Disable Advertisements
 * @param enabled
 */
void gap_advertisements_enable(int enabled){
    hci_stack->le_advertisements_enabled = enabled;
    if (enabled && !hci_stack->le_advertisements_active){
        hci_stack->le_advertisements_todo |= LE_ADVERTISEMENT_TASKS_ENABLE;
    }
    if (!enabled && hci_stack->le_advertisements_active){
        hci_stack->le_advertisements_todo |= LE_ADVERTISEMENT_TASKS_DISABLE;
    }
    le_hci_run();
}


le_command_status_t gap_disconnect(hci_con_handle_t handle){
    hci_connection_t * conn = le_hci_connection_for_handle(handle);
    if (!conn){
        le_hci_emit_disconnection_complete(handle, 0);
        return BLE_PERIPHERAL_OK;
    }
    conn->state = SEND_DISCONNECT;
    le_hci_run();
    return BLE_PERIPHERAL_OK;
}

#ifdef HAVE_BLE

/**
 * @brief Auto Connection Establishment - Start Connecting to device
 * @param address_typ
 * @param address
 * @returns 0 if ok
 */
int gap_auto_connection_start(bd_addr_type_t address_type, bd_addr_t address){
    // check capacity
    int num_entries = linked_list_count(&hci_stack->le_whitelist);
    if (num_entries >= hci_stack->le_whitelist_capacity) return ERROR_CODE_MEMORY_CAPACITY_EXCEEDED;
    whitelist_entry_t * entry = btstack_memory_whitelist_entry_get();
    if (!entry) return BTSTACK_MEMORY_ALLOC_FAILED;
    entry->address_type = address_type;
    memcpy(entry->address, address, 6);
    entry->state = LE_WHITELIST_ADD_TO_CONTROLLER;
    linked_list_add(&hci_stack->le_whitelist, (linked_item_t*) entry);
    le_hci_run();
    return 0;
}

static void hci_remove_from_whitelist(bd_addr_type_t address_type, bd_addr_t address){
    linked_list_iterator_t it;
    linked_list_iterator_init(&it, &hci_stack->le_whitelist);
    while (linked_list_iterator_has_next(&it)){
        whitelist_entry_t * entry = (whitelist_entry_t*) linked_list_iterator_next(&it);
        if (entry->address_type != address_type) continue;
        if (memcmp(entry->address, address, 6) != 0) continue;
        if (entry->state & LE_WHITELIST_ON_CONTROLLER){
            // remove from controller if already present
            entry->state |= LE_WHITELIST_REMOVE_FROM_CONTROLLER;
            continue;
        }
        // direclty remove entry from whitelist
        linked_list_iterator_remove(&it);
        btstack_memory_whitelist_entry_free(entry);
    }
}

/**
 * @brief Auto Connection Establishment - Stop Connecting to device
 * @param address_typ
 * @param address
 * @returns 0 if ok
 */
int gap_auto_connection_stop(bd_addr_type_t address_type, bd_addr_t address){
    hci_remove_from_whitelist(address_type, address);
    le_hci_run();
    return 0;
}

/**
 * @brief Auto Connection Establishment - Stop everything
 * @note  Convenience function to stop all active auto connection attempts
 */
void gap_auto_connection_stop_all(void){
    linked_list_iterator_t it;
    linked_list_iterator_init(&it, &hci_stack->le_whitelist);
    while (linked_list_iterator_has_next(&it)){
        whitelist_entry_t * entry = (whitelist_entry_t*) linked_list_iterator_next(&it);
        if (entry->state & LE_WHITELIST_ON_CONTROLLER){
            // remove from controller if already present
            entry->state |= LE_WHITELIST_REMOVE_FROM_CONTROLLER;
            continue;
        }
        // directly remove entry from whitelist
        linked_list_iterator_remove(&it);
        btstack_memory_whitelist_entry_free(entry);
    }
    le_hci_run();
}

#endif

void hci_disconnect_all(){
    linked_list_iterator_t it;
    linked_list_iterator_init(&it, &hci_stack->connections);
    while (linked_list_iterator_has_next(&it)){
        hci_connection_t * con = (hci_connection_t*) linked_list_iterator_next(&it);
        if (con->state == SENT_DISCONNECT) continue;
        con->state = SEND_DISCONNECT;
    }
    le_hci_run();
}
