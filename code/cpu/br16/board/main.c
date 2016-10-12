#include "hwi.h"
#include "stdio.h"
#include "thread.h"
#include "bt_power.h"


#define CONNEST_TYPE    1   // 0 MASTER  1SLAVE

/*const u8 local_address[6] = {0x2e, 0x2a, 0xba, 0x98, 0x76, 0x54};
const u8 local_name[2] = "16";*/

u32 jiffies;
volatile u32 g_rand_seed ;

void (*thread_fun[PRIORITY_NUM])(u8) = {0};


#define OSSemPend(x, y)
#define OSSemPost(x)

volatile u8 g_thread_created = 0;

int os_create_thread(void (*fun)(u8), u8 priority)
{
	thread_fun[priority] = fun;

    g_thread_created |= BIT(priority);
	/*printf("prio: %d, fun=%x\n", priority, fun);*/

	return 0;
}

int os_delete_thread(u8 priority)
{
	thread_fun[priority] = NULL;

    g_thread_created &= ~BIT(priority);

	return 0;
}

volatile u8 g_task_sleep_bitmap = 0;

#define THREAD_SUSPEND(x)   g_task_sleep_bitmap |= BIT(x)
#define THREAD_RESUME(x)    g_task_sleep_bitmap &= ~BIT(x)

#define TASK_IS_AWAKE()   ((g_task_sleep_bitmap & g_thread_created ) != g_thread_created )
#define TASK_IS_SLEEP()   ((g_task_sleep_bitmap & g_thread_created ) == g_thread_created )

void os_suspend_thread(u8 priority, u8 timeout)
{
    THREAD_SUSPEND(priority);
    if (TASK_IS_SLEEP())
    {
        putchar(')');
        bt_power_off_unlock();
    }
}

void os_resume_thread(u8 priority)
{
    bt_power_off_lock();
    putchar('(');
    THREAD_RESUME(priority);
}

const struct thread_interface os_thread_ins = {
	.os_create = os_create_thread,
	.os_delete = os_delete_thread,
	.os_suspend = os_suspend_thread,
	.os_resume = os_resume_thread,
}; 

void bt_task_run_loop(void)
{
	int i;

    bt_power_off_lock();
    for (i=0; i<PRIORITY_NUM; i++)
    {
        if (thread_fun[i]){
            thread_fun[i](i);
        }

    }
}
/*
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
*/

void exception_isr(void) AT(.dll_api);
void exception_isr(void)
{
    puts("\n\n---------------------------SDK exception_isr---------------------\n\n");
    put_u32hex(DEBUG_MSG);
    /* PWR_CON |= BIT(4); */
    while(1);
}
extern u32 bss_begin;
extern u32 bss_size;
extern u32 bss_end;
extern u32 data_addr;
extern u32 data_begin;
extern u32 data1_size;
extern u32 data_size;
extern u32 ram1_begin;
extern u32 ram1_size;

const char pubDate[] = "\r\n***********BR16 MAIN "__DATE__ " " __TIME__"***********\r\n";

void (* __maskrom_set_exception_isr_hook)(void (*func)(void)) = 0x500b8;

void set_exception_isr_hook(void (*func)(void))
{
    __maskrom_set_exception_isr_hook(func);
}

void disable_wtd(void)
{
   //enable watchdog reset 4s
    CRC_REG = 0x6EA5;
    WDT_CON &= ~BIT(4);
    CRC_REG = 0;
}

const struct bt_low_power_param bt_power_param = {
    .osc_type = BT_OSC,
    .osc_hz = 12000000L,
    .is_use_PR = 0,
    .delay_us = 64000000/1000000L,
};

u8 ram2_memory[0x100] sec(.db_memory);

