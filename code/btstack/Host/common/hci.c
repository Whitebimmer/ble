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

#include "btstack-config.h"

#include "hci.h"
#include "gap.h"

#include <stdarg.h>
#include <string.h>
#include <stdio.h>


#include "ble/btstack_memory.h"
#include "ble/debug.h"
#include "ble/hci_dump.h"

#include <linked_list.h>
#include <hci_cmds.h>
#include "bt_memory.h"
#include "btstack_event.h"

/************************HCI DEBUG CONTROL**************************/
/* #define HCI_DEBUG */

#ifdef HCI_DEBUG
#define hci_puts        puts
#define hci_deg         printf
#define hci_pbuf(x,y)   printf_buf(x,y)

#else
#define hci_puts(...)     
#define hci_deg(...)      
#define hci_pbuf(...)

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
static void hci_run(void);
#ifdef ENABLE_BLE
// called from test/ble_client/advertising_data_parser.c
void le_handle_advertisement_report(uint8_t *packet, int size);
static void hci_remove_from_whitelist(bd_addr_type_t address_type, bd_addr_t address);
#endif

static void hci_emit_event(uint8_t * event, uint16_t size, int dump);

// the STACK is here
#ifndef HAVE_MALLOC
static hci_stack_t   hci_stack_static;
#endif
static hci_stack_t * hci_stack SEC(.btmem_highly_available) = NULL;

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

void hci_connections_get_iterator(linked_list_iterator_t *it){
    linked_list_iterator_init(it, &hci_stack->connections);
}

/**
 * get connection for a given handle
 *
 * @return connection OR NULL, if not found
 */
hci_connection_t * hci_connection_for_handle(hci_con_handle_t con_handle)
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

int  hci_authentication_active_for_handle(hci_con_handle_t handle){
    hci_connection_t * conn = hci_connection_for_handle(handle);
    if (!conn) return 0;
    if (conn->authentication_flags & LEGACY_PAIRING_ACTIVE) return 1;
    if (conn->authentication_flags & SSP_PAIRING_ACTIVE) return 1;
    return 0;
}

static void hci_drop_link_key_for_bd_addr(bd_addr_t addr)
{
    if (hci_stack->remote_device_db) {
        hci_stack->remote_device_db->delete_link_key(addr);
    }
}

int hci_is_le_connection(hci_connection_t * connection){
    return  connection->address_type == BD_ADDR_TYPE_LE_PUBLIC ||
    connection->address_type == BD_ADDR_TYPE_LE_RANDOM;
}


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

static int hci_number_free_acl_slots_for_connection_type(bd_addr_type_t address_type){
    
    int num_packets_sent_classic = 0;
    int num_packets_sent_le = 0;

    linked_item_t *it;
    for (it = (linked_item_t *) hci_stack->connections; it ; it = it->next){
        hci_connection_t * connection = (hci_connection_t *) it;
        if (connection->address_type == BD_ADDR_TYPE_CLASSIC){
            num_packets_sent_classic += connection->num_acl_packets_sent;
        } else {
            num_packets_sent_le += connection->num_acl_packets_sent;
        }
    }
    log_debug("ACL classic buffers: %u used of %u", num_packets_sent_classic, hci_stack->acl_packets_total_num);
    int free_slots_classic = hci_stack->acl_packets_total_num - num_packets_sent_classic;
    int free_slots_le = 0;

    if (free_slots_classic < 0){
        log_error("hci_number_free_acl_slots: outgoing classic packets (%u) > total classic packets (%u)", num_packets_sent_classic, hci_stack->acl_packets_total_num);
        return 0;
    }

    if (hci_stack->le_acl_packets_total_num){
        // if we have LE slots, they are used
        free_slots_le = hci_stack->le_acl_packets_total_num - num_packets_sent_le;
        if (free_slots_le < 0){
            log_error("hci_number_free_acl_slots: outgoing le packets (%u) > total le packets (%u)", num_packets_sent_le, hci_stack->le_acl_packets_total_num);
            return 0;
        }
    } else {
        // otherwise, classic slots are used for LE, too
        free_slots_classic -= num_packets_sent_le;
        if (free_slots_classic < 0){
            log_error("hci_number_free_acl_slots: outgoing classic + le packets (%u + %u) > total packets (%u)", num_packets_sent_classic, num_packets_sent_le, hci_stack->acl_packets_total_num);
            return 0;
        }
    }

    switch (address_type){
        case BD_ADDR_TYPE_UNKNOWN:
            log_error("hci_number_free_acl_slots: unknown address type");
            return 0;

        case BD_ADDR_TYPE_CLASSIC:
            return free_slots_classic;

        default:
           if (hci_stack->le_acl_packets_total_num){
               return free_slots_le;
           }
           return free_slots_classic; 
    }
}

int hci_number_free_acl_slots_for_handle(hci_con_handle_t con_handle){
    // get connection type
    hci_connection_t * connection = hci_connection_for_handle(con_handle);
    if (!connection){
        log_error("hci_number_free_acl_slots: handle 0x%04x not in connection list", con_handle);
        return 0;
    }
    return hci_number_free_acl_slots_for_connection_type(connection->address_type);
}

static int hci_number_free_sco_slots(void){
    int num_sco_packets_sent = 0;
    linked_item_t *it;
    for (it = (linked_item_t *) hci_stack->connections; it ; it = it->next){
        hci_connection_t * connection = (hci_connection_t *) it;
        num_sco_packets_sent += connection->num_sco_packets_sent;
    }
    if (num_sco_packets_sent > hci_stack->sco_packets_total_num){
        log_info("hci_number_free_sco_slots:packets (%u) > total packets (%u)", num_sco_packets_sent, hci_stack->sco_packets_total_num);
        return 0;
    }
    // log_info("hci_number_free_sco_slots u", handle, num_sco_packets_sent);
    return hci_stack->sco_packets_total_num - num_sco_packets_sent;
}
// new functions replacing hci_can_send_packet_now[_using_packet_buffer]
int hci_can_send_command_packet_now(void){
	//TODO
	/* return 1; */
    if (hci_stack->hci_packet_buffer_reserved) {
        hci_puts("error hci_packet_buffer_reserved\n");
        return 0;   
    }

    // check for async hci transport implementations
    if (hci_stack->hci_transport->can_send_packet_now){
        if (!hci_stack->hci_transport->can_send_packet_now(HCI_COMMAND_DATA_PACKET)){
            hci_puts("error can_send_packet_now\n");
            return 0;
        }
    }

    return hci_stack->num_cmd_packets > 0;
}

static int hci_transport_can_send_prepared_packet_now(uint8_t packet_type){
    // check for async hci transport implementations
    if (!hci_stack->hci_transport->can_send_packet_now) return 1;
    return hci_stack->hci_transport->can_send_packet_now(packet_type);
}

static int hci_can_send_prepared_acl_packet_for_address_type(bd_addr_type_t address_type){
    if (!hci_transport_can_send_prepared_packet_now(HCI_ACL_DATA_PACKET)) return 0;
    return hci_number_free_acl_slots_for_connection_type(address_type) > 0;
}

int hci_can_send_acl_classic_packet_now(void){
    if (hci_stack->hci_packet_buffer_reserved) return 0;
    return hci_can_send_prepared_acl_packet_for_address_type(BD_ADDR_TYPE_CLASSIC);
}

int hci_can_send_acl_le_packet_now(void){
    if (hci_stack->hci_packet_buffer_reserved) return 0;
    return hci_can_send_prepared_acl_packet_for_address_type(BD_ADDR_TYPE_LE_PUBLIC);
}

