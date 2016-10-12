#ifndef THREAD_H
#define THREAD_H

#include "typedef.h"
#include "list.h"


#define PRIORITY_NUM   4



struct thread {
	struct list_head entry;
	void (*fun)(struct thread *);
	u8 priority;
    //resume
    u8 resume_cnt;
    //suspend
    u8 suspend_timeout;
};



struct thread_interface{
	int (*os_create)(void (*fun)(u8), u8);
	int (*os_delete)(u8);
	int (*os_suspend)(u8, u8);
	int (*os_resume)(u8);
};


void thread_init(const struct thread_interface *instance);


int thread_create(struct thread *th, void (*fun)(struct thread *), u8 priority);


void thread_suspend(struct thread *th, int timeout);


void thread_resume(struct thread *th);


int thread_delete(struct thread *th);




#endif

