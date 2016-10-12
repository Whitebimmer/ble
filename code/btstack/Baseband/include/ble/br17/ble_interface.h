#ifndef BLE_DRIVER_H
#define BLE_DRIVER_H

#include "lbuf.h"





enum {
	BLE_SET_IV,
	BLE_SET_SKD,
	BLE_SET_LOCAL_ADDR,
	BLE_SET_PEER_ADDR,
	BLE_SET_INSTANT,
	BLE_SET_WINSIZE,
	BLE_SET_CONN_PARAM,
	BLE_CONN_UPDATE,
	BLE_CHANNEL_MAP,
    BLE_WHITE_LIST_ADDR,
    BLE_SET_SEND_ENCRYPT,
    BLE_SET_RECEIVE_ENCRYPT,
    BLE_SET_PRIVACY_ENABLE,
    BLE_SET_LOCAL_ADDR_RAM,
    BLE_SET_RPA_RESOLVE_RESULT,
    BLE_SET_TX_LENGTH,
    BLE_SET_RX_LENGTH,
};

struct ble_rx {
	u8 channel;
	u8 type;
	u8 llid;
	u8 nesn;

	u8 sn;
	u8 md;
	u8 len;
	u8 rxadd;

	u8 txadd;
	u8 res;
	u16 event_count;

	u8 data[0];
};


struct ble_tx {
	u8 type;
	u8 llid;
	u8 txadd;
	u8 rxadd;

	u8 sn;
	u8 md;
	u8 len;
	u8 res;

	u8 data[0];
};


// struct ble_conn_param{
	// u8 aa[4];
	// u8 crcinit[3];
	// u8 peer_addr_type;
	// u8 peer_addr[6];
	// u8 winsize;
	// u16 winoffset;
	// u16 interval;
	// u16 latency;
	// u16 timeout;
	// u16 widening;
	// u8 channel[5];
	// u8 hop;
// };

struct ble_adv {
	u16 interval;
    u16 pdu_interval;
    u8 pkt_cnt;
	u8 adv_type;
    u8 channel_map;
    u8 filter_policy;
	u8 adv_len;
	u8 scan_rsp_len;
	u8 adv_data[31];
	u8 scan_rsp_data[31];
};

struct ble_scan {
    u8 type;
	u16 interval;
    u16 window;
    u8 filter_policy;
    u8 filter_duplicates;
};

struct ble_conn_param{
    u8 aa[4];
    u8 crcinit[3];
    u8 winsize;
    u16 winoffset;
    u16 interval;
    u16 latency;
    u16 timeout;
    u16 widening;
    u8 channel[5];
    u8 hop;
};

struct ble_conn {
    u16 scan_interval;
    u16 scan_windows;
    u8 filter_policy;
    u8 own_addr_type;
    u8 peer_addr_type;
    struct ble_conn_param ll_data;
};



struct ble_handler {
    void (*rx_probe_handler)(void *, struct ble_rx *rx);
    void (*rx_post_handler)(void *, struct ble_rx *rx);
    void (*tx_probe_handler)(void *, struct ble_tx *tx);
	void (*event_handler)(void *, int event_cnt);
};

struct ble_operation{
	void (*init)();

	void *(*open)();

	void (*standby)(void *);

	void (*advertising)(void *, struct ble_adv *adv);

	void (*scanning)(void *, struct ble_scan *scan);

	void (*initiating)(void *, struct ble_conn *conn);

	void (*handler_register)(void *, void *, const struct ble_handler *);

	void (*send_packet)(void *, u8 type, u8 *buf, int len);

	void (*ioctrl)(void *, int, ...);

	void (*close)(void *);

    int (*get_id)(void *);

    void (*upload_data)();

    int (*upload_is_empty)();

    int (*get_conn_event)(void *);

	int (*is_init_enter_conn)(void *);
};


#define REGISTER_BLE_OPERATION(ops) \
	const struct ble_operation *__ble_ops \
			__attribute__((section(".ble"))) = &ops


extern const struct ble_operation *__ble_ops;



#define DEBUG_IO_1(x)    {PORTA_DIR &= ~BIT(x); PORTA_OUT |= BIT(x);}
#define DEBUG_IO_0(x)    {PORTA_DIR &= ~BIT(x); PORTA_OUT &= ~BIT(x);}

#define DEBUG_IOB_1(x)    {PORTB_DIR &= ~BIT(x); PORTB_OUT |= BIT(x);}
#define DEBUG_IOB_0(x)    {PORTB_DIR &= ~BIT(x); PORTB_OUT &= ~BIT(x);}




