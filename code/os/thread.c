#include "thread.h"
#include "stdio.h"



static struct list_head thread_head[PRIORITY_NUM];

static const struct thread_interface *thread_ins;

/*------------------------------------------------------------------
 *  multithread model in one process
 *
 *      process |-thread 1 -> thread 1.1 -> thread 1.2 
 *              |           
 *              |
 *              |--thread 2
 *              |
 *              |
 *              |---thread 3
 *              |
 *              |
 *              |----thread 4
 *              |
 *              |
 *
 *   operation :
 *              1.create priority number & add list
 *              2.delete priority number & del list
 *              3.suspend should wait whole process suspend
 *              4.resume one thread then resume whole process
 * 					
 *
 * -----------------------------------------------------------------*/

static void thread_schedule(u8 priority)
{
	struct thread *p, *n;
    //multithread 
	struct list_head *head= &thread_head[priority];

    int suspend_timeout;

    //run subthread list
	list_for_each_entry_safe(p, n, head, entry){
        //check is not suspend 
        if (p->resume_cnt)
        {
            p->fun(p);
        }
	}

    //manage process suspend
	list_for_each_entry(p, head, entry){
        if (p->resume_cnt && (p->suspend_timeout == 0))
        {
           //keep awake
           return; 
        }
        ASSERT(p->resume_cnt == 0, "%s\n", __func__);
        //find the close suspend_timeout
        if (suspend_timeout > p->suspend_timeout)
        {
            suspend_timeout = p->suspend_timeout;
        }
    }

    thread_ins->os_suspend(priority, suspend_timeout);
}

static bool thread_is_exsit(struct thread *th, u8 priority)
{
    struct thread *p;

	struct list_head *head= &thread_head[priority];

    list_for_each_entry(p, head, entry){
        if (p == th)
        {
            return TRUE;
        }
    }
    return FALSE;
}

int thread_create(struct thread *th, 
		void (*fun)(struct thread *), u8 priority)
{
	int err = 0;
	struct list_head *head;
    /* CPU_SR_ALLOC(); */

	ASSERT(priority < PRIORITY_NUM);

    if (thread_is_exsit(th, priority) == TRUE)
    {
        puts("thread is exsit!\n");
        return err;
    }

	head = &thread_head[priority];

	th->fun = fun;
	th->priority = priority;
    th->resume_cnt = 1;
    th->suspend_timeout = 0;

    //first entry link to head
	if (list_empty(head)){
		err = thread_ins->os_create(thread_schedule, priority);
	}

    /* CPU_INT_DIS(); */

    //etc entry
	if (!err){
		list_add_tail(&th->entry, head);
	}

    /* CPU_INT_EN(); */
	return err;
}

//should wait all priority
void thread_suspend(struct thread *th, int timeout)
{
    CPU_SR_ALLOC();

    CPU_INT_DIS();
    th->suspend_timeout = timeout;
    //suspend now
    if (timeout == 0)
    {
        /* th->resume_cnt = (th->resume_cnt) ? th->resume_cnt-1 : 0; */
        th->resume_cnt = 0;
    }
    CPU_INT_EN();
}


//once  priority
void thread_resume(struct thread *th)
{
    CPU_SR_ALLOC();

    CPU_INT_DIS();
	th->suspend_timeout = 0;
    /* th->resume_cnt = (th->resume_cnt == 255) ? 255 : th->resume_cnt+1; */
    th->resume_cnt = 1;

    thread_ins->os_resume(th->priority);
    CPU_INT_EN();
}

int thread_delete(struct thread *th)
{
	int err = 0;

	list_del(&th->entry);

	if (list_empty(&thread_head[th->priority])){
		err = thread_ins->os_delete(th->priority);
	}

	return err;
}


void thread_init(const struct thread_interface *instance)
{
	int i;

	ASSERT(instance != NULL);

    thread_ins = instance;

	for (i=0; i<PRIORITY_NUM; i++) {
		INIT_LIST_HEAD(&thread_head[i]);
	}
}

void thread_run(struct thread *th)
{


}






