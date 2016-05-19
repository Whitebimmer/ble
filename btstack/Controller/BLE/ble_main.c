#include "typedef.h"
#include "ble/link_layer.h"




#ifndef BREDR 
const u8 sdp_bluetooth_base_uuid[] = { 0x00, 0x00, 0x00, 0x00, /* - */ 0x00, 0x00, /* - */ 0x10, 0x00, /* - */
    0x80, 0x00, /* - */ 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB };

void sdp_normalize_uuid(u8 *uuid, u32 shortUUID){
    memcpy(uuid, sdp_bluetooth_base_uuid, 16);
    net_store_32(uuid, 0, shortUUID);
}
#endif

void ble_main()
{
	hci_firmware_init();

//	le_test_init();

	puts("ble_main_exit\n");
}