/***************************************************************************/

/********************************************************************************/
/*
 *                  HCI LE Command Parameter
 */

//--------------LE READ (R attr)
struct buffer_param
{
    u8 total_num_le_data_pkt;
	u16 buffer_size;
};

struct le_read_parameter{
    u8 features[8];
    u8 version[8];
    u8 versnr;
    u16 compld;
    u16 subversnr;

    struct buffer_param buf_param;

    u8 white_list_size;
    u8 transmit_power_level;
    u8 resolving_list_size;
};

//--------------LE SET (W attr)
struct white_list_parameter{
    u8 Address_Type;
    u8 Address[6];
}; 

struct white_list{
    struct white_list_parameter white_list_param;

	struct list_head entry;
    u16 cnt;
};

struct fragment_pkt{
	struct list_head entry;
    u8 cnt;
};

struct list_head pkt_list_head;

// The Resolving List IRK pairs shall be
// associated with a public or static device address known as the Identity
// Address.
struct resolving_list_parameter{
    u8 Peer_Identity_Address_Type;  
    u8 Peer_Identity_Address[6];
    u8 Peer_IRK[16];
    u8 Local_IRK[16];
};

struct resolving_list{
    struct resolving_list_parameter resolving_list_param; 

    struct list_head entry;

    u8 local_PRA[6];
    u8 peer_PRA[6];
};

struct advertising_param {
	u16 Advertising_Interval_Min;
	u16 Advertising_Interval_Max;
	u8 Advertising_Type;
	u8 Own_Address_Type;
	u8 Peer_Address_Type;
	u8 Peer_Address[6];
	u8 Advertising_Channel_Map;
	u8 Advertising_Filter_Policy;
}__attribute__((packed));

struct advertising_data  {
	u8 length;
	u8 data[31];
};



struct scan_parameter {
	u8 LE_Scan_Type;
	u16 LE_Scan_Interval;
	u16 LE_Scan_Window;
	u8 Own_Address_Type;
	u8 Scanning_Filter_Policy;
}__attribute__((packed));

struct scan_response{
	u8 length;
	u8 data[31];
};

struct connection_parameter {
	u16 le_scan_interval;
	u16 le_scan_window;
	u8 initiator_filter_policy;
	u8 peer_address_type;
	u8 peer_address[6];
	u8 own_address_type;
	u16 conn_interval_min;
	u16 conn_interval_max;
	u16 conn_latency;
	u16 supervision_timeout;
	u16 minimum_ce_length;
	u16 maximum_ce_length;
}__attribute__((packed));

struct connection_update_parameter {
    u16 connection_handle;
	u16 Conn_Interval_Min;
	u16 Conn_Interval_Max;
	u16 Conn_Latency;
	u16 Supervision_Timeout;
	u16 Minimum_CE_Length;
	u16 Maximum_CE_Length;
}__attribute__((packed));


struct start_encryption_parameter{
    u16 connection_handle;
    u8 random_number[8];
    u8 encrypted_diversifier[2];
	u8 long_term_key[16];
}__attribute__((packed));

struct long_term_key_request_reply_parameter{
    u16 connection_handle;
    u8 long_term_key[16];
}__attribute__((packed));

struct long_term_key_request_negative_reply_parameter{
    u16 connection_handle;
}__attribute__((packed));

struct host_channel_classification {
    u8 channel_map[5];
}__attribute__((packed));

struct scan_enable_parameter{
    u8 enable;
    u8 filter_duplicates;
}__attribute__((packed));

struct read_remote_used_features_paramter{
    u16 connection_handle;
}__attribute__((packed));

struct remote_connection_parameter_request_reply_parameter {
    u16 connection_handle;
	u16 Interval_Min;
	u16 Interval_Max;
	u16 Latency;
	u16 Timeout;
	u16 Minimum_CE_Length;
	u16 Maximum_CE_Length;
}__attribute__((packed));

struct remote_connection_parameter_request_negative_reply_parameter {
    u16 connection_handle;
    u8  reason;
}__attribute__((packed));

struct set_data_length_parameter {
    u16 connection_handle;
    u16 txoctets;
    u16 txtime;
}__attribute__((packed));

struct le_parameter{
	u8 event_mask[8];
	u8 random_address[6];
	u8 advertise_enable;
    u8 resolution_enable;
    
