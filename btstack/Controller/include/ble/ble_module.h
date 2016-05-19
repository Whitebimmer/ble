#ifndef BTSTACK_MODULE_H
#define BTSTACK_MODULE_H

#include "module.h"
#include "run_loop.h"
#include "jiffies.h"


MODULE_MEMB_BEGIN(ble)
	u8 will_sleep;
MODULE_MEMB_END(ble)



/*timer functions*/
#define timer_register(timer, msecs, fun) \
	do { \
		(timer)->jiffies =	jiffies + msecs_to_jiffies(msecs); \
		__timer_register(&__this_module()->timer_head, timer, fun); \
	}while(0)

#define timer_set(timer, msecs) \
	do { \
		(timer)->jiffies =	jiffies + msecs_to_jiffies(msecs); \
		__timer_set(&__this_module()->timer_head, timer); \
	}while(0)

#define timer_delete(timer) \
	__timer_delete(&(timer)->entry)

#define timer_delete_region(begin, end) \
	__timer_delete_region(&__this_module()->timer_head, begin, end);

#define timer_schedule() \
	__timer_schedule(&__this_module()->timer_head, jiffies);

#define timer_delete_all() \
	__timer_delete_all(&__this_module()->timer_head)


#if 0
/*run loop functions*/
#define run_loop_register(loop, run) \
    __run_loop_register(&__this_module()->run_loop_head, (loop), run);

#define run_loop_remove(loop) \
	__list_del_entry(&(loop)->entry);

#define run_loop_schedule() \
	do { \
		__this_module()->will_sleep = 1; \
		__run_loop_schedule(&__this_module()->run_loop_head); \
		if (__this_module()->will_sleep) { \
			__this_module()->sleep(); \
		} \
	}while(0)

#define run_loop_keep_awake() \
   __this_module()->will_sleep = 0;	

#endif

#endif

