#ifndef _ERROR_CODE_
#define _ERROR_CODE_



#define ERR_SUCCESS		                                                0x00
#define ERR_UNKNOWN_HCI_COMMAND		                                    0x01
#define ERR_UNKNOWN_CONNECTION_IDENTIFIER		                        0x02
#define ERR_HARDWARE_FAILURE		                                        0x03
#define ERR_PAGE_TIMEOUT		                                            0x04
#define ERR_AUTHENTICATION_FAILURE                                       0x05
#define ERR_PIN_OR_KEY_MISSING                                           0x06
#define ERR_MEMORY_CAPACITY_EXCEEDED                                     0x07
#define ERR_CONNECTION_TIMEOUT                                           0x08
#define ERR_CONNECTION_LIMIT_EXCEEDED                                    0x09
#define ERR_SYNCHRONOUS_CONNECTION_LIMIT_TO_A_DEVICE_EXCEEDED            0x0a
#define ERR_ACL_CONNECTION_ALREADY_EXISTS                                0x0b
#define ERR_CMD_DISALLOWED                                               0x0c
#define ERR_CONNECTION_REJECTDE_DUE_TO_LIMITED_RESOURCES                 0x0d
#define ERR_CONNECTION_REJECTDE_DUE_TO_SECURITY_REASONS                  0x0e
#define ERR_CONNECTION_REJECTDE_DUE_TO_UNACCEPTABLE_BD_ADDR              0x0f
#define ERR_CONNECTION_ACCEPT_TIMEOUT_EXCEEDED                           0x10
#define ERR_UNSUPPORTDE_FEATURE_OR_PARAMETER_VALUE                       0x11
#define ERR_INVALID_HCI_CMD_PARAMETERS                                   0x12
#define ERR_REMOTE_USER_TERMINATED_CONNECTION                            0x13
#define ERR_REMOTE_DEVICE_TERMINATED_CONNECTION_DUE_TO_LOW_RESOURCES     0x14
#define ERR_REMOTE_DEVICE_TERMINATED_CONNECTION_DUE_TO_POWER_OFF         0x15
#define ERR_CONNECTION_TERMINATED_BY_LOCAL_HOST                          0x16
#define ERR_REPEATED_ATTEMPTS                                            0x17
#define ERR_PAIRING_NOT_ALLOWED                                          0x18
#define ERR_UNKNOWN_LMP_PDU                                              0x19
#define ERR_UNSUPPORTED_REMOTE_LMP_FEATURE                               0x1a
#define ERR_SCO_OFFSET_REJECTED                                          0x1b
#define ERR_SCO_INTERVAL_REJECTED                                        0x1c
#define ERR_SCO_AIR_MODE_REJECTED                                        0x1d
#define ERR_INVALID_LMP_PARAMETERS                                       0x1e
#define ERR_UNSPECIFIED_ERROR                                            0x1f
#define ERR_UNSUPPORTED_LMP_PARAMETER_VALUE                              0x20
#define ERR_ROLE_CHANGE_NOT_ALLOWED                                      0x21
#define ERR_LMP_LL_RESPONSE_TIMEOUT                                      0x22
#define ERR_LMP_ERROR_TRANSACTION_COLLISION                              0x23
#define ERR_LMP_PDU_NOT_ALLOWED                                          0x24
#define ERR_ENCRYPTION_MODE_NOT_ACCEPTABLE                               0x25
#define ERR_LINK_KEY_CANNOT_BE_CHANGED                                   0x26
#define ERR_REQUESTED_QOS_NOT_SUPPORTED                                  0x27
#define ERR_INSTANT_PASSED                                               0x28
#define ERR_PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED                          0x29
#define ERR_DIFFERENT_TRANSACTION_COLLISION                              0x2a
#define ERR_RESERVED_0X2B                                                0x2b
#define ERR_QOS_UNACCEPTABLE_PARAMETER                                   0x2c
#define ERR_QOS_REJECTDE                                                 0x2d
#define ERR_CHANNEL_CLASSIFICATION_NOT_SUPPORTED                         0x2e
#define ERR_INSUFFICIENT_SECURITY                                        0x2f
#define ERR_PARAMETER_OUT_OF_MANDATORY_RANGE                             0x30
#define ERR_RESERVED_0X31                                                0x31
#define ERR_ROLE_SWITCH_PENDING                                          0x32
#define ERR_RESERVED_0X33                                                0x33
#define ERR_RESERVED_SLOT_VIOLATION                                      0x34
#define ERR_ROLE_SWITCH_FAILED                                           0x35
#define ERR_EXTENDED_INQUIRY_RESPONSE_TOO_LARGE                          0x36
#define ERR_SECURE_SIMPLE_PAIRING_NOT_SUPPORT_BY_HOST                    0x37
#define ERR_HOST_BUSY_PAIRING                                            0x38
#define ERR_CONNECTION_REJECTED_DUE_TO_NO_SUITABLE_CHANNEL_FOUND         0x39
#define ERR_CONTROLLER_BUSY                                              0x3a
#define ERR_UNACCEPTABLE_CONNECTION_INTERVAL                             0x3b
#define ERR_DIRECTED_ADVERTISING_TIMEOUT                                 0x3c
#define ERR_CONNECTION_TERMINATED_DUE_TO_MIC_FAILURE                     0x3d
#define ERR_CONNECTION_FAILED_TO_BE_ESTABLISHED                          0x3e
#define ERR_MAC_CONNECTION_FAILED                                        0x3f



#endif