    u8 remote_features[8];
    u8 remote_versnr;
    u16 remote_compld;
    u16 remote_subversnr;
	struct advertising_param  	    adv_param;
	struct advertising_data  	    adv_data;
	struct scan_parameter 		    scan_param;
	struct scan_response 		    scan_resp_data;
    struct connection_parameter     conn_param;
    struct scan_enable_parameter    scan_enable_param;
    //LL control procedure
    struct connection_update_parameter *conn_update_param;
    struct host_channel_classification *channel_map_update_param;
    struct read_remote_used_features_paramter  *read_remote_used_features_param;
    struct start_encryption_parameter *start_encryption_param;
    struct long_term_key_request_reply_parameter *long_term_key_request_reply_param;
    struct long_term_key_request_negative_reply_parameter *long_term_key_request_negative_reply_param;
    struct remote_connection_parameter_request_reply_parameter *remote_conn_param_reply_param;
    struct remote_connection_parameter_request_negative_reply_parameter *remote_conn_param_negative_reply_param;
    struct set_data_length_parameter *set_data_length_param;

    //white list
	struct list_head white_list_head;
    //privacy - resolving list
    struct list_head resolving_list_head;
    //
    const struct le_read_parameter *priv;
};

/********************************************************************************/

/********************************************************************************/
/*
 *                  HCI Command Parameter
 */
//--------------HCI READ (R attr)
struct hci_read_parameter{
    u8 features[8];
    u8 public_addr[6];
};

struct read_remote_version_paramter{
    u16 connection_handle;
};

struct disconnect_paramter{
    u16 connection_handle;
    u8 reason;
// };
}__attribute__((packed));

//--------------HCI SET
struct hci_parameter{
    u8 event_mask[8];

    u8 static_addr[6];

    u8 public_addr[6];

    u8 secure_conn_host_support;

    struct read_remote_version_paramter *read_remote_version_param;
    struct disconnect_paramter *disconn_param;
    //
    const struct hci_read_parameter *priv;
};


/********************************************************************************/
/*
 *                  HCI EVENT Parameter
 */
struct advertising_report_parameter{
    u8 Subevent_Code;
    u8 Num_Reports;
    u8 Event_Type[1];
    u8 Address_Type[1];
    u8 Address[1];
    u8 Length[1];
    u8 Data[1];
    u8 RSSI[1];
};

struct le_event_parameter{
    struct advertising_report_parameter adv_report_param;
    
};

/********************************************************************************/

/********************************************************************************/
/*
 *                  Link Control Command
 */
#define HCI_DISCONNECT              0x6

/********************************************************************************/
/*
 *                  HCI LE COMMAND 
 */
