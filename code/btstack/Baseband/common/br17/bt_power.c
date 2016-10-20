#include "bt_power.h"
#include "list.h"
#include "bt_memory.h"
#include "ble_interface.h"

struct bt_power{
	struct list_head entry;
	void *priv;
	const struct bt_power_operation *ops;
};

struct bt_power_hdl{
	struct list_head head_on;
	struct list_head head_off;
	const struct bt_power_driver *driver;
	u8 pending;
	u8 lock;
	u8 is_poweroff;
    u8 config;
    u32 (*enter_lowpower_callback)(void);
    void (*callback)(u8);
};

static struct bt_power_hdl hdl SEC(.btmem_highly_available);

#define __this (&hdl)

static void __power_resume(u8 reason)
{
	struct bt_power *p, *n;

	__this->pending = 0;
	list_for_each_entry_safe(p, n, &__this->head_off, entry){
		if (p->ops){
			p->ops->resume(p->priv, reason);
		}
	}
}

static void __power_on(u8 reason)
{
	struct bt_power *p, *n;

	__this->pending = 0;
	list_for_each_entry_safe(p, n, &__this->head_off, entry){
		if (p->ops){
			p->ops->on(p->priv);
		}
	}
}
static void bt_powerdown_recover()
{
    u8 reason;

    /* reason = __rtc_drv->check_io_wakeup_pend(0); */

	if(__this->callback)
	{
	   __this->callback(reason);
	}
}

//diable flash power  (code must run in RAM!!!)
//recovery flash power (code must run in RAM!!!)
static void __power_off(void) SEC(.poweroff_text);
static void __power_off()
{
	u32 usec;
	u32 timeout = -1;
	struct bt_power *p;

	if(__this->enter_lowpower_callback)
	{
		if(__this->enter_lowpower_callback())
		{
			putchar('Y');
		}
		else
		{
			putchar('N');
			return;
		}
	}
	else
	{
		printf("enter_lowpower_callback NULL\n");	
	}

    CPU_INT_DIS();

	list_for_each_entry(p, &__this->head_off, entry){
		if (p->ops){
			usec = p->ops->get_timeout(p->priv);
		} else {
			usec = -1;
		}
		if (usec == 0){
			putchar('x');
            CPU_INT_EN();
			return;
		}
		if (timeout > usec){
			timeout = usec;
		}
        /* putchar('!'); */
	}

	if (timeout == -1)
    {
        CPU_INT_EN();
        return;
    }
	/* putchar('&'); */
	/* printf("timeout: %d\n", timeout); */

	
	if ( (timeout > POWER_DOWN_TIMEOUT_MIN)
        && __this->config & BT_POWER_DOWN_EN)
    {

		/* puts("power down\n"); */
		timeout = __this->driver->set_suspend_timer(timeout);

		if (timeout == 0)
        {
            CPU_INT_EN();
        	return;
        }
        __this->callback = __power_resume;
		list_for_each_entry(p, &__this->head_off, entry){
			if (p->ops){
				p->ops->suspend_probe(p->priv);
			}
		}
		list_for_each_entry(p, &__this->head_off, entry){
			if (p->ops){
				p->ops->suspend_post(p->priv, timeout);
			}
		}
        __this->driver->suspend_enter();
		//trig_fun();
        DEBUG_IO_0(1);
		__asm__ volatile ("idle");
        __builtin_pi32_nop();
        __builtin_pi32_nop();
        __builtin_pi32_nop();
        DEBUG_IO_1(1);
        __this->driver->suspend_exit(timeout);
		bt_powerdown_recover();

	} 
	else if ((timeout > POWER_OFF_TIMEOUT_MIN)
            && __this->config & BT_POWER_OFF_EN)
	{

		bt_puts("power off\n");
		timeout = __this->driver->set_off_timer(timeout);

		if (timeout == 0)
        {
            CPU_INT_EN();
        	return;
        }
		
		__this->callback = __power_on;

		list_for_each_entry(p, &__this->head_off, entry){
			if (p->ops){
				p->ops->off_probe(p->priv);
			}
		}
		list_for_each_entry(p, &__this->head_off, entry){
			if (p->ops){
				p->ops->off_post(p->priv, timeout);
			}
		}

        __this->driver->off_enter(timeout);
		while(1)
		{
			/* putchar('#'); */
		}
	}
    CPU_INT_EN();
}

AT_POWER
void bt_poweroff_recover()
{
    u8 reason;

    reason = 0;
	if(__this->callback)
	{
	   __this->callback(reason);
	}
}

