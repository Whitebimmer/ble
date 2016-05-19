#include "hwi.h"
#include "stdio.h"
#include "thread.h"


#define CONNEST_TYPE    1   // 0 MASTER  1SLAVE

/*const u8 local_address[6] = {0x2e, 0x2a, 0xba, 0x98, 0x76, 0x54};
const u8 local_name[2] = "16";*/

u32 jiffies;
volatile u32 g_rand_seed ;

void (*thread_fun[PRIORITY_NUM])(u8) = {0};

int os_create_thread(void (*fun)(u8), u8 priority)
{
	thread_fun[priority] = fun;
	/*printf("prio: %d, fun=%x\n", priority, fun);*/

	return 0;
}

int os_delete_thread(u8 priority)
{

	return 0;
}

static u32 malloc_buf[512];


void *malloc(int len)
{
	void *p = lbuf_alloc(malloc_buf, len);
	ASSERT(p != NULL);
	printf("malloc: %x\n", p);
	return p;
}

void *calloc(int v, int len)
{
	void *p;

	p = malloc(len);
	memset(p, 0, len);

	return p;
}

void free(void *ptr)
{
	if (ptr){
		puts("free-");
		lbuf_free(ptr);
		puts("exit\n");
	}
}

void *realloc(void *ptr, int len)
{
	void *p;

	p = malloc(len);
	memcpy(malloc_buf, ptr, len); 
	free(ptr);

	return p;
}

void exception_isr(void) AT(.dll_api);
void exception_isr(void)
{
    puts("\n\n---------------------------exception_isr---------------------\n\n");
    put_u32hex(DEBUG_MSG);
    /* PWR_CON |= BIT(4); */
    while(1);
}

const char pubDate[] = "\r\n***********BT16 MAIN "__DATE__ " " __TIME__"***********\r\n";
int main()
{
	int i;

#ifdef FPGA 
	pll_init();
#else
	bt16_pll_init();
#endif
    delay(500000);

    uart_init((96000000/ 460800));  // pa8
    puts(pubDate);
    puts("...fpga bt16 setup ok.......\n");

#ifdef FPGA 
	spi_int();  
#endif
    puts("-----1\n");
	system_init();

    puts("-----2\n");
	timer0_start();

    puts("-----3\n");
	RF_init();

    //----------debug
    HWI_Install(1, exception_isr, 3) ; //timer0_isr
    

	ENABLE_INT();

    puts("-----4\n");
	thread_init(os_create_thread, os_delete_thread);

    puts("-----5\n");
	sys_timer_init();

    puts("-----6\n");
    ble_main();
    btstack_main();

	/* device_manager_init(); */

	/*INTALL_HWI(BT_BLE_INT, le_hw_isr, 0);
	INTALL_HWI(18, le_test_uart_isr, 0);*/

	lbuf_init(malloc_buf, sizeof(malloc_buf)*4);

	/* btstack_v21_main(); */

    puts("------------4.0 start run loop-----------\n");
    while(1)
    {
		int c;
       //asm("idle");
	   /*delay(100000);*/
		for (i=0; i<PRIORITY_NUM; i++)
		{
			if (thread_fun[i]){
				thread_fun[i](i);
			}

		}
        c = getchar();
        switch(c)
        {
            case 'A':
                puts("user cmd : ADV\n");
                ble_set_adv();
                break;
            case 'S':
                puts("user cmd : SCAN\n");
                ble_set_scan();
                break;
            default:
                break;
        }

		/*run_loop_execute();*/
	   /*printf("k");*/
    }

    return 0;
}


int hal_tick_init()
{
}

void hal_tick_set_handler(void (*p)())
{

}

int hal_tick_get_tick_period_in_ms()
{
}

void hal_cpu_disable_irqs()
{

}

void hal_cpu_enable_irqs()
{

}

void hal_cpu_enable_irqs_and_sleep()
{

}

void bt_puts(const char *p)
{
	puts(p);
}

void bt_flip_access_addr(u8 * dest, u8 * src){
    dest[0] = src[4];
    dest[1] = src[3];
    dest[2] = src[2];
    dest[3] = src[1];
    dest[4] = src[0];
}


void endian_change(u8 * src, tu16 len)
{
    tu16 temp_len;
    int i = 0;
    int temp_value;
    if(len == 0)
    {
        return ;
    }
    temp_len = len - 1;
    for(i=0;i<len/2;i++)
    {
        temp_value = src[temp_len];
        src[temp_len] = src[i];
        src[i] = temp_value;
        temp_len--;
    }

}


u16 crc16(void *data, int len)
{
    int a, b;

    a = crc8(data, len);
    b = crc8(data+1, len-1);
   return (a<<8) | b;
}

















