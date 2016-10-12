#include "ble/btstack_run_loop.h"

#if 0
static struct list_head timer_head = LIST_HEAD_INIT(timer_head);

void btstack_timer_remove(timer_source_t *timer)
{

}

void btstack_timer_register(timer_source_t *timer,
	   	u32 msecs, void (*process)(timer_source_t *timer))
{

}

static struct list_head run_loop_head = LIST_HEAD_INIT(run_loop_head);
void btstack_run_loop_init()
{
	INIT_LIST_HEAD(&timer_head);
	INIT_LIST_HEAD(&run_loop_head);
}

void btstack_run_loop_register(data_source_t *loop, 
		void *(process)(data_source_t*))
{
	__run_loop_register(&run_loop_head, loop, process);
}

void btstack_run_loop_schedule()
{
	__run_loop_schedule(&run_loop_head);
}
#endif