AT_POWER
void bt_power_on(void *priv)
{
    struct bt_power *power = (struct bt_power *)priv;

	__this->pending = 0;
	list_del(&power->entry);
	list_add_tail(&power->entry, &__this->head_on);
}

AT_POWER
void bt_power_off(void *priv)
{
    struct bt_power *power = (struct bt_power *)priv;

	list_del(&power->entry);
	list_add_tail(&power->entry, &__this->head_off);

	//putchar('P');
	if (list_empty(&__this->head_on)){
		if (__this->lock){
			__this->pending = 1;
			putchar('+');
		} else {
			putchar('-');
			__power_off();
		}
	}
}

void bt_power_off_lock()
{
	__this->lock = 1;
}

void bt_power_off_unlock()
{
	__this->lock = 0;
	if (__this->pending){
		__this->pending = 0;
		putchar('*');
		__power_off();
	}
}

void *bt_power_get(void *priv, const struct bt_power_operation *ops)
{
	struct bt_power *power;

	power = bd_ram1_malloc(sizeof(*power));
	if (!power){
		ASSERT(power != NULL);
		return NULL;
	}
	power->ops = ops;
	power->priv = priv;

	list_add_tail(&power->entry, &__this->head_on);

	return power;
}

void bt_power_put(void *priv)
{
    struct bt_power *power = (struct bt_power *)priv;

	puts("bt_power_put:");

	list_del(&power->entry);

	bt_free(power);
	puts("exit\n");
}


void bt_power_init(const struct bt_low_power_param *param)
{
    const struct bt_power_driver *driver = __power_ops;

	INIT_LIST_HEAD(&__this->head_on);
	INIT_LIST_HEAD(&__this->head_off);

    __this->config = param->config;
    __this->enter_lowpower_callback = param->lowpower_fun;
    driver->ioctrl(POWER_OSC_INFO, param->osc_type, param->osc_hz);

    driver->ioctrl(POWER_RTC_PORT, param->is_use_PR);

    driver->ioctrl(POWER_DELAY_US, param->delay_us);

	if (driver && driver->init){
		driver->init();
	}

	driver->ioctrl(POWER_SET_RESET_MASK,1); //reset mask sw
	driver->ioctrl(POWER_SET_D2SH_DIS,param->d2sh_dis_sw);   //d2sh dis sw  rtcvdd口没有拉出来要置1
	__this->driver = driver;
}
void close_d2sh_dis()
{
	__power_ops->ioctrl(POWER_SET_D2SH_DIS,0);   //d2sh dis sw  rtcvdd口没有拉出来要置1
		
}

void fast_charge_sw(u8 sw)
{
	const struct bt_power_driver *driver = __power_ops;

	driver->ioctrl(POWER_SET_FAST_CHARGE_SW,(u32)sw); 
}

u32 get_fast_charge_sta(void)
{
	u32 sta;

	const struct bt_power_driver *driver = __power_ops;

	sta = driver->ioctrl(POWER_GET_FAST_CHARGE_STA,NULL); 

	return sta;
}

#if 0
AT_POWER
void test_mz(void)
{
	/* if((JL_POWER->CON>>5) != 4) */
		/* return; */

	JL_PORTA->DIR &= ~BIT(0);
	while(1)
	{
		JL_PORTA->OUT ^= BIT(0);
		delay(1000000);
	}
}
#endif

u8 bt_power_is_poweroff_probe(void)
{
	
	u32 power_addr;
	power_addr = *((u32 *)POWER_ADDR);	
	
    __this->is_poweroff = __power_is_poweroff(); 


	
	return __this->is_poweroff; 
}

u8 bt_power_is_poweroff_post(void)
{
	/* puts(__func__); */
    /* put_u8hex(__this->is_poweroff); */

    return __this->is_poweroff; 
}
void bt_power_lwr_deal_register_handle(void (*handle)(u8 ,u32 ))
{
    __power_ops->ioctrl(POWER_LWR_DEAL_CALLBACK, handle);
}


void soft_poweroff()
{
    __soft_power_off_enter();
}


void set_pwrmd(u8 mode)
{
    /* maskrom_clear(); */
    __set_ldo_power_mode(mode);
}
void set_poweroff_wakeup_io_handle_register(void (*handle)())
{
    __power_ops->ioctrl(POWER_WAKE_IO_CALLBACK, handle);
}
u8 read_reset_power()
{
    return __read_reset_power();
}



