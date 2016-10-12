#include "init.h"


#define do_initcall(a) \
	do{ \
		extern initcall_t * a##_begin[];  \
		extern initcall_t * a##_end[];  \
		__do_initcall(a##_begin, a##_end); \
	}while(0)


static void __do_initcall(initcall_t *begin[], initcall_t *end[])
{
	initcall_t *func;

	for (func=begin; func<end; func++) {
        /* printf("func : %x\n", func); */
		(*func)();
	}
}


static void do_sys_initcall()
{
	do_initcall(_early_initcall);
	do_initcall(_initcall);
	do_initcall(_late_initcall);
}

void system_init()
{
	platform_init();

	interrupt_init();

	puts("sys_initcall\n");
	do_sys_initcall();
}