int main()
{
    /* printf("CLK_CON0 : %x\n", CLK_CON0); */
    /* printf("CLK_CON1 : %x\n", CLK_CON1); */
    /* printf("CLK_CON2 : %x\n", CLK_CON2); */
    /* printf("SYS_DIV : %x\n", SYS_DIV); */
	disable_wtd();
#ifdef FPGA 
	pll_init();
    uart_init((48000000/ 460800));  // pa8
    puts("...fpga br16 setup ok.......\n");
#else
	br16_pll_init();
    uart_init((64000000/ 460800));  // pa8
    puts("...br16 setup ok.......\n");
    /* SFR(FMA_CON1, 8, 1, 1);   // FM_LDO TO BT */
    /* SFR(LDO_CON, 5, 1, 1);    // open ldo15 */
    FMA_CON1 |= BIT(8);
    LDO_CON |= BIT(5);
#endif
    delay(500000);

    puts(pubDate);

    /* printf("bss_begin : %x\n", &bss_begin); */
    printf("bss_size : %x\n",  &bss_size);
    printf("bss_end: %x\n",    &bss_end);
    /* printf("data_addr: %x\n",  &data_addr); */
    printf("data_begin: %x\n", &data_begin);
    printf("data_size: %x\n",  &data1_size);
    printf("data_size: %x\n",  &data_size);

    printf("ram1_begin: %x\n", &ram1_begin);
    printf("ram1_size: %x\n",  &ram1_size);
    /* pmalloc_init(RAM_S, RAM_E); */
#ifdef FPGA 
    puts("-----0\n");
    spi_int();  
#endif
    bt_power_is_poweroff_post();
    put_u8hex(bt_power_is_poweroff_probe());

    puts("-----1\n");
	system_init();

    puts("-----2\n");
	timer0_start();

    puts("-----3\n");
	RF_init();

    //----------debug
    HWI_Install(EXCEPTION_INIT, exception_isr, 3) ; //timer0_isr
    disable_wtd();
    /* { */
        /* u32 *ptr = 0x55; */

        /* *ptr = 1; */
    /* } */
    u8 *ptr;
    ptr = ram2_memory;
#if 1
    *ptr++ = 0x55; 
    *ptr++ = 0xaa; 
    printf_buf(ram2_memory, 0x10);
#else
    if (*ptr++ != 0x55 || *ptr++ != 0xaa){
        puts("memory lost!\n");
        printf_buf(ram2_memory, 0x10);
        while(1);
    }
#endif


	ENABLE_INT();
    puts("-----4\n");
	thread_init(&os_thread_ins);

	puts("-----5\n");
	sys_timer_init();

    bd_ram1_memory_init();
    bd_memory_init();

    if(bt_power_is_poweroff_post())
    {
    	/* puts("-----6\n"); */
        ble_main();
        bt_poweroff_recover();
    }
    else
    {
    	puts("-----7\n");
    	bt_power_init(&bt_power_param);
    	ble_main();
    	btstack_main();
    }


	/*INTALL_HWI(BT_BLE_INT, le_hw_isr, 0);
	INTALL_HWI(18, le_test_uart_isr, 0);*/

	/* lbuf_init(malloc_buf, sizeof(malloc_buf)*4); */

	/* device_manager_init(); */
	/* btstack_v21_main(); */

    puts("------------4.2 start run loop-----------\n");
    while(1)
    {
		int c;
       //asm("idle");
	   /*delay(100000);*/

        bt_task_run_loop();

        while(1)
        {
            //asm("idle");
            c = getchar();

            /* puts("user cmd : ADV\n"); */
            /* if (c) */
            /* { */
                /* putchar(c); */

                /* stdin_process(c); */
            /* } */
            switch(c)
            {

                case 'A':
                    puts("user cmd : ADV\n");
#ifndef BLE_BQB_PROCESS_EN
                    ble_set_adv();
#else
					ble_set_direct_RPA_adv();
#endif
                    break;
                case 'S':
                    puts("user cmd : SCAN\n");
                    ble_set_scan();
                    break;
                case 'C':
                    puts("user cmd : CONN\n");
                    ble_set_conn();
                    break;
                case 'T':
                    /* puts("user cmd : TEST\n"); */
                    ble_test();
                    break;
                case 'D':
                    puts("user cmd : Data\n");
                    ble_send_data();
                    break;
                default:
                    break;
            }
            if (TASK_IS_AWAKE())
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

#if 0

#define RAM_S       0x8000
#define RAM_E       0x18000
#define RAM_SIZE    0x18000
#define PAGE_SIZE   512
#define RAM_USD     

u32 pm_usd_map[(RAM_SIZE)/PAGE_SIZE/32 + 1];
u32 pm_mem_curren_pos_begin;
u32 pm_mem_curren_pos; 

#define mem_printf      printf

int reverse(int idx)
{
    int i;
    int x;

    mem_printf("reverse =%x \r\n", idx);
    for (i = 31, x=-1; i >= idx; i--) 
    {
        x &= ~BIT(i);
        
        mem_printf("i =%x %x\r\n", i, x);
    }
    return x;
}

u32 pmalloc_init(u32 saddr ,u32 eaddr)
{
    u32 head_bit_idx, tail_bit_idx;
    u8 head_word_usd_bit, tail_word_usd_bit;
    u32 total_bit, total_word;
    u32 word;

    mem_printf("saddr = %x eaddr =%x \r\n",saddr, eaddr ) ;
    head_bit_idx = (saddr)/ PAGE_SIZE;
    tail_bit_idx = (eaddr)/ PAGE_SIZE;
    //bit map cnt
    total_bit = tail_bit_idx - head_bit_idx;
    mem_printf("head_bit_idx = %x tail_bit_idx =%x total_bit =%x \r\n",head_bit_idx, tail_bit_idx, total_bit ) ;

    memset(pm_usd_map, 0,sizeof(pm_usd_map));

    //usd bit in word 32
    head_word_usd_bit = head_bit_idx&0x1f;
    tail_word_usd_bit = tail_bit_idx&0x1f;

    //bit map cnt - head usd cnt
    total_bit -= (31 - head_word_usd_bit);
    //bit map cnt - tail usd cnt
    total_bit -= (31 - tail_word_usd_bit);

    mem_printf("head_word_usd_bit = %x tail_word_usd_bit =%x total_bit =%x \r\n",head_word_usd_bit, tail_word_usd_bit, total_bit ) ;
    total_word = total_bit/32 ;
    mem_printf("total_word =%x \r\n", total_word ) ;

    pm_mem_curren_pos_begin = head_bit_idx;
    pm_mem_curren_pos = pm_mem_curren_pos_begin ;

    //skip head usd bit
    mem_printf("range = %x =%x  \r\n",head_bit_idx/32, tail_bit_idx/32 ) ;

    //fix head usd bit in word
    word = (head_bit_idx/32);
    if (head_word_usd_bit)
    {
        pm_usd_map[word] = -(1<<head_word_usd_bit);//head_word_usd_bit
        mem_printf("head =%x \r\n", pm_usd_map[word]) ;
        word++;
    }
    for (; word < tail_bit_idx/32; word++)
    {
        pm_usd_map[word] = -1;
        mem_printf("%x \r", pm_usd_map[word]) ;
        
    }
    //fix tail usd bit in word
    if (tail_word_usd_bit)
    {
        word = (tail_bit_idx/32);
        pm_usd_map[word] = reverse(tail_word_usd_bit);//tail_word_usd_bit
        mem_printf("tail =%x \r\n", pm_usd_map[word] ) ;
    }

    printf_buf(pm_usd_map, sizeof(pm_usd_map)+8);
    while(1);
}

//private 
static int cpu_cnt_step;
static int cpu_idle_cnt;
static int cpu_busy_cnt;
static int cpu_usage;

struct cpu_usage_operation
{
    //func
    //
}

static void cpu_usage_init(void)
{
    cpu_usage = 0;
    cpu_cnt_step = 0;
    cpu_idle_cnt = 0; 
    cpu_busy_cnt = 0;
};

static int cpu_usage_result(void) 
{
    return cpu_usage;
}

static int cpu_idle_state(void)
{
    u32 tmp;

    asm("mov %0, RETI " : "=r" (tmp));
    put_u32hex(tmp);
    if ()
}

static const struct cpu_usage_operation cpu_usage_ops = {
    .init = ;
    .idle_state = ;
    .result = ;
}

void cpu_usage_callc(void)
{
    if (cpu_cnt_step > 1000)
    {
        //update usage
        cpu_usage = cpu_busy_cnt/cpu_cnt_step;

        //reset cnt
        cpu_cnt_step = 0;
        cpu_idle_cnt = 0;
        cpu_busy_cnt = 0;
    }
    cpu_cnt_step++;
    
    //in idel state
    if (cpu_usage_ops->idle_state())
    {
        cpu_busy_cnt++;
        return;
    }


    cpu_idle_cnt++; 
}
#endif
