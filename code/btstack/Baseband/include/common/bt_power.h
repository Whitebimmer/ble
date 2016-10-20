#ifndef BT_POWER_H
#define BT_POWER_H

#include "typedef.h"
//API
#define AT_POWER    AT(.poweroff_text)

#define POWER_DOWN_TIMEOUT_MIN  (50*625)  //间隔至少要50slot以上才进入power down
#define POWER_OFF_TIMEOUT_MIN   (60*625)  //间隔至少要60slot以上才进入power off

#define BT_POWER_OFF_EN         BIT(1)
#define BT_POWER_DOWN_EN        BIT(2)

#define RAM1_START      0x40000L
#define RAM1_SIZE       (24*1024L)
#define RAM1_END        (RAM1_START + RAM1_SIZE)
#define MAGIC_ADDR      (RAM1_END - 8)
#define RAM_ADDR        (RAM1_END - 12)
#define FUNC_ADDR       (RAM1_END - 16)
#define POWER_ADDR      (RAM1_END - 20)


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
    u8 config;
	u8 d2sh_dis_sw;
	u32 (*lowpower_fun)(void);
	void (*poweroff_fun)(void);
};


struct bt_power_operation {
	u32  (*get_timeout)(void *priv);

	void (*suspend_probe)(void *priv);

	void (*suspend_post)(void *priv, u32 usec);

	void (*resume)(void *priv, u8 reason);

	void (*off_probe)(void *priv);

	void (*off_post)(void *priv, u32 usec);

	void (*on)(void *priv);
};

enum {
	POWER_OSC_INFO,
    POWER_RTC_PORT,
    POWER_DELAY_US,                                                                                            
    POWER_WAKE_IO_CALLBACK,                                                                                            
    POWER_LWR_DEAL_CALLBACK,
    POWER_SET_POWEROFF_FUN,
	POWER_SET_RESET_MASK,
	POWER_SET_D2SH_DIS,
	POWER_SET_FAST_CHARGE_SW,
	POWER_GET_FAST_CHARGE_STA,
};


struct bt_power_driver{
    void (*init)();

	u32 (*set_suspend_timer)(u32 usec);

	u32 (*set_off_timer)(u32 usec);

    void (*suspend_enter)(void);

    void (*suspend_exit)(u32 usec);

    void (*off_enter)(u32 usec);

    void (*off_exit)(void);

    u32 (*ioctrl)(int ctrl, ...);
};


// #define REGISTER_POWER_OPERATION(ops) \
	// const struct bt_power_driver *__power_ops \
			// __attribute__((section(".bt_power"))) = &ops
#define REGISTER_POWER_OPERATION(ops) \
	const struct bt_power_driver *__power_ops \
			SEC(.bt_power) = &ops

extern const struct bt_power_driver *__power_ops;


/********************************************************************************/
/*
 *      bluetooth power off recover link 
 */
void bt_poweroff_recover(void);
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


void bt_power_init(const struct bt_low_power_param *param);

u8 bt_power_is_poweroff_probe();

u8 bt_power_is_poweroff_post();

void bt_power_lwr_deal_register_handle(void  (*handle)(u8 mode,u32 timer_ms));

void test_power_init(void);

void fast_charge_sw(u8 sw);

u32 get_fast_charge_sta(void);

extern void set_pwrmd(u8 sm);
#endif

