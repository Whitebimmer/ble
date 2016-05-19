#include "ble/hci.h"
#include "ble/hci_transport.h"
#include "thread.h"
#include "ble/ble_h4_transport.h"


struct h4_memb{
	void (*packet_handler)(int type, u8 *packet, u16 size);
	struct thread h4_thread;
};


static struct h4_memb h4 sec(.btmem_highly_available);

static void h4_packet_handler(int type, u8 *packet, int size)
{
	h4.packet_handler(type, packet, size);
}
REGISTER_H4_HOST(h4_packet_handler);

#if 0
static void h4_thread_process(struct thread *th)
{
	int awake = 0;

	awake += hci_bottom_upload(HCI_EVENT_PACKET, h4_packet_handler);

	awake += hci_bottom_upload(HCI_ACL_DATA_PACKET, h4_packet_handler);

	if (awake){
		thread_suspend(&h4.h4_thread, 0);
	}
}
#endif

static int h4_open(void *transport_config)
{
	//thread_create(&h4.h4_thread, h4_thread_process, 0);

	puts("h4 open\n");
	return 0;
}

static int h4_close(void *transport_config)
{
    return 0;
}


static int h4_send_packet(u8 packet_type, u8 * packet, int len)
{
	ble_h4_send_packet(packet_type, packet, len);

    /* printf_buf(packet, len); */
	/* puts("h4_send\n"); */
	return 0;
}

static void h4_register_packet_handler(void (*handler)(u8 type, u8 *packet, u16 size))
{
    h4.packet_handler = handler;
}

//void h4_upload_wakeup()
//{
//	thread_resume(&h4.h4_thread);
//}

static const char * h4_get_transport_name(void)
{
    return "H4";
}

static const hci_transport_t transport={
    .open                          = h4_open,
    .close                         = h4_close,
    .send_packet                   = h4_send_packet,
    .register_packet_handler       = h4_register_packet_handler,
    .get_transport_name            = h4_get_transport_name,
    .set_baudrate                  = NULL,
    .can_send_packet_now           = NULL,
};


hci_transport_t * ble_hci_transport_h4_instance()
{
	return	(hci_transport_t *)&transport;
}








