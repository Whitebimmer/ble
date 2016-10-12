#include "lbuf.h"
#include "bt_memory.h"

static u8 mem_pool[4*1024] sec(.baseband);

static u8 ram1_pool[2*1024] sec(.ram1_data);


void *bd_malloc(int size)
{
    u8 *p;
    p = lbuf_alloc(mem_pool, size);
    if(p)
    {
       my_memset(p,0,size);
    }
    else
    {
        puts("malloc null");
        while(1);
    }
	return p;
}

void bd_realloc(void *p, int size)
{
	lbuf_realloc(p, size);
}

void bd_free(void *p)
{
	lbuf_free(p);
}

void bd_memory_init()
{
	lbuf_init(mem_pool, sizeof(mem_pool));
}


void *bd_ram1_malloc(int size)
{
    u8 *p;
    p = lbuf_alloc(ram1_pool, size);
    if(p)
    {
       my_memset(p,0,size);
    }
    else
    {
        puts("ram1 malloc null");
        while(1);
    }
	return p;
}

void bd_ram1_memory_init()
{
	lbuf_init(ram1_pool, sizeof(ram1_pool));
}
void *bt_malloc(int flag, u32 size)
{
//	if (flag & BTMEM_NORMAL){
//		return malloc(size);
//	}
	if (flag & BTMEM_BASEBAND){
//		return lbuf_alloc(mem_pool, size);
	}
	if (flag & BTMEM_HIGHLY_AVAILABLE){
		return lbuf_alloc(ram1_pool, size);
	}
	puts("bt_malloc: faild\n");
	while(1);
}

void bt_free(void *p)
{
	lbuf_free(p);

}