int hci_can_send_prepared_acl_packet_now(hci_con_handle_t con_handle) {
    if (!hci_transport_can_send_prepared_packet_now(HCI_ACL_DATA_PACKET)) return 0;
    return hci_number_free_acl_slots_for_handle(con_handle) > 0;
}

int hci_can_send_acl_packet_now(hci_con_handle_t con_handle){
    if (hci_stack->hci_packet_buffer_reserved) return 0;
    return hci_can_send_prepared_acl_packet_now(con_handle);
}

int hci_can_send_prepared_sco_packet_now(void){
    if (!hci_transport_can_send_prepared_packet_now(HCI_SCO_DATA_PACKET)) return 0;
    if (!hci_stack->synchronous_flow_control_enabled) return 1;
    return hci_number_free_sco_slots() > 0;    
}

int hci_can_send_sco_packet_now(void){
    if (hci_stack->hci_packet_buffer_reserved) return 0;
    return hci_can_send_prepared_sco_packet_now();
}

void hci_request_sco_can_send_now_event(void){
    hci_stack->sco_waiting_for_can_send_now = 1;
    hci_notify_if_sco_can_send_now();
}

// used for internal checks in l2cap[-le].c
int hci_is_packet_buffer_reserved(void){
    return hci_stack->hci_packet_buffer_reserved;
}

// reserves outgoing packet buffer. @returns 1 if successful
int hci_reserve_packet_buffer(void){
    if (hci_stack->hci_packet_buffer_reserved) {
        log_error("hci_reserve_packet_buffer called but buffer already reserved");
        return 0;
    }
    hci_stack->hci_packet_buffer_reserved = 1;
    return 1;    
}

void hci_release_packet_buffer(void){
    hci_stack->hci_packet_buffer_reserved = 0;
}

// assumption: synchronous implementations don't provide can_send_packet_now as they don't keep the buffer after the call
static int hci_transport_synchronous(void){
    return hci_stack->hci_transport->can_send_packet_now == NULL;
}


static int hci_send_acl_packet_fragments(hci_connection_t *connection)
{
    // log_info("hci_send_acl_packet_fragments  %u/%u (con 0x%04x)", hci_stack->acl_fragmentation_pos, hci_stack->acl_fragmentation_total_size, connection->con_handle);

    // max ACL data packet length depends on connection type (LE vs. Classic) and available buffers
    uint16_t max_acl_data_packet_length = hci_stack->acl_data_packet_length;
    if (/*hci_is_le_connection(connection) && */hci_stack->le_data_packets_length > 0){
        max_acl_data_packet_length = hci_stack->le_data_packets_length;
        /* hci_deg("max_acl_data_packet_length use le: %x - %x\n", hci_stack->le_data_packets_length, max_acl_data_packet_length ); */
    }
    /* hci_deg("max_acl_data_packet_length : %x - %x\n", hci_stack->le_data_packets_length, max_acl_data_packet_length ); */
	/* hci_puts("hci_send_acl\n"); */

    // testing: reduce buffer to minimum
    // max_acl_data_packet_length = 52;

    int err;
    // multiple packets could be send on a synchronous HCI transport
    while (1){

        // get current data
        const uint16_t acl_header_pos = hci_stack->acl_fragmentation_pos - 4;
        int current_acl_data_packet_length = hci_stack->acl_fragmentation_total_size - hci_stack->acl_fragmentation_pos;
        int more_fragments = 0;

		current_acl_data_packet_length -= 4;

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
        const int size = current_acl_data_packet_length + 4;
        bt_store_16(hci_stack->hci_packet_buffer, acl_header_pos + 2, size);

        // count packet
        connection->num_acl_packets_sent++;

        // send packet
        uint8_t * packet = &hci_stack->hci_packet_buffer[acl_header_pos];
        /* _printf_buf(packet, 0x8); */
        err = hci_stack->hci_transport->send_packet(HCI_ACL_DATA_PACKET, packet, size);

        /* hci_deg("pos %x - size 0x%x\n", acl_header_pos, size); */
        hci_puts("acl more ...");
        // done yet?
        if (!more_fragments) break;

        // update start of next fragment to send
        hci_stack->acl_fragmentation_pos += current_acl_data_packet_length;
        hci_stack->acl_fragmentation_pos += 4;

        // can send more?
        if (!hci_can_send_prepared_acl_packet_now(connection->con_handle))
		   	return err;
    }

    hci_puts("done!\n");
    // done    
    hci_stack->acl_fragmentation_pos = 0;
    hci_stack->acl_fragmentation_total_size = 0;

    // release buffer now for synchronous transport
    if (hci_transport_synchronous()){
        hci_release_packet_buffer();
        // notify upper stack that iit might be possible to send again
        uint8_t event[] = { DAEMON_EVENT_HCI_PACKET_SENT, 0};
        /* hci_stack->packet_handler(HCI_EVENT_PACKET, &event[0], sizeof(event)); */
        hci_emit_event(&event[0], sizeof(event), 0);
    }
	/* hci_puts("exit2\n"); */

    return err;
}

// pre: caller has reserved the packet buffer
int hci_send_acl_packet_buffer(int size){
	/* hci_puts("le_send_acl\n"); */

    // log_info("hci_send_acl_packet_buffer size %u", size);

    if (!hci_stack->hci_packet_buffer_reserved) {
        return 0;
    }

    uint8_t * packet = hci_stack->hci_packet_buffer;
    hci_con_handle_t con_handle = READ_ACL_CONNECTION_HANDLE(packet);

    // check for free places on Bluetooth module
    if (!hci_can_send_prepared_acl_packet_now(con_handle)) {
        hci_release_packet_buffer();
        return BTSTACK_ACL_BUFFERS_FULL;
    }

    hci_connection_t *connection = hci_connection_for_handle( con_handle);
    if (!connection) {
        hci_release_packet_buffer();
        return 0;
    }
    hci_connection_timestamp(connection);
    

    // setup data
    hci_stack->acl_fragmentation_total_size = size;
    hci_stack->acl_fragmentation_pos = 4;   // start of L2CAP packet

    return hci_send_acl_packet_fragments(connection);
}

static void hci_emit_acl_packet(uint8_t * packet, uint16_t size){
    if (!hci_stack->acl_packet_handler) return;
    hci_stack->acl_packet_handler(HCI_ACL_DATA_PACKET, 0, packet, size);
}

