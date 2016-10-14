#ifndef INIT_H
#define INIT_H

#include "typedef.h"

typedef void (*initcall_t)(void);

#define __initcall(fn)  \
	const initcall_t __initcall_##fn \
			AT(.initcall) = fn

#define early_initcall(fn)  \
	const initcall_t __initcall_##fn \
			AT(.early.initcall) = fn

#define late_initcall(fn)  \
	const initcall_t __initcall_##fn \
			AT(.late.initcall) = fn


















#endif

