#include "interrupt.h"

struct irq_dec{
	u8 irq;
	u16 flag;
	struct list_head entry;
	void (*handler)(void *);
	void *data;
};

static struct irq_dec decs[8];
static struct list_head head;

int request_irq(int irq, void (*handler)(void *), int flag, void *data)
{
	int i;
	static struct irq_dec  *dec;

	for (i=0; i<sizeof(decs); i++)
	{
		dec = &decs[i];
		if (dec->irq == 0xff){
			dec->irq = irq;
			dec->flag = flag;
			dec->handler = handler;
			dec->data = data;
			if (flag & IRQF_SHARED){
				list_add_tail(&dec->entry, &head);
			} else {
				list_add(&dec->entry, &head);
			}
			return 0;
		}
	}

	return -1;
}

void generic_irq_handler(int irq)
{
	static struct irq_dec  *dec;

	list_for_each_entry(dec, &head, entry){
		if (dec->irq == irq){
			dec->handler(dec->data);
			if (!(dec->flag & IRQF_SHARED)){
				return;
			}
		}
	}
}


void interrupt_init()
{
	int i;

	INIT_LIST_HEAD(&head);

	for (i=0; i<ARRAY_SIZE(decs); i++) {
		decs[i].irq = 0xff;
	}
}

