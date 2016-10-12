#ifndef BT_POWER_H
#define BT_POWER_H

#include "typedef.h"


struct bt_power_operation {
	u32  (*get_timeout)(void *priv);

	void (*suspend_probe)(void *priv);

	void (*suspend_post)(void *priv, u32 usec);

	void (*resume)(void *priv);


	void (*off_probe)(void *priv);

	void (*off_post)(void *priv, u32 usec);

	void (*on)(void *priv);
};

enum {
    POWER_OSC_INFO,
    POWER_RTC_PORT,
    POWER_DELAY_US,
};

struct bt_power_driver{
    void (*init)();

	u32 (*set_suspend_timer)(u32 usec, void (*resume)());

	u32 (*set_off_timer)(u32 usec, void (*on)());

    void (*suspend_enter)(void);

    void (*suspend_exit)(void);

    void (*off_enter)(void);

    void (*off_exit)(void);

    void (*ioctrl)(int ctrl, ...);
};

typedef enum{
    BT_OSC = 0,
    RTC_OSCH,
    RTC_OSCL,
}OSC_TYPE;

struct bt_low_power_param{
    OSC_TYPE osc_type;
    u32 osc_hz;
    u8 is_use_PR;
    u8 delay_us;
};

#define REGISTER_POWER_OPERATION(ops) \
	const struct bt_power_driver *__power_ops \
			__attribute__((section(".bt_power"))) = &ops

extern const struct bt_power_driver *__power_ops;

//API
/********************************************************************************/
/*
 *      bluetooth link request power manager on 
 */
void bt_power_on(void *priv);

/********************************************************************************/
/*
 *      bluetooth link request power manager off 
 */
void bt_power_off(void *priv);

/********************************************************************************/
/*
 *      bluetooth lock power manager 
 */
void bt_power_off_lock();

/********************************************************************************/
/*
 *      bluetooth unlock power manager 
 */
void bt_power_off_unlock();

/********************************************************************************/
/*
 *      bluetooth link get power manager unit
 */
void *bt_power_get(void *priv, const struct bt_power_operation *ops);

/********************************************************************************/
/*
 *      bluetooth link delete power manager unit
 */
void bt_power_put(void *priv);

void bt_power_init();

u8 bt_power_is_poweroff_probe();

u8 bt_power_is_poweroff_post();

#endif

