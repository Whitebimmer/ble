#include "init.h"
#include "hwi.h"
#include "platform_device.h"



struct resource ble_resource[2] = {
	[0] = {
		.start = IRQ_BLE, 
		.end = IRQ_BLE,
		.flags = IORESOURCE_IRQ, 
	},
	[1] = {
		.start = IRQ_BLE_DBG, 
		.end = IRQ_BLE_DBG,
		.flags = IORESOURCE_IRQ, 
	},
};

struct platform_device device_ble = {
	.name = "ble",
	.num_resources = ARRAY_SIZE(ble_resource),
	.resource = ble_resource,
};


const struct platform_device *bt16_devices[] = {
	&device_ble,
};


static void bt16_machine_init()
{
	printf("bt16_machine_init: %x\n", &device_ble);

	platform_add_devices(bt16_devices, ARRAY_SIZE(bt16_devices));
}
early_initcall(bt16_machine_init);

