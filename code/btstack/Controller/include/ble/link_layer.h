#ifndef LINK_LAYER_H
#define LINK_LAYER_H


#include "typedef.h"
#include "list.h"
#include "ble_interface.h"
#include "sys_timer.h"

//--------------LL command
#define LL_CONNECTION_UPDATE_REQ 	0x00
#define LL_CHANNEL_MAP_REQ 			0x01
#define LL_TERMINATE_IND 			0x02
#define LL_ENC_REQ 					0x03
#define LL_ENC_RSP 					0x04
#define LL_START_ENC_REQ 			0x05
#define LL_START_ENC_RSP 			0x06
#define LL_UNKNOWN_RSP 				0x07
#define LL_FEATURE_REQ 				0x08
#define LL_FEATURE_RSP 				0x09
#define LL_PAUSE_ENC_REQ 			0x0A
#define LL_PAUSE_ENC_RSP 			0x0B
#define LL_VERSION_IND 				0x0C
#define LL_REJECT_IND 				0x0D
#define LL_SLAVE_FEATURE_REQ        0x0E
#define LL_CONNECTION_PARAM_REQ     0x0F
#define LL_CONNECTION_PARAM_RSP     0x10 
#define LL_REJECT_IND_EXT           0x11 
#define LL_PING_REQ                 0x12 
#define LL_PING_RSP                 0x13 
#define LL_LENGTH_REQ               0x14 
#define LL_LENGTH_RSP               0x15 
//--------------LE command

//rx tx type ADV_PDU
#define ADV_IND            0       //37
#define ADV_DIRECT_IND     1       //12
#define ADV_NONCONN_IND    2       //37
#define ADV_SCAN_IND       6       //37
#define SCAN_REQ           3       //12
#define SCAN_RSP           4       //37
#define CONNECT_REQ        5       //34


#define LL_RESERVED             0x0
#define LL_DATA_PDU_CONTINUE    0x1
#define LL_DATA_PDU_START       0x2
#define LL_CONTROL_PDU          0x3
#define LL_DATA_PDU_SN          0x8
#define LL_DATA_PDU_CRC         0x9

enum {
	LL_STANDBY = 0,
	LL_INITIATING,
	LL_ADVERTISING,
	LL_SCANNING,
	LL_CONNECTION_CREATE,
	LL_CONNECTION_ESTABLISHED,
	LL_DISCONNECT,
};


/*
 *-------------------LE FEATURE SUPPORT
 */
#define LE_ENCRYPTION                       BIT(0)
#define CONNECTION_PARAMETER_REQUEST        BIT(1)
#define EXTENDED_REJECT_INDICATION          BIT(2)
#define LE_SLAVE_INIT_FEATURES_EXCHANGE     BIT(3)
#define LE_PING                             BIT(4)
#define LE_DATA_PACKET_LENGTH_EXTENSION     BIT(5)
#define LL_PRIVACY                          BIT(6)
#define EXTENDED_SCANNER_FILTER_POLICIES    BIT(7)

struct le_feature_handler{
	struct list_head entry;
    u8 features_flag;
    void (*handler)(struct le_link *link, struct ble_rx *rx);
};

#define READ_BT_16(packet, pos) 		(packet[pos]|(packet[pos+1]<<8))


#define CONTROLLER_MAX_CMD_PAYLOAD      0x200
#define CONTROLLER_MAX_EVENT_PAYLOAD    0x200
#define CONTROLLER_MAX_RX_PAYLOAD       0x400

#define CONTROLLER_MAX_TOTAL_PAYLOAD    (CONTROLLER_MAX_CMD_PAYLOAD + \
                                        CONTROLLER_MAX_EVENT_PAYLOAD + \
                                        CONTROLLER_MAX_RX_PAYLOAD)

