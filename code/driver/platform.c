#include "platform_device.h"


struct bus {
	struct list_head device;
	struct list_head driver;
};


static struct bus platform_bus;



void platform_device_register(struct platform_device *dev)
{
	list_add_tail(&dev->entry, &platform_bus.device);
}

void platform_add_devices(struct platform_device **devs, int num)
{
	int i;

	for (i=0; i<num; i++) {
		list_add_tail(&devs[i]->entry, &platform_bus.device);
	}
}

struct resource *platform_get_resource(struct platform_device *dev, 
		unsigned int type, unsigned int num)
{
	int i;

	for (i=0; i<dev->num_resources; i++) {
		struct resource *r = &dev->resource[i];

		if (type == r->flags && num-- == 0){
			return r;
		}
	}
	return NULL;
}

int platform_get_irq(struct platform_device *dev, unsigned int num)
{
	struct resource *r;

	r = platform_get_resource(dev, IORESOURCE_IRQ, num);

	return r? r->start : -1;
}


int platform_driver_register(struct platform_driver *drv)
{
	struct platform_device *dev;

	list_for_each_entry(dev, &platform_bus.device, entry){
		if (!strcmp(dev->name, drv->name)){
			drv->device = dev;
			drv->probe(dev);
			list_add_tail(&drv->entry, &platform_bus.driver);
			return 0;
		}
	}

	return -1;
}

struct platform_driver *platform_get_driver(const char *name)
{
	struct platform_driver *drv;

	list_for_each_entry(drv, &platform_bus.driver, entry){
		if (!strcmp(drv->name, name)){
			return drv;
		}
	}
	return NULL;
}


void platform_init()
{
	INIT_LIST_HEAD(&platform_bus.device);
	INIT_LIST_HEAD(&platform_bus.driver);
}


