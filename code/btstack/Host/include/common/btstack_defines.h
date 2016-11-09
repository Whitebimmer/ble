/*
 * Copyright (C) 2015 BlueKitchen GmbH
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
 * btstack-defines.h
 *
 * BTstack definitions, events, and error codes */

#ifndef __BTSTACK_DEFINES_H
#define __BTSTACK_DEFINES_H

#include <stdint.h>
#include <linked_list.h>

// TYPES

// packet handler
typedef void (*btstack_packet_handler_t) (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

// packet callback supporting multiple registrations
typedef struct {
    linked_item_t    item;
    btstack_packet_handler_t callback;
} btstack_packet_callback_registration_t;


/**
 * packet types - used in BTstack and over the H4 UART interface
 */
#define HCI_COMMAND_DATA_PACKET 0x01
#define HCI_ACL_DATA_PACKET     0x02
#define HCI_SCO_DATA_PACKET     0x03
#define HCI_EVENT_PACKET          0x04

// extension for client/server communication
#define DAEMON_EVENT_PACKET     0x05
    
// L2CAP data
#define L2CAP_DATA_PACKET       0x06

// RFCOMM data
#define RFCOMM_DATA_PACKET      0x07

// Attribute protocol data
#define ATT_DATA_PACKET         0x08

// Security Manager protocol data
#define SM_DATA_PACKET          0x09

// SDP query result
// format: type (8), record_id (16), attribute_id (16), attribute_length (16), attribute_value (max 1k)
#define SDP_CLIENT_PACKET       0x0a

// BNEP data
#define BNEP_DATA_PACKET        0x0b

// Unicast Connectionless Data
#define UCD_DATA_PACKET         0x0c
 
// debug log messages
#define LOG_MESSAGE_PACKET      0xfc

    
// Fixed PSM numbers
#define PSM_SDP    0x01
#define PSM_RFCOMM 0x03
#define PSM_HID_CONTROL 0x11
#define PSM_HID_INTERRUPT 0x13
#define PSM_BNEP   0x0F

// Events from host controller to host

/**
 * @format 1
 * @param status
 */
#define HCI_EVENT_INQUIRY_COMPLETE                         0x01
// no format yet, can contain multiple results

/** 
 * @format 1B11132
 * @param num_responses
 * @param bd_addr
 * @param page_scan_repetition_mode
 * @param reserved1
 * @param reserved2
 * @param class_of_device
 * @param clock_offset
 */
#define HCI_EVENT_INQUIRY_RESULT                           0x02

/**
 * @format 12B11
 * @param status
 * @param connection_handle
 * @param bd_addr
 * @param link_type
 * @param encryption_enabled
 */
#define HCI_EVENT_CONNECTION_COMPLETE                      0x03
/**
 * @format B31
 * @param bd_addr
 * @param class_of_device
 * @param link_type
 */
#define HCI_EVENT_CONNECTION_REQUEST                       0x04
/**
 * @format 121
 * @param status
 * @param connection_handle
 * @param reason 
 */
#define HCI_EVENT_DISCONNECTION_COMPLETE                   0x05
/**
 * @format 12
 * @param status
 * @param connection_handle
 */
#define HCI_EVENT_AUTHENTICATION_COMPLETE_EVENT            0x06
/**
 * @format 1BN
 * @param status
 * @param bd_addr
 * @param remote_name
 */
#define HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE             0x07
/**
 * @format 121
 * @param status
 * @param connection_handle
 * @param encryption_enabled 
 */
#define HCI_EVENT_ENCRYPTION_CHANGE                        0x08
/**
 * @format 12
 * @param status
 * @param connection_handle
 */
#define HCI_EVENT_CHANGE_CONNECTION_LINK_KEY_COMPLETE      0x09
/**
 * @format 121
 * @param status
 * @param connection_handle
 * @param key_flag 
 */
#define HCI_EVENT_MASTER_LINK_KEY_COMPLETE                 0x0A
#define HCI_EVENT_READ_REMOTE_SUPPORTED_FEATURES_COMPLETE  0x0B
#define HCI_EVENT_READ_REMOTE_VERSION_INFORMATION_COMPLETE 0x0C
#define HCI_EVENT_QOS_SETUP_COMPLETE                       0x0D

/**
 * @format 12R
 * @param num_hci_command_packets
 * @param command_opcode
 * @param return_parameters
 */
#define HCI_EVENT_COMMAND_COMPLETE                         0x0E
/**
 * @format 112
 * @param status
 * @param num_hci_command_packets
 * @param command_opcode
 */
#define HCI_EVENT_COMMAND_STATUS                                   0x0F

/**
 * @format 121
 * @param hardware_code
 */
#define HCI_EVENT_HARDWARE_ERROR                           0x10

#define HCI_EVENT_FLUSH_OCCURED                            0x11
#define HCI_EVENT_ROLE_CHANGE                              0x12
#define HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS              0x13
#define HCI_EVENT_MODE_CHANGE_EVENT                        0x14
#define HCI_EVENT_RETURN_LINK_KEYS                         0x15
#define HCI_EVENT_PIN_CODE_REQUEST                         0x16
#define HCI_EVENT_LINK_KEY_REQUEST                         0x17
#define HCI_EVENT_LINK_KEY_NOTIFICATION                    0x18
#define HCI_EVENT_DATA_BUFFER_OVERFLOW                     0x1A
#define HCI_EVENT_MAX_SLOTS_CHANGED                        0x1B
#define HCI_EVENT_READ_CLOCK_OFFSET_COMPLETE               0x1C
#define HCI_EVENT_PACKET_TYPE_CHANGED                      0x1D

/** 
 * @format 1B11321
 * @param num_responses
 * @param bd_addr
 * @param page_scan_repetition_mode
 * @param reserved
 * @param class_of_device
 * @param clock_offset
 * @param rssi
 */
#define HCI_EVENT_INQUIRY_RESULT_WITH_RSSI                 0x22

/**
 * @format 1HB111221
 * @param status
 * @param handle
 * @param bd_addr
 * @param link_type
 * @param transmission_interval
 * @param retransmission_interval
 * @param rx_packet_length
 * @param tx_packet_length
 * @param air_mode
 */
#define HCI_EVENT_SYNCHRONOUS_CONNECTION_COMPLETE          0x2C

// TODO: serialize extended_inquiry_response and provide parser
/** 
 * @format 1B11321
 * @param num_responses
 * @param bd_addr
 * @param page_scan_repetition_mode
 * @param reserved
 * @param class_of_device
 * @param clock_offset
 * @param rssi
 */
#define HCI_EVENT_EXTENDED_INQUIRY_RESPONSE                0x2F

#define HCI_EVENT_ENCRYPTION_KEY_REFRESH_COMPLETE          0x30
#define HCI_EVENT_IO_CAPABILITY_REQUEST                    0x31
#define HCI_EVENT_IO_CAPABILITY_RESPONSE                   0x32
#define HCI_EVENT_USER_CONFIRMATION_REQUEST                0x33
#define HCI_EVENT_USER_PASSKEY_REQUEST                     0x34
#define HCI_EVENT_REMOTE_OOB_DATA_REQUEST                  0x35
#define HCI_EVENT_SIMPLE_PAIRING_COMPLETE                  0x36

#define HCI_EVENT_LE_META                                  0x3E

/** 
 * @format 11H11A2221
 * @param subevent_code
 * @param status
 * @param connection_handle
 * @param role
 * @param peer_address_type
 * @param peer_address
 * @param conn_interval
 * @param conn_latency
 * @param supervision_timeout
 * @param master_clock_accuracy
 */
#define HCI_SUBEVENT_LE_CONNECTION_COMPLETE                0x01
#define HCI_SUBEVENT_LE_ADVERTISING_REPORT                 0x02
#define HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE         0x03
#define HCI_SUBEVENT_LE_READ_REMOTE_USED_FEATURES_COMPLETE 0x04
#define HCI_SUBEVENT_LE_LONG_TERM_KEY_REQUEST              0x05
#define HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE                 0x07
/** 
 * @format 11H11AAA2221
 * @param subevent_code
 * @param status
 * @param connection_handle
 * @param role
 * @param peer_address_type
 * @param peer_address
 * @param local_irk 
 * @param peer_irk 
 * @param conn_interval
 * @param conn_latency
 * @param supervision_timeout
 * @param master_clock_accuracy
 */
#define HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE_EVENT 0x0A
    
// last used HCI_EVENT in 2.1 is 0x3d
// last used HCI_EVENT in 4.1 is 0x57

/**
 * @format 1
 * @param state
 */
#define BTSTACK_EVENT_STATE                                0x60

// data: event(8), len(8), nr hci connections
#define BTSTACK_EVENT_NR_CONNECTIONS_CHANGED               0x61

/**
 * @format 
 */
#define BTSTACK_EVENT_POWERON_FAILED                       0x62

/**
 * @format 112
 * @param major
 * @param minor
 @ @param revision
 */
#define BTSTACK_EVENT_VERSION                              0x63

// data: system bluetooth on/off (bool)
#define BTSTACK_EVENT_SYSTEM_BLUETOOTH_ENABLED             0x64

// data: event (8), len(8), status (8) == 0, address (48), name (1984 bits = 248 bytes)
#define BTSTACK_EVENT_REMOTE_NAME_CACHED                   0x65

// data: discoverable enabled (bool)
#define BTSTACK_EVENT_DISCOVERABLE_ENABLED                 0x66

// Daemon Events used internally

// data: event(8)
#define DAEMON_EVENT_CONNECTION_OPENED                     0x68

// data: event(8)
#define DAEMON_EVENT_CONNECTION_CLOSED                     0x69

// data: event(8), nr_connections(8)
#define DAEMON_NR_CONNECTIONS_CHANGED                      0x6A

// data: event(8)
#define DAEMON_EVENT_NEW_RFCOMM_CREDITS                    0x6B

// data: event(8)
#define DAEMON_EVENT_HCI_PACKET_SENT                       0x6C

// L2CAP EVENTS
    
// data: event (8), len(8), status (8), address(48), handle (16), psm (16), local_cid(16), remote_cid (16), local_mtu(16), remote_mtu(16), flush_timeout(16)
#define L2CAP_EVENT_CHANNEL_OPENED                         0x70

// data: event (8), len(8), channel (16)
#define L2CAP_EVENT_CHANNEL_CLOSED                         0x71

// data: event (8), len(8), address(48), handle (16), psm (16), local_cid(16), remote_cid (16) 
#define L2CAP_EVENT_INCOMING_CONNECTION                    0x72

// data: event(8), len(8), handle(16)
#define L2CAP_EVENT_TIMEOUT_CHECK                          0x73

// data: event(8), len(8), local_cid(16), credits(8)
#define L2CAP_EVENT_CREDITS                                0x74

// data: event(8), len(8), status (8), psm (16)
#define L2CAP_EVENT_SERVICE_REGISTERED                     0x75

// data: event(8), len(8), handle(16), interval min(16), interval max(16), latency(16), timeout multiplier(16)
#define L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_REQUEST    0x76

// data: event(8), len(8), handle(16), result (16) (0 == ok, 1 == fail)
#define L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE   0x77

/**
 * @format 2
 * @param local_cid
 */
#define L2CAP_EVENT_CAN_SEND_NOW                           0x78
// RFCOMM EVENTS
/**
 * @format 1B2122
 * @param status
 * @param bd_addr
 * @param con_handle
 * @param server_channel
 * @param rfcomm_cid
 * @param max_frame_size
 */
#define RFCOMM_EVENT_OPEN_CHANNEL_COMPLETE                 0x80

/**
 * @format 2
 * @param rfcomm_cid
 */
#define RFCOMM_EVENT_CHANNEL_CLOSED                        0x81

/**
 * @format B12
 * @param bd_addr
 * @param server_channel
 * @param rfcomm_cid
 */
#define RFCOMM_EVENT_INCOMING_CONNECTION                   0x82

/**
 * @format 21
 * @param rfcomm_cid
 * @param line_status
 */
#define RFCOMM_EVENT_REMOTE_LINE_STATUS                    0x83
    
/**
 * @format 21
 * @param rfcomm_cid
 * @param credits
 */
#define RFCOMM_EVENT_CREDITS                               0x84

/**
 * @format 11
 * @param status
 * @param channel_id
 */
#define RFCOMM_EVENT_SERVICE_REGISTERED                    0x85
    
/**
 * @format 11
 * @param status
 * @param server_channel_id
 */
#define RFCOMM_EVENT_PERSISTENT_CHANNEL                    0x86
    
// data: event (8), len(8), rfcomm_cid (16), modem status (8)

/**
 * @format 21
 * @param rfcomm_cid
 * @param modem_status
 */
#define RFCOMM_EVENT_REMOTE_MODEM_STATUS                   0x87

// data: event (8), len(8), rfcomm_cid (16), rpn_data_t (67)
 /**
  * TODO: format for variable data
  * @param rfcomm_cid
  * @param rpn_data
  */
#define RFCOMM_EVENT_PORT_CONFIGURATION                    0x88

    
// data: event(8), len(8), status(8), service_record_handle(32)
 /**
  * @format 14
  * @param status
  * @param service_record_handle
  */
#define SDP_SERVICE_REGISTERED                             0x90

// data: event(8), len(8), status(8)
/**
 * @format 1
 * @param status
 */
#define SDP_QUERY_COMPLETE                                 0x91 

// data: event(8), len(8), rfcomm channel(8), name(var)
/**
 * @format 1T
 * @param rfcomm_channel
 * @param name
 * @brief SDP_QUERY_RFCOMM_SERVICE 0x92
 */
#define SDP_QUERY_RFCOMM_SERVICE                           0x92

// data: event(8), len(8), record nr(16), attribute id(16), attribute value(var)
/**
 * TODO: format for variable data
 * @param record_nr
 * @param attribute_id
 * @param attribute_value
 */
#define SDP_QUERY_ATTRIBUTE_VALUE                          0x93

// not provided by daemon, only used for internal testing
#define SDP_QUERY_SERVICE_RECORD_HANDLE                    0x94

/**
 * @format H1
 * @param handle
 * @param status
 */
#define GATT_QUERY_COMPLETE                                0xA0

/**
 * @format HX
 * @param handle
 * @param service
 */
#define GATT_SERVICE_QUERY_RESULT                          0xA1

/**
 * @format HY
 * @param handle
 * @param characteristic
 */
#define GATT_CHARACTERISTIC_QUERY_RESULT                   0xA2

/**
 * @format HX
 * @param handle
 * @param service
 */
#define GATT_INCLUDED_SERVICE_QUERY_RESULT                 0xA3

/**
 * @format HZ
 * @param handle
 * @param characteristic_descriptor
 */
#define GATT_ALL_CHARACTERISTIC_DESCRIPTORS_QUERY_RESULT   0xA4

/**
 * @format H2LV
 * @param handle
 * @param value_handle
 * @param value_length
 * @param value
 */
#define GATT_CHARACTERISTIC_VALUE_QUERY_RESULT             0xA5

/**
 * @format H2LV
 * @param handle
 * @param value_handle
 * @param value_length
 * @param value
 */
#define GATT_LONG_CHARACTERISTIC_VALUE_QUERY_RESULT        0xA6

/**
 * @format H2LV
 * @param handle
 * @param value_handle
 * @param value_length
 * @param value
 */
#define GATT_NOTIFICATION                                  0xA7

/**
 * @format H2LV
 * @param handle
 * @param value_handle
 * @param value_length
 * @param value
 */
#define GATT_INDICATION                                    0xA8

#define GATT_CHARACTERISTIC_DESCRIPTOR_QUERY_RESULT        0xA9
#define GATT_LONG_CHARACTERISTIC_DESCRIPTOR_QUERY_RESULT   0xAA

/** 
 * @format H2
 * @param handle
 * @param MTU
 */    
#define GATT_MTU										   0xAB

/** 
 * @format H2
 * @param handle
 * @param MTU
 */    
#define ATT_MTU_EXCHANGE_COMPLETE						   0xB5

// data: event(8), len(8), status (8), hci_handle (16), attribute_handle (16)
#define ATT_HANDLE_VALUE_INDICATION_COMPLETE               0xB6


// data: event(8), len(8), status (8), bnep service uuid (16) 
#define BNEP_EVENT_SERVICE_REGISTERED                      0xC0

// data: event(8), len(8), status (8), bnep source uuid (16), bnep destination uuid (16), mtu (16), remote_address (48) 
#define BNEP_EVENT_OPEN_CHANNEL_COMPLETE                   0xC1

// data: event(8), len(8), status (8), bnep source uuid (16), bnep destination uuid (16), mtu (16), remote_address (48) 
#define BNEP_EVENT_INCOMING_CONNECTION                     0xC2

// data: event(8), len(8), bnep source uuid (16), bnep destination uuid (16), remote_address (48) 
#define BNEP_EVENT_CHANNEL_CLOSED                          0xC3

// data: event(8), len(8), bnep source uuid (16), bnep destination uuid (16), remote_address (48), channel state (8)
#define BNEP_EVENT_CHANNEL_TIMEOUT                         0xC4    
    
// data: event(8), len(8)
#define BNEP_EVENT_READY_TO_SEND                           0xC5

// data: event(8), address_type(8), address (48), [number(32)]
 /**
  * @format H1B
  * @param handle
  * @param addr_type
  * @param address
  */
#define SM_EVENT_JUST_WORKS_REQUEST                              0xD0

 /**
  * @format H1B
  * @param handle
  * @param addr_type
  * @param address
  */
#define SM_EVENT_JUST_WORKS_CANCEL                               0xD1 

 /**
  * @format H1B4
  * @param handle
  * @param addr_type
  * @param address
  * @param passkey
  */
#define SM_EVENT_PASSKEY_DISPLAY_NUMBER                          0xD2

 /**
  * @format H1B
  * @param handle
  * @param addr_type
  * @param address
  */
#define SM_EVENT_PASSKEY_DISPLAY_CANCEL                          0xD3

 /**
  * @format H1B421
  * @param handle
  * @param addr_type
  * @param address
  */
#define SM_EVENT_PASSKEY_INPUT_NUMBER                            0xD4

 /**
  * @format H1B
  * @param handle
  * @param addr_type
  * @param address
  */
#define SM_EVENT_PASSKEY_INPUT_CANCEL                            0xD5

 /**
  * @format H1B4
  * @param handle
  * @param addr_type
  * @param address
  * @param passkey
  */
#define SM_EVENT_NUMERIC_COMPARISON_REQUEST                      0xD6

 /**
  * @format H1B4
  * @param handle
  * @param addr_type
  * @param address
  */
#define SM_EVENT_NUMERIC_COMPARISON_CANCEL                       0xD7

 /**
  * @format H1B
  * @param handle
  * @param addr_type
  * @param address
  */
#define SM_EVENT_IDENTITY_RESOLVING_STARTED                      0xD8

 /**
  * @format H1B
  * @param handle
  * @param addr_type
  * @param address
  */
#define SM_EVENT_IDENTITY_RESOLVING_FAILED                       0xD9

 /**
  * @format H1B2
  * @param handle
  * @param addr_type
  * @param address
  * @param le_device_db_index
  */
#define SM_EVENT_IDENTITY_RESOLVING_SUCCEEDED                    0xDA

 /**
  * @format H1B
  * @param handle
  * @param addr_type
  * @param address
  */
#define SM_EVENT_AUTHORIZATION_REQUEST                           0xDB

 /**
  * @format H1B1
  * @param handle
  * @param addr_type
  * @param address
  * @param authorization_result
  */
#define SM_EVENT_AUTHORIZATION_RESULT                            0xDC

 /**
  * @format H1
  * @param handle
  * @param action see SM_KEYPRESS_*
  */
#define SM_EVENT_KEYPRESS_NOTIFICATION                           0xDD

// GAP

// data: event(8), len(8), hci_handle (16), security_level (8)
#define GAP_SECURITY_LEVEL                                 0xE0

// data: event(8), len(8), status (8), bd_addr(48)
#define GAP_DEDICATED_BONDING_COMPLETED                    0xE1

/**
 * @format 11B1JV
 * @param advertising_event_type
 * @param address_type
 * @param address
 * @param rssi
 * @param data_length
 * @param data
 */
#define GAP_LE_ADVERTISING_REPORT                          0xE2

#define HCI_EVENT_HSP_META                                 0xE8

#define HSP_SUBEVENT_AUDIO_CONNECTION_COMPLETE             0x01
#define HSP_SUBEVENT_AUDIO_DISCONNECTION_COMPLETE          0x02
#define HSP_SUBEVENT_MICROPHONE_GAIN_CHANGED               0x03
#define HSP_SUBEVENT_SPEAKER_GAIN_CHANGED                  0x04
#define HSP_SUBEVENT_HS_COMMAND                            0x05
#define HSP_SUBEVENT_AG_INDICATION                         0x06
#define HSP_SUBEVENT_ERROR                                 0x07
#define HSP_SUBEVENT_RING                                  0x08


// ANCS Client
#define ANCS_CLIENT_CONNECTED                              0xF0
#define ANCS_CLIENT_NOTIFICATION                           0xF1
#define ANCS_CLIENT_DISCONNECTED                           0xF2



// #define HCI_EVENT_HFP_META                                 0xxx
// #define HCI_EVENT_GATT_META                                0xxx
// #define HCI_EVENT_SDP_META                                 0xxx
// #define HCI_EVENT_ANCS_META                                0xxx
// #define HCI_EVENT_SM_META                                  0xxx
// #define HCI_EVENT_GAP_META                                 0xxx
// #define HCI_EVENT_BNEP_META                                0xxx
// #define HCI_EVENT_PAN_META                                 0xxx


#define HCI_EVENT_VENDOR_SPECIFIC                          0xFF

//
// Error Codes
//

// from Bluetooth Core Specification
#define ERROR_CODE_UNKNOWN_CONNECTION_IDENTIFIER           0x02
#define ERROR_CODE_AUTHENTICATION_FAILURE				   0x05
#define ERROR_CODE_MEMORY_CAPACITY_EXCEEDED	    		   0x07
#define ERROR_CODE_COMMAND_DISALLOWED                      0x0C
#define ERROR_CODE_PAIRING_NOT_ALLOWED                     0x18
#define ERROR_CODE_INSUFFICIENT_SECURITY                   0x2F
 
// last error code in 2.1 is 0x38 - we start with 0x50 for BTstack errors
#define BTSTACK_CONNECTION_TO_BTDAEMON_FAILED              0x50
#define BTSTACK_ACTIVATION_FAILED_SYSTEM_BLUETOOTH         0x51
#define BTSTACK_ACTIVATION_POWERON_FAILED                  0x52
#define BTSTACK_ACTIVATION_FAILED_UNKNOWN                  0x53
#define BTSTACK_NOT_ACTIVATED                              0x54
#define BTSTACK_BUSY                                       0x55
#define BTSTACK_MEMORY_ALLOC_FAILED                        0x56
#define BTSTACK_ACL_BUFFERS_FULL                           0x57

// l2cap errors - enumeration by the command that created them
#define L2CAP_COMMAND_REJECT_REASON_COMMAND_NOT_UNDERSTOOD 0x60
#define L2CAP_COMMAND_REJECT_REASON_SIGNALING_MTU_EXCEEDED 0x61
#define L2CAP_COMMAND_REJECT_REASON_INVALID_CID_IN_REQUEST 0x62

#define L2CAP_CONNECTION_RESPONSE_RESULT_SUCCESSFUL        0x63
#define L2CAP_CONNECTION_RESPONSE_RESULT_PENDING           0x64
#define L2CAP_CONNECTION_RESPONSE_RESULT_REFUSED_PSM       0x65
#define L2CAP_CONNECTION_RESPONSE_RESULT_REFUSED_SECURITY  0x66
#define L2CAP_CONNECTION_RESPONSE_RESULT_REFUSED_RESOURCES 0x67
#define L2CAP_CONNECTION_RESPONSE_RESULT_RTX_TIMEOUT       0x68

#define L2CAP_SERVICE_ALREADY_REGISTERED                   0x69
#define L2CAP_DATA_LEN_EXCEEDS_REMOTE_MTU                  0x6A
    
#define RFCOMM_MULTIPLEXER_STOPPED                         0x70
#define RFCOMM_CHANNEL_ALREADY_REGISTERED                  0x71
#define RFCOMM_NO_OUTGOING_CREDITS                         0x72
#define RFCOMM_AGGREGATE_FLOW_OFF                          0x73
#define RFCOMM_DATA_LEN_EXCEEDS_MTU                        0x74

#define SDP_HANDLE_ALREADY_REGISTERED                      0x80
#define SDP_QUERY_INCOMPLETE                               0x81
#define SDP_SERVICE_NOT_FOUND                              0x82
 
#define ATT_HANDLE_VALUE_INDICATION_IN_PORGRESS            0x90 
#define ATT_HANDLE_VALUE_INDICATION_TIMEOUT                0x91

#define GATT_CLIENT_NOT_CONNECTED                          0x93
#define GATT_CLIENT_BUSY                                   0x94

#define BNEP_SERVICE_ALREADY_REGISTERED                    0xA0
#define BNEP_CHANNEL_NOT_CONNECTED                         0xA1
#define BNEP_DATA_LEN_EXCEEDS_MTU                          0xA2
#endif
