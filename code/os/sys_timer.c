#include "sys_timer.h"
#include "jiffies.h"
#include "thread.h"


static volatile struct list_head timer_head sec(.btmem_highly_available);
static struct thread timer_thread;

static bool __timer_is_listed(struct sys_timer *timer)
{
	struct sys_timer *p;

	list_for_each_entry(p, &timer_head, entry){
		if (p == timer){
            return TRUE;
		}
	}
    return FALSE;
}

static void __timer_insert(struct sys_timer *timer)
{
	struct sys_timer *p;

	list_for_each_entry(p, &timer_head, entry){
		if (p == timer){
			list_del(&p->entry);
			break;
		}
	}

	list_for_each_entry(p, &timer_head, entry){
		if (p->jiffies > timer->jiffies){
			__list_add(&timer->entry, p->entry.prev, &p->entry);
			return;
		}
	}

	list_add_tail(&timer->entry, &timer_head);
}


void sys_timer_register(struct sys_timer *timer, u32 msec,
		void (*fun)(struct sys_timer *timer))
{
	CPU_SR_ALLOC();

	CPU_INT_DIS();

	timer->fun = fun;
	timer->jiffies = jiffies + msecs_to_jiffies(msec);
	__timer_insert(timer);

	CPU_INT_EN();
}

void sys_timer_reset(struct sys_timer *timer, u32 msec)
{
	CPU_SR_ALLOC();

	CPU_INT_DIS();
	timer->jiffies = jiffies + msecs_to_jiffies(msec);
	__timer_insert(timer);
	CPU_INT_EN();
}


void sys_timer_remove(struct sys_timer *timer)
{
	CPU_SR_ALLOC();

	CPU_INT_DIS();
    /* printf("timeout->entry.prev %x\n", timer->entry.prev); */
    /* printf("timeout->entry.next %x\n", timer->entry.next); */
    if (__timer_is_listed(timer) == FALSE)
    {
        CPU_INT_EN();
        return;
    }
    ASSERT(timer->entry.prev, "%s\n", "timer del prev NULL\n");
    ASSERT(timer->entry.next, "%s\n", "timer del next NULL\n");
	list_del(&timer->entry);
	CPU_INT_EN();
}


void sys_timer_schedule()
{
	struct sys_timer *p, *n;

	jiffies++;

	list_for_each_entry_safe(p, n, &timer_head, entry){
		if (time_before(jiffies, p->jiffies)){
			break;
		}
		sys_timer_remove(p);
		p->fun(p);
	}
#if 0
	while(!list_empty(&timer_head))
	{
		/*putchar('x');*/
		p = list_first_entry(&timer_head, struct sys_timer, entry);
		if (time_after(jiffies, p->jiffies)){
			sys_timer_remove(p);
			p->fun(p);
			putchar('d');
		}
	}
#endif
}


void sys_timer_init()
{
	INIT_LIST_HEAD(&timer_head);
}

void sys_timer_set_user(struct sys_timer *timer, u32 user)
{
	timer->user = user;
}

u32 sys_timer_get_user(struct sys_timer *timer)
{
	return timer->user;
}

