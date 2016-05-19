#ifndef INIT_H
#define INIT_H

#include "typedef.h"

typedef void (*initcall_t)(void);

#define __initcall(fn)  \
	const initcall_t __initcall_##fn \
			__attribute__((section(".initcall"))) = fn

#define early_initcall(fn)  \
	const initcall_t __initcall_##fn \
		__attribute__((section(".early.initcall"))) = fn

#define late_initcall(fn)  \
	const initcall_t __initcall_##fn \
		__attribute__((section(".late.initcall"))) = fn


















#endif

