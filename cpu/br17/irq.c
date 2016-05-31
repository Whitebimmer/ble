#include "hwi.h"
#include "init.h"



static void ble_irq_handler()
{
	generic_irq_handler(IRQ_BLE);

	ILAT1_CLR |= BIT(IRQ_BLE-32);
}
REG_INIT_HANDLE(ble_irq_handler);




static void irq_handler_init()
{
	puts("irq_handler_init\n");
	INTALL_HWI(IRQ_BLE, ble_irq_handler, 7);
}
early_initcall(irq_handler_init);



