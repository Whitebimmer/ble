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
        /* putchar(')'); */
        bt_power_off_unlock();
    }
}

void os_resume_thread(u8 priority)
{
    bt_power_off_lock();
    /* putchar('('); */
    THREAD_RESUME(priority);
}

const struct thread_interface os_thread_ins = {
	.os_create = os_create_thread,
	.os_delete = os_delete_thread,
	.os_suspend = os_suspend_thread,
	.os_resume = os_resume_thread,
}; 


void task_run_loop(void)
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

/* void exception_isr(void) AT(.dll_api); */
void exception_isr(void)
{
    puts("\n\n---------------------------SDK exception_isr---------------------\n\n");
    put_u32hex(DEBUG_MSG);
    if (DEBUG_MSG & BIT(2))
        puts("Peripheral ex limit err\n");
    if (DEBUG_MSG & BIT(8))
        puts("DSP MMU err,DSP write MMU All 1\n");
    if (DEBUG_MSG & BIT(9))
        puts("PRP MMU err,PRP write MMU 1\n");
    if (DEBUG_MSG & BIT(16))
        puts("PC err,PC over limit\n");
    if (DEBUG_MSG & BIT(17))
        puts("DSP ex err,DSP store over limit\n");
    if (DEBUG_MSG & BIT(18))
        puts("DSP illegal,DSP instruction\n");
    if (DEBUG_MSG & BIT(19))
        puts("DSP misaligned err\n");

    u32 tmp;

    __asm__ volatile ("mov %0,RETS" : "=r"(tmp));
    printf("RETS = %08x \n", tmp);
    __asm__ volatile ("mov %0,RETI" : "=r"(tmp));
    printf("RETI = %08x \n", tmp);
    __asm__ volatile ("mov %0,sp" : "=r"(tmp));
    printf("SP = %08x \n", tmp);
    __asm__ volatile ("mov %0,usp" : "=r"(tmp));
    printf("USP = %08x \n", tmp);
    __asm__ volatile ("mov %0,ssp" : "=r"(tmp));
    printf("SSP = %08x \n", tmp);
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

const char pubDate[] = "\r\n***********BR17 MAIN "__DATE__ " " __TIME__"***********\r\n";

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
    .osc_type = RTC_OSCL,
    .osc_hz = 32768L,
    .is_use_PR = 0,
    .delay_us = 160000000/1000000L,
    .config = BT_POWER_DOWN_EN,
    .d2sh_dis_sw = 0,
};

u8 ram2_memory[0x100] SEC(.db_memory);


_NO_INLINE_
void foo1(void)
{
    puts("---call foo1\n");
    u32 tmp;
    u8 buf[50];
    buf[0] = 1;
    __asm__ volatile ("mov %0,sp" : "=r"(tmp));
    printf("SP = %08x \n", tmp);
    __asm__ volatile ("mov %0,usp" : "=r"(tmp));
    printf("USP = %08x \n", tmp);
    __asm__ volatile ("mov %0,ssp" : "=r"(tmp));
    printf("SSP = %08x \n", tmp);
}

_NO_INLINE_
void foo(void)
{
    u32 tmp;
	ENABLE_INT();
    puts("---call foo\n");
    __asm__ volatile ("mov %0,ie0" : "=r"(tmp));
    printf("ie0 = %08x \n", tmp);
    __asm__ volatile ("mov %0,ie1" : "=r"(tmp));
    printf("ie1 = %08x \n", tmp);
    __asm__ volatile ("mov %0,icfg" : "=r"(tmp));
    printf("icfg = %08x \n", tmp);
    u8 buf[100];

    buf[0] = 1;
    __asm__ volatile ("mov %0,sp" : "=r"(tmp));
    printf("SP = %08x \n", tmp);
    __asm__ volatile ("mov %0,usp" : "=r"(tmp));
    printf("USP = %08x \n", tmp);
    __asm__ volatile ("mov %0,ssp" : "=r"(tmp));
    printf("SSP = %08x \n", tmp);
    foo1();
    buf[0] = 2;
    buf[1] = 2;
    buf[2] = buf[0] + buf[1];
    printf("buf = %08x \n", buf[0]);
}

