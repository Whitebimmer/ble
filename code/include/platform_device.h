#ifndef PLATFORM_DEVICE_H
#define PLATFORM_DEVICE_H



#include "typedef.h"
#include "list.h"





#define IORESOURCE_IRQ    1

struct resource {
	void *start;
	void *end;
	unsigned long flags;
};

struct platform_device {
	const char *name;
	u8 num_resources;
	struct resource *resource;
	struct list_head entry;
	void *priv;
};

struct platform_driver{
	const char *name;
	struct list_head entry;
	struct platform_device *device;
	void (*probe)(struct platform_device *dev);
	void (*remove)(struct platform_device *dev);
};

void platform_device_register(struct platform_device *);


void platform_add_devices(struct platform_device **, int );


struct resource *platform_get_resource(struct platform_device *dev, 
		unsigned int type, unsigned int num);

int platform_get_irq(struct platform_device *dev, unsigned int num);

int platform_driver_register(struct platform_driver *driver);






#endif

