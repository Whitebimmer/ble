#ifndef __MODULE_H
#define __MODULE_H


#include "list.h"

#define THIS_MODULE(m) \
	static inline struct __module_##m *__this_module() { \
		extern struct __module_##m module_##m; \
		return &module_##m; \
	}

#define MODULE_MEMB_BEGIN(m) \
	struct __module_##m { \
		struct list_head timer_head; \
		struct list_head run_loop_head; \
		void (*sleep)(); \
		void (*wakeup)(); \


#define MODULE_MEMB_END(m) \
	}; \
	THIS_MODULE(m)

#define MODULE_INIT_BEGIN(m, _sleep, _wakeup) \
	struct __module_##m module_##m = { \
		.timer_head = LIST_HEAD_INIT(module_##m.timer_head), \
		.run_loop_head = LIST_HEAD_INIT(module_##m.run_loop_head), \
		.sleep = _sleep, \
		.wakeup = _wakeup, \


#define MODULE_INIT_END() \
	};




#endif