enum
{
    /* NO_EVENTS_SPECIFIED                                 = , */
    /* INQUIRY_COMPLETE_EVENT                              = , */
    /* INQUIRY_RESULT_EVENT                                = , */
    /* CONNECTION_COMPLETE_EVENT                           = , */
    /* CONNECTION_REQUEST_EVENT                            = , */
    /* DISCONNECTION_COMPLETE_EVENT                        = , */
    /* AUTHENTICATION_COMPLETE_EVENT                       = , */
    /* REMOTE_NAME_REQUEST_COMPLETE_EVENT                  = , */
    /* ENCRYPTION_CHANGE_EVENT                             = , */
    /* CHANGE_CONNECTION_LINK_KEY_COMPLETE_EVENT           = , */
    /* MASTER_LINK_KEY_COMPLETE_EVENT                      = , */
    /* READ_REMOTE_SUPPORTED_FEATURES_COMPLETE_EVENT       = , */
    /* READ_REMOTE_VERSION_INFORMATION_COMPLETE_EVENT      = , */
    /* QOS_SETUP_COMPLETE_EVENT                            = , */
    /* RESERVED                                            = , */
    /* RESERVED                                            = , */
    /* HARDWARE_ERROR_EVENT                                = , */
    /* FLUSH_OCCURRED_EVENT                                = , */
    /* ROLE_CHANGE_EVENT                                   = , */
    /* RESERVED                                            = , */
    /* MODE_CHANGE_EVENT                                   = , */
    /* RETURN_LINK_KEYS_EVENT                              = , */
    /* PIN_CODE_REQUEST_EVENT                              = , */
    /* LINK_KEY_REQUEST_EVENT                              = , */
    /* LINK_KEY_NOTIFICATION_EVENT                         = , */
    /* LOOPBACK_COMMAND_EVENT                              = , */
    /* DATA_BUFFER_OVERFLOW_EVENT                          = , */
    /* MAX_SLOTS_CHANGE_EVENT                              = , */
    /* READ_CLOCK_OFFSET_COMPLETE_EVENT                    = , */
    /* CONNECTION_PACKET_TYPE_CHANGED_EVENT                = , */
    /* QOS_VIOLATION_EVENT                                 = , */
    /* PAGE_SCAN_MODE_CHANGE_EVENT                         = , */
    /* PAGE_SCAN_REPETITION_MODE_CHANGE_EVENT              = , */
    /* FLOW_SPECIFICATION_COMPLETE_EVENT                   = , */
    /* INQUIRY_RESULT_WITH_RSSI_EVENT                      = , */
    /* READ_REMOTE_EXTENDED_FEATURES_COMPLETE_EVENT        = , */
    /* RESERVED                                            = , */
    /* RESERVED                                            = , */
    /* RESERVED                                            = , */
    /* RESERVED                                            = , */
    /* RESERVED                                            = , */
    /* RESERVED                                            = , */
    /* RESERVED                                            = , */
    /* RESERVED                                            = , */
    /* SYNCHRONOUS_CONNECTION_COMPLETE_EVENT               = , */
    /* SYNCHRONOUS_CONNECTION_CHANGED_EVENT                = , */
    /* SNIFF_SUBRATING_EVENT                               = , */
    /* EXTENDED_INQUIRY_RESULT_EVENT                       = , */
    /* ENCRYPTION_KEY_REFRESH_COMPLETE_EVENT               = , */
    /* IO_CAPABILITY_REQUEST_EVENT                         = , */
    /* IO_CAPABILITY_REQUEST_REPLY_EVENT                   = , */
    /* USER_CONFIRMATION_REQUEST_EVENT                     = , */
    /* USER_PASSKEY_REQUEST_EVENT                          = , */
    /* REMOTE_OOB_DATA_REQUEST_EVENT                       = , */
    /* SIMPLE_PAIRING_COMPLETE_EVENT                       = , */
    /* RESERVED                                            = , */
    /* LINK_SUPERVISION_TIMEOUT_CHANGED_EVENT              = , */
    /* ENHANCED_FLUSH_COMPLETE_EVENT                       = , */
    /* RESERVED                                            = , */
    /* USER_PASSKEY_NOTIFICATION_EVENT                     = , */
    /* KEYPRESS_NOTIFICATION_EVENT                         = , */
    /* REMOTE_HOST_SUPPORTED_FEATURES_NOTIFICATION_EVENT   = , */
    /* LE_META_EVENT                                       = , */
    /* RESERVED_FOR_FUTURE_USE                             = , */
    /* DEFAULT */
    LE_META_EVENT = 61,
}EVENT_MASK;



/********************************************************************************/
/*
 *                  HCI LE EVENT META
 */
enum
{
    LE_CONNECTION_COMPLETE_EVENT                   = 1, 
    LE_ADVERTISING_REPORT_EVENT                    = 2,
    LE_CONNECTION_UPDATE_COMPLETE_EVENT            = 3,
    LE_READ_REMOTE_USED_FEATURES_COMPLETE_EVENT    = 4,
    LE_LONG_TERM_KEY_REQUEST_EVENT                 = 5,
    LE_REMOTE_CONNECTION_PARAMETER_REQUEST_EVENT   = 6,
    LE_DATA_LENGTH_CHANGE_EVENT                    = 7,
    LE_READ_LOCAL_P_256_PUBLIC_KEY_COMPLETE_EVENT  = 8,
    LE_GENERATE_DHKEY_COMPLETE_EVENT               = 9,
    LE_ENHANCED_CONNECTION_COMPLETE_EVENT          = 10,
    LE_DIRECT_ADVERTISING_REPORT_EVENT             = 11,
    DEFAULT                                        = 12,
    RESERVED_FOR_FUTURE_USE                        = 13, 
}LE_EVENT_MASK;


/********************************************************************************/
/*
 *                  Link Layer State Machine
 */