#define  HCI_LE_SET_EVENT_MASK 							0x01 //Mandatory
#define  HCI_LE_READ_BUFFER_SIZE 						0x02 //Mandatory
#define  HCI_LE_READ_LOCAL_SUPPORT_FEATURES 			0x03 //Mandatory
#define  HCI_LE_SET_RANDOM_ADDRESS 						0x05
#define  HCI_LE_SET_ADVERTISING_PARAMETERS 				0x06
#define  HCI_LE_READ_ADVERTISING_CHANNEL_TX_POWER		0x07
#define  HCI_LE_SET_ADVERTISING_DATA 					0x08
#define  HCI_LE_SET_SCAN_RESPONSE_DATA 					0x09
#define  HCI_LE_SET_ADVERTISE_ENABLE 					0x0a
#define  HCI_LE_SET_SCAN_PARAMETERS 					0x0b
#define  HCI_LE_SET_SCAN_ENABLE 						0x0c
#define  HCI_LE_CREATE_CONNECTION 						0x0d
#define  HCI_LE_CREATE_CONNECTION_CANCEL 				0x0e
#define  HCI_LE_READ_WHIRTE_LIST_SIZE 					0x0f //Mandatory
#define  HCI_LE_CLEAR_WHIRTE_LIST 						0x10 //Mandatory
#define  HCI_LE_ADD_DEVICE_TO_WHIRTE_LIST 				0x11 //Mandatory
#define  HCI_LE_REMOVE_DEVICE_FROM_WHIRTE_LIST 			0x12 //Mandatory
#define  HCI_LE_CONNECTION_UPDATE 						0x13
#define  HCI_LE_SET_HOST_CHANNEL_CLASSIFICATION 		0x14
#define  HCI_LE_READ_CHANNEL_MAP 						0x15
#define  HCI_LE_READ_REMOTE_USED_FEATURES 				0x16
#define  HCI_LE_ENCRYPT 								0x17
#define  HCI_LE_RAND 									0x18
#define  HCI_LE_START_ENCRYPT 							0x19
#define  HCI_LE_LONG_TERM_KEY_REQUEST_REPLY 			0x1a
#define  HCI_LE_LONG_TERM_KEY_REQUEST_NAGATIVE_REPLY 	0x1b
#define  HCI_LE_READ_SUPPORTED_STATES 					0x1c //Mandatory
#define  HCI_LE_RECEIVER_TEST 							0x1d
#define  HCI_LE_TRANSMITTER_TEST 						0x1e
#define  HCI_LE_TEST_END 								0x1f //Mandatory 
#define  HCI_LE_REMOTE_CONNECTION_PARAMETER_REQUEST_REPLY           0X20
#define  HCI_LE_REMOTE_CONNECTION_PARAMETER_REQUEST_NEGATIVE_REPLY  0x21
#define  HCI_LE_SET_DATA_LENGTH                         0x22
#define  HCI_LE_READ_SUGGESTED_DEFAULT_DATA_LENGTH      0x23
#define  HCI_LE_WRITE_SUGGESTED_DEFAULT_DATA_LENGTH     0x24
#define  HCI_LE_READ_LOCAL_P256_PUBLIC_KEY              0x25
#define  HCI_LE_GENERATE_DHKEY                          0x26
#define  HCI_LE_ADD_DEVICE_TO_RESOLVING_LIST            0x27
#define  HCI_LE_REMOVE_DEVICE_FROM_RESOLVING_LIST       0x28
#define  HCI_LE_CLEAR_RESOLVING_LIST                    0x29
#define  HCI_LE_READ_RESOLVING_LIST_SIZE                0x2A
#define  HCI_LE_READ_PEER_RESOLVABLE_ADDRESS            0x2B
#define  HCI_LE_READ_LOCAL_RESOLVABLE_ADDRESS           0x2C
#define  HCI_LE_SET_ADDRESS_RESOLUTION_ENABLE           0x2D
#define  HCI_LE_SET_RESOLVABLE_PRIVATE_ADDRESS_TIMEOUT  0x2E
#define  HCI_LE_READ_MAXIMUM_DATA_LENGTH                0x2F


#define HCI_READ_LOCAL_SUPPORT_FEATURES     0x03
#define HCI_READ_BD_ADDR                    0x09

#define HCI_READ_REMOTE_VERSION_INFORMATION 0x1D
/********************************************************************************/
/*
 *                  Packet Type
 */
#define HCI_COMMAND_DATA_PACKET     0x01
#define HCI_ACL_DATA_PACKET         0x02
#define HCI_SCO_DATA_PACKET         0x03
#define HCI_EVENT_PACKET            0x04

/********************************************************************************/
/*
 *                  HCI EVENT 
 */
//Vol 2 Part E 7
enum
{
    HCI_EVENT_DISCONNECTION_COMPLETE = 0x05,
    HCI_EVENT_ENCRYPTION_CHANGE = 0x08,
    HCI_EVENT_READ_REMOTE_VERSION_INFORMATION_COMPLETE = 0x0C, 
    HCI_EVENT_COMMAND_COMPLETE = 0x0e,
    HCI_EVENT_COMMAND_STATUS = 0x0f,
    HCI_EVENT_ENCRYPTION_KEY_REFRESH_COMPLETE= 0x30,
    HCI_EVENT_LE_META  = 0x3e,
}HCI_EVENTS; 

/********************************************************************************/
/*
 *                  HCI ERROR CODES 
 */