static void acl_handler(uint8_t *packet, int size)
{
	hci_con_handle_t con_handle = READ_ACL_CONNECTION_HANDLE(packet);
	hci_connection_t *conn      = hci_connection_for_handle(con_handle);
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
            hci_deg("acl pos : %x / acl length : %x\n",conn->acl_recombination_pos, conn->acl_recombination_length);
			if (conn->acl_recombination_pos >= conn->acl_recombination_length + 4 + 4){ // pos already incl. ACL header

				/* hci_stack->packet_handler(HCI_ACL_DATA_PACKET, &conn->acl_recombination_buffer[HCI_INCOMING_PRE_BUFFER_SIZE], conn->acl_recombination_pos); */
                hci_emit_acl_packet(&conn->acl_recombination_buffer[HCI_INCOMING_PRE_BUFFER_SIZE], conn->acl_recombination_pos);
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
                hci_deg( "ACL First Fragment: acl_len %x, l2cap_len %x", acl_length, l2cap_length);
                hci_deg("acl pos : %x\n", conn->acl_recombination_pos);

				// compare fragment size to L2CAP packet size
				if (acl_length >= l2cap_length + 4){

					// forward fragment as L2CAP packet
					/* hci_stack->packet_handler(HCI_ACL_DATA_PACKET, packet, acl_length + 4); */
                    hci_emit_acl_packet(packet, acl_length + 4);
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
	hci_run();
}

static void hci_shutdown_connection(hci_connection_t *conn){
    
    /* printf_buf(0x0, 0x10); */
    /* hci_deg("conn->timeout %x\n", &conn->timeout); */
    /* hci_deg("conn->timeout.entry %x\n", &conn->timeout.entry); */
    //bug fix : no match register
    /* sys_timer_remove(&conn->timeout); */
    
    /* printf_buf(0x0, 0x10); */
    linked_list_remove(&hci_stack->connections, (linked_item_t *) conn);
    le_btstack_memory_hci_connection_free( conn );
    
    // now it's gone
    le_hci_emit_nr_connections_changed();
}

uint16_t hci_usable_acl_packet_types(void){
    return hci_stack->packet_types;
}

uint8_t* hci_get_outgoing_packet_buffer(void){
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
    /* hci_puts(__func__); */
    if (hci_stack->adv_addr_type){
        memcpy(addr, hci_stack->adv_address, 6);
        /* hci_puts("****random addr \n"); */
    } else 
    {
        memcpy(addr, hci_stack->local_bd_addr, 6);
        /* hci_puts("****public\n"); */
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
        /* hci_stack->packet_handler(HCI_EVENT_PACKET, event, pos); */
        hci_emit_event(event, pos, 0);
    }
}


static void hci_initializing_next_state(){
    hci_stack->substate = (hci_substate_t )( ((int) hci_stack->substate) + 1);
	/* hci_puts("next_substate\n"); */
}

static const u8 adv_ind_data[] = {
	0x02, 0x01, 0x06,
	//0x05, 0x12, 0x80, 0x02, 0x80, 0x02,
	0x04, 0x0d, 0x00, 0x05, 0x10,
	0x03, 0x03, 0x0d, 0x18,
};


//len, type, Manufacturer Specific data
static const u8 scan_rsp_data[] = {
    0x02, 0x0A, 0x00,
#ifdef BT16
	0x05, 0x09, 'b','t', '1', '6',
#endif
#ifdef BR16
	0x09, 0x09, 'b','r', '1', '6', '-','4', '.', '1',
#endif
#ifdef BR17
	0x09, 0x09, 'b','r', '1', '7', '-','b', 'l', 'e',
#endif
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

/* static bd_addr_t tester_address = {0x00, 0xA0, 0x50, 0xB4, 0x51, 0x58}; */
static bd_addr_t tester_address = {0x54, 0x36, 0x98, 0xba, 0x3a, 0x2e};

// assumption: hci_can_send_command_packet_now() == true
static void hci_initializing_run()
{
    hci_puts("hci init run : ");
    switch (hci_stack->substate)
	{
        case HCI_INIT_SEND_RESET:
			hci_puts("HCI_INIT_SEND_RESET\n");
            hci_state_reset();
            /*-TODO-*/
            hci_stack->substate = HCI_INIT_W4_SEND_RESET;
            hci_send_cmd(&hci_reset);
            break;
		case HCI_INIT_READ_BD_ADDR:
			hci_puts("HCI_INIT_READ_BD_ADDR\n");
            hci_stack->substate = HCI_INIT_W4_READ_BD_ADDR;
			hci_send_cmd(&hci_read_bd_addr);
			break;
        // LE INIT
        case HCI_INIT_LE_READ_BUFFER_SIZE:
			hci_puts("HCI_INIT_LE_READ_BUFFER_SIZE\n");
            hci_stack->substate = HCI_INIT_W4_LE_READ_BUFFER_SIZE;
            hci_send_cmd(&hci_le_read_buffer_size);
            break;
        case HCI_INIT_WRITE_LE_HOST_SUPPORTED:
			hci_puts("HCI_INIT_WRITE_LE_HOST_SUPPORTED\n");
            // LE Supported Host = 1, Simultaneous Host = 0
            hci_stack->substate = HCI_INIT_W4_WRITE_LE_HOST_SUPPORTED;
            hci_send_cmd(&hci_write_le_host_supported, 1, 0);
            break;
        case HCI_INIT_READ_WHITE_LIST_SIZE:
			hci_puts("HCI_INIT_READ_WHITE_LIST_SIZE\n");
            hci_stack->substate = HCI_INIT_W4_READ_WHITE_LIST_SIZE;
            hci_send_cmd(&hci_le_read_white_list_size);
            break;
        case HCI_INIT_ADD_DEVICE_TO_WHITE_LIST:
            hci_puts("HCI_INIT_ADD_DEVICE_TO_WHITE_LIST\n");
            hci_stack->substate = HCI_INIT_W4_ADD_DEVICE_TO_WHITE_LIST;
            hci_send_cmd(&hci_le_add_device_to_white_list,0,&tester_address);
            break;

#if 0
        case HCI_INIT_LE_SET_ADV_PARAMETERS:
                hci_puts("HCI_INIT_LE_SET_ADV_PARAMETERS\n");
                hci_stack->substate = HCI_INIT_W4_LE_SET_ADV_PARAMETERS;
                hci_send_cmd(&hci_le_set_advertising_parameters,
                    0x0320, 0x0320, 
                    0x00, 0x00, 
                    0x0, NULL,
                    0x7, 0x0);
                    /* 0x0320, 0x0320,  */
                    /* 0x00, 0x00,  */
                    /* 0x0, rpa[0].peer_identity_address,  */
                    /* 0x7, 0x0); */
                break;
        case HCI_INIT_LE_SET_ADV_DATA:
            hci_puts("HCI_INIT_LE_SET_ADV_DATA\n");
            hci_stack->substate = HCI_INIT_W4_LE_SET_ADV_DATA;
            hci_send_cmd(&hci_le_set_advertising_data, sizeof(adv_ind_data), sizeof(adv_ind_data), adv_ind_data);
                break;
        case HCI_INIT_LE_SET_RSP_DATA:
            hci_puts("HCI_INIT_LE_SET_RSP_DATA\n");
            hci_stack->substate = HCI_INIT_W4_LE_SET_RSP_DATA;
            hci_send_cmd(&hci_le_set_scan_response_data, sizeof(scan_rsp_data), sizeof(scan_rsp_data), scan_rsp_data);
            break;
        //privacy 
        /* case HCI_INIT_LE_READ_RESOLVING_LIST_SIZE: */
            /* hci_puts("HCI_INIT_LE_READ_RESOLVING_LIST_SIZE\n"); */
            /* hci_stack->substate = HCI_INIT_W4_LE_READ_RESOLVING_LIST_SIZE; */
            /* hci_send_cmd(&hci_le_read_resolving_list_size); */
            /* break; */
        /* case HCI_INIT_LE_ADD_DEVICE_TO_RESOLVING_LIST: */
            /* hci_puts("HCI_INIT_LE_ADD_DEVICE_TO_RESOLVING_LIST\n"); */
            /* hci_stack->substate = HCI_INIT_W4_LE_ADD_DEVICE_TO_RESOLVING_LIST; */
            /* hci_send_cmd(&hci_le_add_device_to_resolving_list,  */
                    /* rpa[0].peer_identity_address_type, */
                    /* rpa[0].peer_identity_address, */
                    /* rpa[0].peer_irk, */
                    /* rpa[0].local_irk); */
            /* break; */
        /* case HCI_INIT_LE_SET_RANDOM_PRIVATE_ADDRESS_TIMEOUT: */
            /* hci_puts("HCI_INIT_LE_SET_RANDOM_PRIVATE_ADDRESS_TIMEOUT\n"); */
            /* hci_stack->substate = HCI_INIT_W4_LE_SET_RANDOM_PRIVATE_ADDRESS_TIMEOUT; */
            /* hci_send_cmd(&hci_le_set_resolvable_private_address_timeout, 0x384); */
            /* break; */
        /* case HCI_INIT_LE_ADDRESS_RESOLUTION_ENABLE: */
            /* hci_puts("HCI_INIT_LE_ADDRESS_RESOLUTION_ENABLE\n"); */
            /* hci_stack->substate = HCI_INIT_W4_LE_ADDRESS_RESOLUTION_ENABLE; */
            /* hci_send_cmd(&hci_le_set_address_resolution_enable, 1); */
            /* break; */

        case HCI_INIT_LE_SET_ADV_EN:
            hci_puts("HCI_INIT_LE_SET_ADV_EN\n");
            hci_stack->substate = HCI_INIT_W4_LE_SET_ADV_EN;
            hci_send_cmd(&hci_le_set_advertise_enable, 1);
            break;
#endif
        // DONE
        case HCI_INIT_DONE:
            // done.
			hci_puts("HCI_STATE_WORKING\n");
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
            log_info("Command complete for expected opcode %04x at substate %u", opcode, hci_stack->substate);
        } else {
            log_info("Command complete for different opcode %04x, expected %04x, at substate %u", opcode, hci_stack->last_cmd_opcode, hci_stack->substate);
        }
    }
    if (packet[0] == HCI_EVENT_COMMAND_STATUS){
        uint8_t  status = packet[2];
        uint16_t opcode = READ_BT_16(packet,4);
        if (opcode == hci_stack->last_cmd_opcode){
            if (status){
                command_completed = 1;
                log_error("Command status error 0x%02x for expected opcode %04x at substate %u", status, opcode, hci_stack->substate);
            } else {
                log_info("Command status OK for expected opcode %04x, waiting for command complete", opcode);
            }
        } else {
            log_info("Command status for opcode %04x, expected %04x", opcode, hci_stack->last_cmd_opcode);
        }
    }

    if (!command_completed) return;

    hci_initializing_next_state();
}

static void event_command_complete_handler(uint8_t *packet, int size)
{
    if (HCI_EVENT_IS_COMMAND_COMPLETE(packet, hci_read_buffer_size)){
        // from offset 5
        // status 
        // "The HC_ACL_Data_Packet_Length return parameter will be used to determine the size of the L2CAP segments contained in ACL Data Packets"
        hci_stack->acl_data_packet_length = READ_BT_16(packet, 6);
        hci_stack->sco_data_packet_length = packet[8];
        hci_stack->acl_packets_total_num  = READ_BT_16(packet, 9);
        hci_stack->sco_packets_total_num  = READ_BT_16(packet, 11); 

        if (hci_stack->state == HCI_STATE_INITIALIZING){
            // determine usable ACL payload size
            if (HCI_ACL_PAYLOAD_SIZE < hci_stack->acl_data_packet_length){
                hci_stack->acl_data_packet_length = HCI_ACL_PAYLOAD_SIZE;
            }
            log_info("hci_read_buffer_size: acl used size %u, count %u / sco size %u, count %u",
                     hci_stack->acl_data_packet_length, hci_stack->acl_packets_total_num,
                     hci_stack->sco_data_packet_length, hci_stack->sco_packets_total_num); 
        }
    }

    // Dump local address
    if (HCI_EVENT_IS_COMMAND_COMPLETE(packet, hci_read_bd_addr)) {
        reverse_bd_addr(&packet[OFFSET_OF_DATA_IN_COMMAND_COMPLETE + 1],
        hci_stack->local_bd_addr);
        log_info("Local Address, Status: 0x%02x: Addr: %s",
            packet[OFFSET_OF_DATA_IN_COMMAND_COMPLETE], bd_addr_to_str(hci_stack->local_bd_addr));
        if (hci_stack->link_key_db){
            hci_stack->link_key_db->set_local_bd_addr(hci_stack->local_bd_addr);
        }
    }
    if (HCI_EVENT_IS_COMMAND_COMPLETE(packet, hci_write_scan_enable))
    {
#ifdef ENABLE_CLASSIC
#error "ENABLE_CLASSIC defined but hci_emit_discoverable_enabled not implement"
        hci_emit_discoverable_enabled(hci_stack->discoverable);
#endif
    }
    // Note: HCI init checks 
    if (HCI_EVENT_IS_COMMAND_COMPLETE(packet, hci_read_local_supported_features))
    {
        memcpy(hci_stack->local_supported_features, &packet[OFFSET_OF_DATA_IN_COMMAND_COMPLETE+1], 8);

#ifdef ENABLE_CLASSIC
#error "ENABLE_CLASSIC defined but hci_acl_packet_types_for_buffer_size_and_local_features not implement"
        // determine usable ACL packet types based on host buffer size and supported features
        hci_stack->packet_types = hci_acl_packet_types_for_buffer_size_and_local_features(HCI_ACL_PAYLOAD_SIZE, &hci_stack->local_supported_features[0]);
        log_info("packet types %04x", hci_stack->packet_types); 

        // Classic/LE
        log_info("BR/EDR support %u, LE support %u", hci_classic_supported(), hci_le_supported());
#endif
    }
    if (HCI_EVENT_IS_COMMAND_COMPLETE(packet, hci_read_local_version_information)){
        // hci_stack->hci_version    = READ_BT_16(packet, 4);
        // hci_stack->hci_revision   = READ_BT_16(packet, 6);
        // hci_stack->lmp_version    = READ_BT_16(packet, 8);
        hci_stack->manufacturer   = READ_BT_16(packet, 10);
        // hci_stack->lmp_subversion = READ_BT_16(packet, 12);
        log_info("Manufacturer: 0x%04x", hci_stack->manufacturer);
        // notify app
        if (hci_stack->local_version_information_callback){
            hci_stack->local_version_information_callback(packet);
        }
    }
    if (HCI_EVENT_IS_COMMAND_COMPLETE(packet, hci_read_local_supported_commands)){
        hci_stack->local_supported_commands[0] =
            (packet[OFFSET_OF_DATA_IN_COMMAND_COMPLETE+1+14] & 0X80) >> 7 |  // Octet 14, bit 7
            (packet[OFFSET_OF_DATA_IN_COMMAND_COMPLETE+1+24] & 0x40) >> 5;   // Octet 24, bit 6 
    }
    if (HCI_EVENT_IS_COMMAND_COMPLETE(packet, hci_write_synchronous_flow_control_enable)){
        if (packet[5] == 0){
            hci_stack->synchronous_flow_control_enabled = 1;
        }
    } 
}
// avoid huge local variables
static void le_event_command_complete_handler(uint8_t *packet, int size)
{
    //
    if (HCI_EVENT_IS_COMMAND_COMPLETE(packet, hci_le_read_buffer_size)){
        hci_stack->le_data_packets_length = READ_BT_16(packet, 6);
        hci_stack->le_acl_packets_total_num  = packet[8];
        // determine usable ACL payload size
        if (HCI_ACL_PAYLOAD_SIZE < hci_stack->le_data_packets_length){
            hci_stack->le_data_packets_length = HCI_ACL_PAYLOAD_SIZE;
        }
        if (hci_stack->le_data_packets_length < 27)
        {
            //spec 4.2 Vol 2 Part E
            hci_puts("Host shall not fragment HCI ACL Data Packets on an LE-U logial link\n");
        }
        hci_deg("le_data_packets_length : %x\n", hci_stack->le_data_packets_length);
        hci_deg("le_acl_packets_total_num : %x\n", hci_stack->le_acl_packets_total_num);
    }
    if (HCI_EVENT_IS_COMMAND_COMPLETE(packet, hci_le_read_white_list_size)){
        hci_stack->le_whitelist_capacity = READ_BT_16(packet, 6);
        hci_deg("le_whitelist_capacity %x\n", hci_stack->le_whitelist_capacity);
        log_info("hci_le_read_white_list_size: size %u", hci_stack->le_whitelist_capacity);
    }   
    if (HCI_EVENT_IS_COMMAND_COMPLETE(packet, hci_le_read_resolving_list_size)){
        hci_stack->le_resolvinglist_capacity = READ_BT_16(packet, 6);
        hci_deg("le_resolvinglist_capacit %x\n", hci_stack->le_resolvinglist_capacity);
    }
    //data length extension begin
    if (HCI_EVENT_IS_COMMAND_COMPLETE(packet, hci_le_read_suggested_default_data_length))
    {
        hci_deg("suggestedMaxTxOctets: %04x\n",  READ_BT_16(packet, 6));
        hci_deg("suggestedMaxTxTime: %04x\n",    READ_BT_16(packet, 8));
    }
    if (HCI_EVENT_IS_COMMAND_COMPLETE(packet, hci_le_read_maximum_data_length))
    {
        hci_deg("supportedMaxTxOctets: %04x\n",  READ_BT_16(packet, 6));
        hci_deg("supportedMaxTxTime: %04x\n",    READ_BT_16(packet, 8));
        hci_deg("supportedMaxRxOctets: %04x\n",  READ_BT_16(packet, 10));
        hci_deg("supportedMaxRxTime: %04x\n",    READ_BT_16(packet, 12));
    }
    //data length extension end
}

static void le_meta_event_handler(uint8_t *packet, int size)
{
	bd_addr_t addr;
	bd_addr_type_t addr_type;
	uint8_t link_type;
	hci_con_handle_t handle;
	hci_connection_t * conn;
	int i;

    switch (packet[2])
    {
        case HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE:
            hci_puts("le_data_length_change\n");
            hci_deg("connEffectiveMaxTxOctets: %04x\n",    packet[5]<<8|packet[4]);
            hci_deg("connEffectiveMaxTxTime:   %04x\n",    packet[7]<<8|packet[6]);
            hci_deg("connEffectiveMaxRxOctets: %04x\n",    packet[9]<<8|packet[8]);
            hci_deg("connEffectiveMaxRxTime:   %04x\n",    packet[11]<<8|packet[10]);
            break;
        case HCI_SUBEVENT_LE_ADVERTISING_REPORT:
            hci_puts("advertising report received");
            if (hci_stack->le_scanning_state != LE_SCANNING)
                break;
            le_handle_advertisement_report(packet, size);
            break;

        case HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE_EVENT:
            hci_puts("le_enhanced_connection_complete\n");
            hci_deg("interval: %04x\n",    packet[27]<<8|packet[26]);
            hci_deg("latency : %04x\n",    packet[29]<<8|packet[28]);
            hci_deg("timeout : %04x\n",    packet[31]<<8|packet[30]);
            hci_puts("local RPA : "); printf_buf(&packet[14], 6); bt_flip_addr(hci_stack->adv_address, &packet[14]);
            hci_puts("peer RPA : "); printf_buf(&packet[20], 6);
            hci_puts("\n");
        case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
            // Connection management
            hci_puts("le_connection_complete\n");
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
            conn->role = packet[6];
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
}

// Create various non-HCI events. 
// TODO: generalize, use table similar to hci_create_command

static void hci_emit_event(uint8_t * event, uint16_t size, int dump){
    // dump packet
    /* if (dump) { */
        /* hci_dump_packet( HCI_EVENT_PACKET, 0, event, size); */
    /* }  */

    // dispatch to all event handlers
    linked_list_iterator_t it;
    linked_list_iterator_init(&it, &hci_stack->event_handlers);
    while (linked_list_iterator_has_next(&it)){
        btstack_packet_callback_registration_t * entry = (btstack_packet_callback_registration_t*) linked_list_iterator_next(&it);
        entry->callback(HCI_EVENT_PACKET, 0, event, size);
    }
}

static void event_handler(uint8_t *packet, int size)
{
	uint16_t event_length = packet[1];

	// assert packet is complete
	if (size != event_length + 2){
        hci_puts("event_handler : packet is incomplete\n");
		return;
	}

	bd_addr_t addr;
	bd_addr_type_t addr_type;
	uint8_t link_type;
	hci_con_handle_t handle;
	hci_connection_t * conn;
	int i;

	switch (hci_event_packet_get_type(packet)) {
		case HCI_EVENT_COMMAND_COMPLETE:
            // get num cmd packets
            hci_stack->num_cmd_packets = packet[2];
            //for Classic
            event_command_complete_handler(packet, size);

#ifdef ENABLE_BLE
            //for LE
            le_event_command_complete_handler(packet, size);
#endif
			break;

		case HCI_EVENT_COMMAND_STATUS:
			// get num cmd packets
			hci_stack->num_cmd_packets = packet[3];
			break;

		case HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS:
            hci_puts("HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS\n");
			{
				int offset = 3;

				for (i=0; i<packet[2];i++)
				{
					handle = READ_BT_16(packet, offset);
					offset += 2;
					uint16_t num_packets = READ_BT_16(packet, offset);
					offset += 2;
                    hci_deg("acl pkt complete : %x\n", num_packets);

					conn = hci_connection_for_handle(handle);
					if (!conn){
						continue;
					}

					if (conn->num_acl_packets_sent >= num_packets){
						conn->num_acl_packets_sent -= num_packets;
					} else {
						conn->num_acl_packets_sent = 0;
					}
                    hci_deg("acl pkt remain : %x\n", conn->num_acl_packets_sent);
				}
			}
			break;

		case HCI_EVENT_ENCRYPTION_CHANGE:
			handle = READ_BT_16(packet, 3);
			conn = hci_connection_for_handle(handle);
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
			conn = hci_connection_for_handle(handle);
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
			hci_deg("hci_event_disconnect: %x\n", handle);
            // drop outgoing ACL fragments if it is for closed connection
            if (hci_stack->acl_fragmentation_total_size > 0) {
                if (handle == READ_ACL_CONNECTION_HANDLE(hci_stack->hci_packet_buffer)){
                    log_info("hci: drop fragmented ACL data for closed connection");
                     hci_stack->acl_fragmentation_total_size = 0;
                     hci_stack->acl_fragmentation_pos = 0;
                }
            }
            // re-enable advertisements for le connections if active
            conn = hci_connection_for_handle(handle);
            if (!conn) break; 
            if (hci_is_le_connection(conn) && hci_stack->le_advertisements_enabled){
                hci_stack->le_advertisements_todo |= LE_ADVERTISEMENT_TASKS_ENABLE;
            }
            conn->state = RECEIVED_DISCONNECTION_COMPLETE;
			break;

#ifdef ENABLE_BLE
		case HCI_EVENT_LE_META:
            le_meta_event_handler(packet, size);
			break;
#endif

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
			&& HCI_EVENT_IS_COMMAND_COMPLETE(packet, hci_write_scan_enable)){
		hci_initializing_next_state();
	}
#endif


	// notify upper stack
	/* hci_stack->packet_handler(HCI_EVENT_PACKET, packet, size); */
    hci_emit_event(packet, size, 0);


	// moved here to give upper stack a chance to close down everything with hci_connection_t intact
	if (packet[0] == HCI_EVENT_DISCONNECTION_COMPLETE){
		if (!packet[2]){
			handle = READ_BT_16(packet, 3);
			hci_connection_t * conn = hci_connection_for_handle(handle);

            /* hci_deg("conn handle %x\n", handle); */
			if (conn) {
                /* hci_deg("conn->addr %02x - %02x - %02x - %02x - %02x - %02x \n", conn->address[0], conn->address[1],conn->address[2],conn->address[3],conn->address[4],conn->address[5]); */
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
	hci_run();
}

/**
 * @brief Add event packet handler. 
 */
void hci_add_event_handler(btstack_packet_callback_registration_t * callback_handler){
    linked_list_add_tail(&hci_stack->event_handlers, (linked_item_t*) callback_handler);
}


/** Register HCI packet handlers */
void hci_register_acl_packet_handler(btstack_packet_handler_t handler){
    hci_stack->acl_packet_handler = handler;
}

/**
 * @brief Registers a packet handler for SCO data. Used for HSP and HFP profiles.
 */
/* void hci_register_sco_packet_handler(btstack_packet_handler_t handler){ */
    /* hci_stack->sco_packet_handler = handler;     */
/* } */

static void hci_packet_handler(uint8_t packet_type, uint8_t *packet, uint16_t size)
{
    /* hci_puts("--HCI PH "); */
    switch (packet_type) 
	{
        case HCI_EVENT_PACKET:
            event_handler(packet, size);
            break;
        case HCI_ACL_DATA_PACKET:
            hci_puts("HCI_ACL_DATA_PACKET\n");
            acl_handler(packet, size);
            break;
        default:
            break;
    }
    /* hci_puts("\nHCI exit "); */
}

/** Register HCI packet handlers */
/* void le_hci_register_packet_handler(void (*handler)(uint8_t packet_type, uint8_t *packet, uint16_t size)){ */
    /* hci_stack->packet_handler = handler; */
/* } */

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
    /* hci_stack->packet_handler = dummy_handler; */

    // store and open remote device db
    hci_stack->remote_device_db = remote_device_db;
    if (hci_stack->remote_device_db) {
        hci_stack->remote_device_db->open();
    }
    
    // max acl payload size defined in config.h
    hci_stack->acl_data_packet_length = HCI_ACL_PAYLOAD_SIZE;
    
    // register packet handlers with transport
    transport->register_packet_handler(&hci_packet_handler);

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
	hci_run();
	
    return 0;
}


void le_hci_local_bd_addr(bd_addr_t address_buffer){
    memcpy(address_buffer, hci_stack->local_bd_addr, 6);
}

static bool hci_le_sub_run(void)
{
    hci_puts("\nhci_le_sub_run - ");
    if (hci_stack->state == HCI_STATE_WORKING){
        // handle le scan
        switch(hci_stack->le_scanning_state){
            case LE_START_SCAN:
                hci_stack->le_scanning_state = LE_SCANNING;
                hci_send_cmd(&hci_le_set_scan_enable, 1, 0);
                hci_puts("hci_le_set_scan_enable 1\n");
                return TRUE;
                
            case LE_STOP_SCAN:
                hci_stack->le_scanning_state = LE_SCAN_IDLE;
                hci_send_cmd(&hci_le_set_scan_enable, 0, 0);
                hci_puts("hci_le_set_scan_enable 0\n");
                return TRUE;
            default:
                break;
        }
        if (hci_stack->le_scan_type != 0xff){
            // defaults: active scanning, accept all advertisement packets
            int scan_type = hci_stack->le_scan_type;
            hci_stack->le_scan_type = 0xff;
            hci_send_cmd(&hci_le_set_scan_parameters, scan_type, hci_stack->le_scan_interval, hci_stack->le_scan_window, hci_stack->adv_addr_type, 0);
            hci_puts("hci_le_set_scan_parameters\n");
            return TRUE;
        }
        // le advertisement control
        if (hci_stack->le_advertisements_todo){
            log_info("hci_run: gap_le: adv todo: %x", hci_stack->le_advertisements_todo );
            hci_deg("hci_run: gap_le: adv todo: %x", hci_stack->le_advertisements_todo );
        }
        if (hci_stack->le_advertisements_todo & LE_ADVERTISEMENT_TASKS_DISABLE){
            hci_stack->le_advertisements_todo &= ~LE_ADVERTISEMENT_TASKS_DISABLE;
            hci_send_cmd(&hci_le_set_advertise_enable, 0);
            hci_puts("LE_ADVERTISEMENT_TASKS_DISABLE\n");
            return TRUE;
        }
        if (hci_stack->le_advertisements_todo & LE_ADVERTISEMENT_TASKS_SET_PARAMS){
            hci_stack->le_advertisements_todo &= ~LE_ADVERTISEMENT_TASKS_SET_PARAMS;
            hci_send_cmd(&hci_le_set_advertising_parameters,
                 hci_stack->le_advertisements_interval_min,
                 hci_stack->le_advertisements_interval_max,
                 hci_stack->le_advertisements_type,
                 hci_stack->le_advertisements_own_address_type,
                 hci_stack->le_advertisements_direct_address_type,
                 hci_stack->le_advertisements_direct_address,
                 hci_stack->le_advertisements_channel_map,
                 hci_stack->le_advertisements_filter_policy);
            hci_puts("LE_ADVERTISEMENT_TASKS_SET_PARAMS\n");
            return TRUE;
        }
        if (hci_stack->le_advertisements_todo & LE_ADVERTISEMENT_TASKS_SET_ADV_DATA){
            hci_stack->le_advertisements_todo &= ~LE_ADVERTISEMENT_TASKS_SET_ADV_DATA;
            hci_puts("LE_ADVERTISEMENT_TASKS_SET_ADV_DATA\n");
            hci_send_cmd(&hci_le_set_advertising_data, hci_stack->le_advertisements_data_len, hci_stack->le_advertisements_data_len,
                hci_stack->le_advertisements_data);
            hci_puts("LE_ADVERTISEMENT_TASKS_SET_ADV_DATA 2\n");
            return TRUE;
        }
        if (hci_stack->le_advertisements_todo & LE_ADVERTISEMENT_TASKS_SET_SCAN_DATA){
            hci_stack->le_advertisements_todo &= ~LE_ADVERTISEMENT_TASKS_SET_SCAN_DATA;
            hci_send_cmd(&hci_le_set_scan_response_data, hci_stack->le_scan_response_data_len, hci_stack->le_scan_response_data_len,
                hci_stack->le_scan_response_data);
            hci_puts("LE_ADVERTISEMENT_TASKS_SET_SCAN_DATA\n");
            return TRUE;
        }
        if (hci_stack->le_advertisements_todo & LE_ADVERTISEMENT_TASKS_ENABLE){
            hci_stack->le_advertisements_todo &= ~LE_ADVERTISEMENT_TASKS_ENABLE;
            hci_send_cmd(&hci_le_set_advertise_enable, 1);
            hci_puts("LE_ADVERTISEMENT_TASKS_ENABLE\n");
            return TRUE;
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
                hci_send_cmd(&hci_le_create_connection_cancel);
                hci_puts("LE_CONNECTING_IDLE - hci_le_create_connection_cancel\n");
                return TRUE;
            }

            // add/remove entries
            linked_list_iterator_init(&lit, &hci_stack->le_whitelist);
            while (linked_list_iterator_has_next(&lit)){
                whitelist_entry_t * entry = (whitelist_entry_t*) linked_list_iterator_next(&lit);
                if (entry->state & LE_WHITELIST_ADD_TO_CONTROLLER){
                    entry->state = LE_WHITELIST_ON_CONTROLLER;
                    hci_send_cmd(&hci_le_add_device_to_white_list, entry->address_type, entry->address);
                    hci_puts("LE_WHITELIST_ADD_TO_CONTROLLER - hci_le_add_device_to_white_list\n");
                    return TRUE;

                }
                if (entry->state & LE_WHITELIST_REMOVE_FROM_CONTROLLER){
                    bd_addr_t address;
                    bd_addr_type_t address_type = entry->address_type;                    
                    memcpy(address, entry->address, 6);
                    linked_list_remove(&hci_stack->le_whitelist, (linked_item_t *) entry);
                    btstack_memory_whitelist_entry_free(entry);
                    hci_send_cmd(&hci_le_remove_device_from_white_list, address_type, address);
                    hci_puts("LE_WHITELIST_REMOVE_FROM_CONTROLLER - hci_le_add_device_to_white_list\n");
                    return TRUE;
                }
            }
        }

        // start connecting
        if ( hci_stack->le_connecting_state == LE_CONNECTING_IDLE && 
            !linked_list_empty(&hci_stack->le_whitelist)){
            bd_addr_t null_addr;
            memset(null_addr, 0, 6);
            hci_send_cmd(&hci_le_create_connection,
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
            return TRUE;
        }
    }

    return FALSE;
}

static void hci_run(void)
{

	// log_info("hci_run: entered");
    /* hci_puts("hci_run: entered"); */
	hci_connection_t * connection;
	linked_item_t * it;

	// send continuation fragments first, as they block the prepared packet buffer
	if (hci_stack->acl_fragmentation_total_size > 0) {
		hci_con_handle_t con_handle = READ_ACL_CONNECTION_HANDLE(hci_stack->hci_packet_buffer);
		if (hci_can_send_prepared_acl_packet_now(con_handle)){
			hci_connection_t *connection = hci_connection_for_handle(con_handle);
			if (connection) {
				hci_send_acl_packet_fragments(connection);
				return;
			} 
			// connection gone -> discard further fragments
			hci_stack->acl_fragmentation_total_size = 0;
			hci_stack->acl_fragmentation_pos = 0;
		}
	}

	if (!hci_can_send_command_packet_now()) {
        hci_puts("hci_run - hci_can_send_command_packet_now\n");
        return;   
    }


#ifdef ENABLE_BLE
    if (hci_le_sub_run() == TRUE)       return;
#endif

	// send pending HCI commands
	for (it = (linked_item_t *) hci_stack->connections; it ; it = it->next){
		connection = (hci_connection_t *) it;

		switch(connection->state){
			case SEND_CREATE_CONNECTION:
                switch (connection->address_type){
                    case BD_ADDR_TYPE_CLASSIC:
                        log_info("sending hci_le_create_connection");
                        ASSERT(0, "%s\n", "--TO*DO--");
                        /* hci_send_cmd(&hci_create_connection, connection->address, hci_usable_acl_packet_types(), 0, 0, 0, 1); */
                        break;
                    default:
#ifdef ENABLE_BLE
                        log_info("sending hci_le_create_connection");
                        hci_send_cmd(&hci_le_create_connection,
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
#endif
                        break;
                }
				return;

#ifdef ENABLE_BLE
			case SEND_CANCEL_CONNECTION:
				connection->state = SENT_CANCEL_CONNECTION;
				hci_send_cmd(&hci_le_create_connection_cancel);
				return;
#endif

			case SEND_DISCONNECT:
				connection->state = SENT_DISCONNECT;
				hci_send_cmd(&hci_disconnect, connection->con_handle, 0x13); // remote closed connection
				return;

			default:
				break;
		}

#ifdef ENABLE_BLE
        if (connection->le_con_parameter_update_state == CON_PARAMETER_UPDATE_CHANGE_HCI_CON_PARAMETERS){
            connection->le_con_parameter_update_state = CON_PARAMETER_UPDATE_NONE; 
            
            uint16_t connection_interval_min = connection->le_conn_interval_min;
            connection->le_conn_interval_min = 0;
            hci_puts("hci_le_connection_update\n");
            hci_send_cmd(&hci_le_connection_update, connection->con_handle, connection_interval_min,
                connection->le_conn_interval_max, connection->le_conn_latency, connection->le_supervision_timeout,
                0x0000, 0xffff);
        }
#endif
	}

	switch (hci_stack->state){
		case HCI_STATE_INITIALIZING:
			hci_initializing_run();
			break;
#if 0 
		case HCI_STATE_HALTING:

			log_info("HCI_STATE_HALTING");
            // free whitelist entries
#ifdef ENABLE_BLE
            {
                btstack_linked_list_iterator_t lit;
                btstack_linked_list_iterator_init(&lit, &hci_stack->le_whitelist);
                while (btstack_linked_list_iterator_has_next(&lit)){
                    whitelist_entry_t * entry = (whitelist_entry_t*) btstack_linked_list_iterator_next(&lit);
                    btstack_linked_list_remove(&hci_stack->le_whitelist, (btstack_linked_item_t *) entry);
                    btstack_memory_whitelist_entry_free(entry);
                }
            }
#endif
			// close all open connections
			connection =  (hci_connection_t *) hci_stack->connections;
			if (connection){

				// send disconnect
                if (!hci_can_send_command_packet_now()) {
                    hci_puts("hci_run2 - hci_can_send_command_packet_now\n");
                    return;   
                }

				log_info("HCI_STATE_HALTING, connection %p, handle %u", connection, (uint16_t)connection->con_handle);

				// send disconnected event right away - causes higher layer connections to get closed, too.
				hci_shutdown_connection(connection);

                // finally, send the disconnect command
				hci_send_cmd(&hci_disconnect, connection->con_handle, 0x13);  // remote closed connection
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
                        if (!hci_can_send_command_packet_now()) {
                            hci_puts("hci_run3 - hci_can_send_command_packet_now\n");
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
                        if (!hci_can_send_command_packet_now()) {
                            hci_puts("hci_run4 - hci_can_send_command_packet_now\n");
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
    /* hci_puts("debug2\n"); */
}

int hci_send_cmd_packet(uint8_t *packet, int size)
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
    hci_connection_t * connection = hci_connection_for_handle(con_handle);
    if (!connection) return;
    connection->bonding_flags |= BONDING_DISCONNECT_SECURITY_BLOCK;
}
#endif

/**
 * pre: numcmds >= 0 - it's allowed to send a command to the controller
 */
int hci_send_cmd(const hci_cmd_t *cmd, ...)
{
    if (!hci_can_send_command_packet_now()){ 
        hci_deg("hci_send_cmd called but cannot send packet now");
        return 0;
    }

    // for HCI INITIALIZATION
    // log_info("hci_send_cmd: opcode %04x", cmd->opcode);
    /* hci_deg("hci_send_cmd: ogf %04x- ocf %04x\n", cmd->opcode>>10, cmd->opcode & 0x3ff); */
    
    hci_stack->last_cmd_opcode = cmd->opcode;

    hci_reserve_packet_buffer();
    uint8_t * packet = hci_stack->hci_packet_buffer;

    va_list argptr;
    va_start(argptr, cmd);
    uint16_t size = le_hci_create_cmd_internal(packet, cmd, argptr);
    va_end(argptr);

    return hci_send_cmd_packet(packet, size);
}


/**********************************huayue add************************/
int pc_h4_send_hci_cmd(void *data, u16 size)
{
    hci_reserve_packet_buffer();
    uint8_t * packet = hci_stack->hci_packet_buffer;

    memcpy(packet, data, size);

    return hci_send_cmd_packet(packet, size);
}

// Create various non-HCI events. 
// TODO: generalize, use table similar to hci_create_command
void hci_emit_state()
{
    log_info("BTSTACK_EVENT_STATE %u", hci_stack->state);
    hci_deg("BTSTACK_EVENT_STATE %x\n", hci_stack->state);
    uint8_t event[3];
    event[0] = BTSTACK_EVENT_STATE;
    event[1] = sizeof(event) - 2;
    event[2] = hci_stack->state;
    /* hci_stack->packet_handler(HCI_EVENT_PACKET, event, sizeof(event)); */
    hci_emit_event(event, sizeof(event), 0);
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
    /* hci_stack->packet_handler(HCI_EVENT_PACKET, event, sizeof(event)); */
    hci_emit_event(event, sizeof(event), 0);
}

void le_hci_emit_disconnection_complete(uint16_t handle, uint8_t reason){
    uint8_t event[6];
    event[0] = HCI_EVENT_DISCONNECTION_COMPLETE;
    event[1] = sizeof(event) - 2;
    event[2] = 0; // status = OK
    bt_store_16(event, 3, handle);
    event[5] = reason;
    /* hci_stack->packet_handler(HCI_EVENT_PACKET, event, sizeof(event)); */
    hci_emit_event(event, sizeof(event), 0);
}

void le_hci_emit_l2cap_check_timeout(hci_connection_t *conn){
    if (disable_l2cap_timeouts) return;
    log_info("L2CAP_EVENT_TIMEOUT_CHECK");
    uint8_t event[4];
    event[0] = L2CAP_EVENT_TIMEOUT_CHECK;
    event[1] = sizeof(event) - 2;
    bt_store_16(event, 2, conn->con_handle);
    /* hci_stack->packet_handler(HCI_EVENT_PACKET, event, sizeof(event)); */
    hci_emit_event(event, sizeof(event), 0);
}

void le_hci_emit_nr_connections_changed(){
    log_info("BTSTACK_EVENT_NR_CONNECTIONS_CHANGED %u", nr_hci_connections());
    /* hci_deg("BTSTACK_EVENT_NR_CONNECTIONS_CHANGED %x", nr_hci_connections()); */
    uint8_t event[3];
    event[0] = BTSTACK_EVENT_NR_CONNECTIONS_CHANGED;
    event[1] = sizeof(event) - 2;
    event[2] = nr_hci_connections();
    /* hci_stack->packet_handler(HCI_EVENT_PACKET, event, sizeof(event)); */
    hci_emit_event(event, sizeof(event), 0);
}

void le_hci_emit_hci_open_failed(){
    log_info("BTSTACK_EVENT_POWERON_FAILED");
    uint8_t event[2];
    event[0] = BTSTACK_EVENT_POWERON_FAILED;
    event[1] = sizeof(event) - 2;
    /* hci_stack->packet_handler(HCI_EVENT_PACKET, event, sizeof(event)); */
    hci_emit_event(event, sizeof(event), 0);
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
    /* hci_stack->packet_handler(HCI_EVENT_PACKET, event, sizeof(event)); */
    hci_emit_event(event, sizeof(event), 0);
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
    hci_connection_t * connection = hci_connection_for_handle(con_handle);
    if (!connection) return LEVEL_0;
    return gap_security_level_for_connection(connection);
}

/**
 * @brief request connection to device to
 * @result GAP_AUTHENTICATION_RESULT
 */
void gap_request_security_level(hci_con_handle_t con_handle, gap_security_level_t requested_level)
{
    hci_connection_t * connection = hci_connection_for_handle(con_handle);
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
    hci_run();
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

    hci_run();

    return 0;
}

void gap_set_local_name(const char * local_name){
    hci_stack->local_name = local_name;
}

le_command_status_t le_central_start_scan(){
    if (hci_stack->le_scanning_state == LE_SCANNING) return BLE_PERIPHERAL_OK;
    hci_stack->le_scanning_state = LE_START_SCAN;
    hci_run();
    return BLE_PERIPHERAL_OK;
}

le_command_status_t le_central_stop_scan(){
    if ( hci_stack->le_scanning_state == LE_SCAN_IDLE) return BLE_PERIPHERAL_OK;
    hci_stack->le_scanning_state = LE_STOP_SCAN;
    hci_run();
    return BLE_PERIPHERAL_OK;
}

void le_central_set_scan_parameters(uint8_t scan_type, uint16_t scan_interval, uint16_t scan_window){
    hci_stack->le_scan_type     = scan_type;
    hci_stack->le_scan_interval = scan_interval;
    hci_stack->le_scan_window   = scan_window;
    hci_run();
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
        hci_run();
        return BLE_PERIPHERAL_OK;
    }
    
    if (conn->state == SEND_CREATE_CONNECTION ||
        conn->state == SENT_CREATE_CONNECTION) {
        hci_emit_le_connection_complete(conn->address_type, conn->address, 0, ERROR_CODE_COMMAND_DISALLOWED);
        return BLE_PERIPHERAL_IN_WRONG_STATE;
    }
    
    hci_emit_le_connection_complete(conn->address_type, conn->address, conn->con_handle, 0);
    hci_run();

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
            hci_run();
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
    hci_connection_t * connection = hci_connection_for_handle(con_handle);
    if (!connection) return ERROR_CODE_UNKNOWN_CONNECTION_IDENTIFIER;
    connection->le_conn_interval_min = conn_interval_min;
    connection->le_conn_interval_max = conn_interval_max;
    connection->le_conn_latency = conn_latency;
    connection->le_supervision_timeout = supervision_timeout;
    connection->le_con_parameter_update_state = CON_PARAMETER_UPDATE_CHANGE_HCI_CON_PARAMETERS;
    hci_run();
    return 0;
}


static void gap_advertisments_changed(void){
    // disable advertisements before updating adv, scan data, or adv params
    if (hci_stack->le_advertisements_active){
        hci_stack->le_advertisements_todo |= LE_ADVERTISEMENT_TASKS_DISABLE | LE_ADVERTISEMENT_TASKS_ENABLE;
    }
    hci_run();
}
/**
 * @brief Set Advertisement Data
 * @param advertising_data_length
 * @param advertising_data (max 31 octets)
 * @note data is not copied, pointer has to stay valid
 */
void gap_advertisements_set_data(uint8_t advertising_data_length, uint8_t * advertising_data){
    hci_puts("gap_advertisements_set_data\n");
    hci_stack->le_advertisements_data_len = advertising_data_length;
    hci_stack->le_advertisements_data = advertising_data;
    hci_stack->le_advertisements_todo |= LE_ADVERTISEMENT_TASKS_SET_ADV_DATA;
    gap_advertisments_changed();
}

/** 
 * @brief Set Scan Response Data
 * @param advertising_data_length
 * @param advertising_data (max 31 octets)
 * @note data is not copied, pointer has to stay valid
 */
void gap_scan_response_set_data(uint8_t scan_response_data_length, uint8_t * scan_response_data){
    hci_puts("gap_scan_response_set_data\n");
    hci_stack->le_scan_response_data_len = scan_response_data_length;
    hci_stack->le_scan_response_data = scan_response_data;
    hci_stack->le_advertisements_todo |= LE_ADVERTISEMENT_TASKS_SET_SCAN_DATA;
    gap_advertisments_changed();
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
    gap_advertisments_changed();
}

/**
 * @brief Enable/Disable Advertisements
 * @param enabled
 */
void gap_advertisements_enable(int enabled){
    hci_puts("gap_advertisements_enable\n");
    hci_stack->le_advertisements_enabled = enabled;
    if (enabled && !hci_stack->le_advertisements_active){
        hci_stack->le_advertisements_todo |= LE_ADVERTISEMENT_TASKS_ENABLE;
    }
    if (!enabled && hci_stack->le_advertisements_active){
        hci_stack->le_advertisements_todo |= LE_ADVERTISEMENT_TASKS_DISABLE;
    }
    hci_run();
}


uint8_t gap_disconnect(hci_con_handle_t handle){
    hci_connection_t * conn = hci_connection_for_handle(handle);
    if (!conn){
        le_hci_emit_disconnection_complete(handle, 0);
        return BLE_PERIPHERAL_OK;
    }
    conn->state = SEND_DISCONNECT;
    hci_run();
    return BLE_PERIPHERAL_OK;
}

#ifdef ENABLE_BLE

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
    hci_run();
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
    hci_run();
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
    hci_run();
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
    hci_run();
}