enum {
    SLAVE_CONNECTION_PARAMETER_REQUEST_STEPS = 0,     
    MASTER_CONNECTION_PARAMETER_REQUEST_STEPS,
    MASTER_CONNECTION_UPDATE_STEPS,
    SLAVE_CONNECTION_PARAMETER_RESPOND_STEPS,
    SLAVE_REJECT_EXT_STEPS,
    MASTER_CHANNEL_MAP_REQUEST_STEPS,
    MASTER_FEATURES_EXCHANGE_STEPS,
    SLAVE_FEATURES_EXCHANGE_STEPS,
    VERSION_IND_STEPS,
    START_ENCRYPTION_STEPS,
    RESTART_ENCRYPTION_STEPS,
    START_ENCRYPTION_REQ_STEPS,
    RESTART_ENCRYPTION_REQ_STEPS,
    SLAVE_REJECT_STEPS,
    DISCONNECT_STEPS,
    DATA_LENGTH_UPDATE_STEPS,
}LL_CONTROL_PROCEDURE;

struct le_addr{
	u8 addr_type;
	u8 addr[6];
};

struct data_length{
    u16 connMaxTxOctets;
    u16 connMaxRxOctets;

    u16 connRemoteMaxTxOctets;
    u16 connRemoteMaxRxOctets;

    u16 connEffectiveMaxTxOctets;
    u16 connEffectiveMaxRxOctets;

    u16 connMaxTxTime;
    u16 connMaxRxTime;

    u16 connRemoteMaxTxTime;
    u16 connRemoteMaxRxTime;

    u16 connEffectiveMaxTxTime;
    u16 connEffectiveMaxRxTime;
};

struct ll_data_pdu_length_read_only{

    u16 supportedMaxTxOctets;
    u16 supportedMaxTxTime;

    u16 supportedMaxRxOctets;
    u16 supportedMaxRxTime;
};

struct ll_data_pdu_length{
    u16 suggestedMaxTxOctets;
    u16 suggestedMaxTxTime;

    u16 connInitialMaxTxOctets;
    u16 connInitialMaxTxTime;

    struct ll_data_pdu_length_read_only *priv;
};

struct le_link{
	u8 state;
	u8 role;
	u8 master_clock_accuracy;
	u8 rssi;
    u8 last_opcode;
    u8 encrypt_enable;
	u8 disconnect;
	u16 interval;
	u16 handle;
	struct list_head entry;
	struct list_head rx_oneshot_head;
	struct list_head event_oneshot_head;
	u8 feature[8];
	u8 long_term_key[16];
	u8 skdm[64/8];
    u8 skds[64/8];
	u8 ivm[32/8];
    u8 ivs[32/8];
    struct le_addr local;
    struct le_addr peer;
	u8 send_buf[48];
	struct ble_adv adv;
    struct ble_scan scan;
	struct ble_conn conn;
    // struct ble_conn conn;
	void *hw;
    //supervision timeout
    struct sys_timer timeout;
    struct sys_timer adv_timeout;
    struct sys_timer response_timeout;    //control procedures response timeout Tprt
    
    //Data Length Update
    struct data_length pdu_len;
};


struct lc_handler {

    void (*rx_handler)(struct le_link *link, struct ble_rx *rx);

    void (*tx_handler)(struct le_link *link, struct ble_tx *tx);

    void (*event_handler)(u8 procedure, struct le_link *link, struct ble_rx *rx, u8 status)
};

struct ll_interface{
	void *(*init)(struct hci_parameter *);

	void *(*open)();

    void (*close)(int);

    void (*handler_register)(const struct ll_handler_t *);

    void (*white_list_init)();

    void (*white_list_add)(const u8 *);
    
    void (*white_list_remove)(const u8 *);

    void (*resolving_list_init)();

    void (*resolving_list_add)(const u8 *);
    
    void (*resolving_list_remove)(const u8 *);

    void (*read_peer_RPA)(const u8, const u8 *, u8 *);

    void (*read_local_RPA)(const u8, const u8 *, u8 *);

    void (*resolution_enable)();
    
    void (*set_RPA_timeout)(u16);

	void (*ioctrl)(int, ...);
};

#define REGISTER_LL_INTERFACE(ops) \
	const struct ll_interface *__ll_api \
			SEC(.ble) = &ops

extern const struct ll_interface *__ll_api;



void le_ll_push_control_data(u8 opcode, const u8 *param);

//channel
// typedef void (*rx_handler_t)(struct le_link *link, struct ble_rx *rx);
// typedef void (*tx_handler_t)(struct le_link *link, struct ble_tx *tx);

// #define REGISTER_LLP_ACL_RXCHANNEL(fn) \
	// const rx_handler_t llp_acl_rxchannel \
			// SEC(.ble) = fn

// #define REGISTER_LLP_ACL_TXCHANNEL(fn) \
	// const tx_handler_t llp_acl_txchannel \
			// SEC(.ble) = fn

// extern const rx_handler_t llp_acl_rxchannel;

// extern const tx_handler_t llp_acl_txchannel;

#endif

