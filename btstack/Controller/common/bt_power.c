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
};

static struct bt_power_hdl hdl sec(.btmem_highly_available);

#define __this (&hdl)

static void __power_resume()
{
}

static void __power_on()
{
	struct bt_power *p, *n;

    /* puts("__power_on\n"); */
	__this->pending = 0;
	list_for_each_entry_safe(p, n, &__this->head_off, entry){
		if (p->ops){
			p->ops->on(p->priv);
		}
	}
}

//diable flash power  (code must run in RAM!!!)
//recovery flash power (code must run in RAM!!!)
static void __power_off(void) sec(.poweroff_text);
static void __power_off()
{
	u32 usec;
	u32 timeout = -1;
	struct bt_power *p;

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
	//printf("timeout: %d\n", timeout);
    /* put_u32hex(timeout); */

	if (timeout < 800*1000){
        /* puts("suspend\n"); */
		timeout = __this->driver->set_suspend_timer(timeout, __power_resume);

		if (timeout == 0)
        {
            CPU_INT_EN();
        	return;
        }
        /* putchar('0'); */
		list_for_each_entry(p, &__this->head_off, entry){
			if (p->ops){
				p->ops->suspend_probe(p->priv);
			}
		}
        /* putchar('1'); */
		list_for_each_entry(p, &__this->head_off, entry){
			if (p->ops){
				p->ops->suspend_post(p->priv, timeout);
			}
		}
        /* putchar('2'); */
        __this->driver->suspend_enter();
		//trig_fun();
        DEBUG_IO_0(1);
		__asm__ volatile ("idle");
        __builtin_pi32_nop();
        __builtin_pi32_nop();
        __builtin_pi32_nop();
        DEBUG_IO_1(1);
        __this->driver->suspend_exit();

	} else {
        /* puts("power off\n"); */
		timeout = __this->driver->set_off_timer(timeout, __power_on);

		if (timeout == 0)
        {
            CPU_INT_EN();
        	return;
        }
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

        __this->driver->off_enter();
		RFI_CON &= ~BIT(1);
		/* while(1); */
	}
    CPU_INT_EN();
}


void bt_power_on(void *priv)
{
    struct bt_power *power = (struct bt_power *)priv;

	__this->pending = 0;
	list_del(&power->entry);
	list_add_tail(&power->entry, &__this->head_on);
}

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

    driver->ioctrl(POWER_OSC_INFO, param->osc_type, param->osc_hz);

    driver->ioctrl(POWER_RTC_PORT, param->is_use_PR);

    driver->ioctrl(POWER_DELAY_US, param->delay_us);

	if (driver && driver->init){
		driver->init();
	}

	__this->driver = driver;
}


u8 bt_power_is_poweroff_probe(void)
{
    __this->is_poweroff = __power_is_poweroff(); 
    return __this->is_poweroff; 
}

u8 bt_power_is_poweroff_post(void)
{
	/* puts(__func__); */
    /* put_u8hex(__this->is_poweroff); */

    return __this->is_poweroff; 
}