int main()
{
    /* printf("CLK_CON0 : %x\n", CLK_CON0); */
    /* printf("CLK_CON1 : %x\n", CLK_CON1); */
    /* printf("CLK_CON2 : %x\n", CLK_CON2); */
    /* printf("SYS_DIV : %x\n", SYS_DIV); */
    /* {PORTA_DIR &= ~BIT(1); PORTA_OUT |= BIT(1);} */
#ifdef FPGA 
	fpga_pll_init();
    uart_init((48000000/ 460800));  // pa8
    /* uart_init((48000000/ 115200));  // pa8 */
    puts("...fpga br17 setup ok.......\n");
#else
    pll_init();
    uart_init((48000000/ 460800));  // pa8
    puts("...br17 setup ok.......\n");
    /* SFR(FMA_CON1, 8, 1, 1);   // FM_LDO TO BT */
    /* SFR(LDO_CON, 5, 1, 1);    // open ldo15 */
    SFR(FMA_CON1, 12, 1, 1);   // FM_LDO TO BT
    SFR(LDO_CON, 31, 1, 1);    // open ldo15
    SFR(LDO_CON, 1, 1, 1);    // open ldo15
#endif

    puts("\nbt_power_is_poweroff_post : ");
    put_u8hex(bt_power_is_poweroff_post());
    /* puts("\nbt_power_is_poweroff_probe : "); */
    /* put_u8hex(bt_power_is_poweroff_probe()); */
    u32 tmp;
    __asm__ volatile ("mov %0,sp" : "=r"(tmp));
    printf("SP = %08x \n", tmp);
    __asm__ volatile ("mov %0,usp" : "=r"(tmp));
    printf("USP = %08x \n", tmp);
    __asm__ volatile ("mov %0,ssp" : "=r"(tmp));
    printf("SSP = %08x \n", tmp);
    foo();

    if (0)//if ((PWR_CON & 0xe0) != 0x80)
    {
        delay(500000);

        puts(pubDate);
        printf("PWR_CON : %x\n", PWR_CON);

        /* printf("bss_begin : %x\n", &bss_begin); */
        printf("bss_size : %x\n",  &bss_size);
        printf("bss_end: %x\n",    &bss_end);
        /* printf("data_addr: %x\n",  &data_addr); */
        printf("data_begin: %x\n", &data_begin);
        printf("data_size: %x\n",  &data1_size);
        printf("data_size: %x\n",  &data_size);

        printf("ram1_begin: %x\n", &ram1_begin);
        printf("ram1_size: %x\n",  &ram1_size);
    }
    /* pmalloc_init(RAM_S, RAM_E); */

    puts("-----1\n");
	system_init();

    puts("-----2\n");
	timer0_start();

    bt_power_init(&bt_power_param);
    set_pwrmd(2);
    bt_osc_internal_cfg(0x11, 0x11);

    puts("-----3\n");

    RF_init();

    //----------debug
    /* HWI_Install(EXCEPTION_INIT, exception_isr, 3) ; //timer0_isr */
    HWI_Install(0, exception_isr, 3) ; //timer0_isr
    disable_wtd();
    /* { */
        /* u32 *ptr = 0x55; */

        /* *ptr = 1; */
    /* } */
#if 0
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

#endif

    /* __asm__ volatile ("mov %0,ie0" : "=r"(tmp)); */
    /* printf("ie0 = %08x \n", tmp); */
    /* __asm__ volatile ("mov %0,ie1" : "=r"(tmp)); */
    /* printf("ie1 = %08x \n", tmp); */
    /* __asm__ volatile ("mov %0,icfg" : "=r"(tmp)); */
    /* printf("icfg = %08x \n", tmp); */
    /* CPU_INT_EN(); */
	ENABLE_INT();
    /* puts("-----4\n"); */
	thread_init(&os_thread_ins);

    /* puts("-----5\n"); */
	sys_timer_init();

    bd_ram1_memory_init();
    bd_memory_init();

#ifdef LE_CONTROLLER_MODE
    ble_main();
    h4_uart_init();

    while(1)
    {
        h4_uart_data_rxloop();

        task_run_loop();
        char c;
        c = getchar();

        /* puts("user cmd : \n"); */
        if (c)
        {
            /* printf("user cmd = %s \n", c); */
            putchar(c);
            switch (c)
            {
            case 'A':
                h4_uart_dump();
                break;
            case 'C':
                h4_uart_clear();
                break;
            }
        }
    }
#else
    if(bt_power_is_poweroff_post())
    {
		ble_main();
        bt_poweroff_recover();
    }
    else
    {

    	puts("-----6\n");
    	ble_main();
    	btstack_main();
    }
    /* zstack_main(); */


	/*INTALL_HWI(BT_BLE_INT, le_hw_isr, 0);
	INTALL_HWI(18, le_test_uart_isr, 0);*/

    /* __asm__ volatile ("mov %0,ie0" : "=r"(tmp)); */
    /* printf("ie0 = %08x \n", tmp); */
    /* __asm__ volatile ("mov %0,ie1" : "=r"(tmp)); */
    /* printf("ie1 = %08x \n", tmp); */
    /* __asm__ volatile ("mov %0,icfg" : "=r"(tmp)); */
    /* printf("icfg = %08x \n", tmp); */
    puts("------------BLE 4.2 start run loop-----------\n");


    while(1)
    {
		int c;
       //asm("idle");

        task_run_loop();

        while(1)
        {
            //asm("idle");
            c = getchar();

            /* puts("user cmd : \n"); */
            if (c)
            {
                /* printf("user cmd = %s \n", c); */
                /* stdin_process(c); */
            }
            if (TASK_IS_AWAKE())
                break;
        }

		/*run_loop_execute();*/
	   /*printf("k");*/
    }

    return 0;
#endif
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