enum{
    SUCCESS                                                                      = 0x00,
    UNKNOWN_HCI_COMMAND                                                          = 0x01,
    UNKNOWN_CONNECTION_IDENTIFIER                                                = 0x02,
    HARDWARE_FAILURE                                                             = 0x03,
    PAGE_TIMEOUT                                                                 = 0x04,
    AUTHENTICATION_FAILURE                                                       = 0x05,
    PIN_OR_KEY_MISSING                                                           = 0x06,
    MEMORY_CAPACITY_EXCEEDED                                                     = 0x07,
    CONNECTION_TIMEOUT                                                           = 0x08,
    CONNECTION_LIMIT_EXCEEDED                                                    = 0x09,
    SYNCHRONOUS_CONNECTION_LIMIT_TO_A_DEVICE_EXCEEDED                            = 0x0A,
    ACL_CONNECTION_ALREADY_EXISTS                                                = 0x0B,
    COMMAND_DISALLOWED                                                           = 0x0C,
    CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES                                 = 0x0D,
    CONNECTION_REJECTED_DUE_TO_SECURITY_REASONS                                  = 0x0E,
    CONNECTION_REJECTED_DUE_TO_UNACCEPTABLE_BD_ADDR                              = 0x0F,
    CONNECTION_ACCEPT_TIMEOUT_EXCEEDED                                           = 0x10,
    UNSUPPORTED_FEATURE_OR_PARAMETER_VALUE                                       = 0x11,
    INVALID_HCI_COMMAND_PARAMETERS                                               = 0x12,
    REMOTE_USER_TERMINATED_CONNECTION                                            = 0x13,
    REMOTE_DEVICE_TERMINATED_CONNECTION_DUE_TO_LOW_RESOURCES                     = 0x14,
    REMOTE_DEVICE_TERMINATED_CONNECTION_DUE_TO_POWER_OFF                         = 0x15,
    CONNECTION_TERMINATED_BY_LOCAL_HOST                                          = 0x16,
    REPEATED_ATTEMPTS                                                            = 0x17,
    PAIRING_NOT_ALLOWED                                                          = 0x18,
    UNKNOWN_LMP_PDU                                                              = 0x19,
    UNSUPPORTED_REMOTE_FEATURE_UNSUPPORTED_LMP_FEATURE                           = 0x1A,
    SCO_OFFSET_REJECTED                                                          = 0x1B,
    SCO_INTERVAL_REJECTED                                                        = 0x1C,
    SCO_AIR_MODE_REJECTED                                                        = 0x1D,
    INVALID_LMP_PARAMETERS_INVALID_LL_PARAMETERS                                 = 0x1E,
    UNSPECIFIED_ERROR                                                            = 0x1F,
    UNSUPPORTED_LMP_PARAMETER_VALUE_UNSUPPORTED_LL_PARAMETER_VALUE               = 0x20,
    ROLE_CHANGE_NOT_ALLOWED                                                      = 0x21,
    LMP_RESPONSE_TIMEOUT_LL_RESPONSE_TIMEOUT                                     = 0x22,
    LMP_ERROR_TRANSACTION_COLLISION                                              = 0x23,
    LMP_PDU_NOT_ALLOWED                                                          = 0x24,
    ENCRYPTION_MODE_NOT_ACCEPTABLE                                               = 0x25,
    LINK_KEY_CANNOT_BE_CHANGED                                                   = 0x26,
    REQUESTED_QOS_NOT_SUPPORTED                                                  = 0x27,
    INSTANT_PASSED                                                               = 0x28,
    PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED                                          = 0x29,
    DIFFERENT_TRANSACTION_COLLISION                                              = 0x2A,
    RESERVED                                                                     = 0x2B,
    QOS_UNACCEPTABLE_PARAMETER                                                   = 0x2C,
    QOS_REJECTED                                                                 = 0x2D,
    CHANNEL_CLASSIFICATION_NOT_SUPPORTED                                         = 0x2E,
    INSUFFICIENT_SECURITY                                                        = 0x2F,
    PARAMETER_OUT_OF_MANDATORY_RANGE                                             = 0x30,
    RESERVED_PAD1                                                                = 0x31,
    ROLE_SWITCH_PENDING                                                          = 0x32,
    RESERVED_PAD2                                                                = 0x33,
    RESERVED_SLOT_VIOLATION                                                      = 0x34,
    ROLE_SWITCH_FAILED                                                           = 0x35,
    EXTENDED_INQUIRY_RESPONSE_TOO_LARGE                                          = 0x36,
    SECURE_SIMPLE_PAIRING_NOT_SUPPORTED_BY_HOST                                  = 0x37,
    HOST_BUSY_PAIRING                                                            = 0x38,
    CONNECTION_REJECTED_DUE_TO_NO_SUITABLE_CHANNEL_FOUND                         = 0x39,
    CONTROLLER_BUSY                                                              = 0x3A,
    UNACCEPTABLE_CONNECTION_PARAMETERS                                           = 0x3B,
    DIRECTED_ADVERTISING_TIMEOUT                                                 = 0x3C,
    CONNECTION_TERMINATED_DUE_TO_MIC_FAILURE                                     = 0x3D,
    CONNECTION_FAILED_TO_BE_ESTABLISHED                                          = 0x3E,
    MAC_CONNECTION_FAILED                                                        = 0x3F,
    COARSE_CLOCK_ADJUSTMENT_REJECTED_BUT_WILL_TRY_TO_ADJUST_USING_CLOCK_DRAGGING = 0x40,
}ERROR_CODES_LIST;

#endif

