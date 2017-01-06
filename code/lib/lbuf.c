#include "lbuf.h"


#define __get_entry(lbuf)   \
	(struct hentry *)((u8 *)lbuf - sizeof(struct hentry))


struct hfree{
	struct list_head entry;
	u16 len;
};

struct hentry{
	struct list_head entry;
	struct lbuff_head *head;
	u16 len;
	u8 state;
	char ref;
};

/*
 * Function: lbuf_init
 *
 *  buf[                                                     ]
 *       (struct hfree)|
 *       (lbuff_head)  |(struct hfree free)|<------len----->|
 *        head,free    |
 *
 */

struct lbuff_head * lbuf_init(void *buf, u32 buf_len)
{
	struct hfree *free;
	struct lbuff_head *head = buf;

	ASSERT((((u32)buf) & 0x03)==0, "%x\n", buf);

	free = (struct hfree *)(head+1);
	free->len = buf_len - sizeof(*head)-sizeof(*free);

	INIT_LIST_HEAD(&head->head);
	INIT_LIST_HEAD(&head->free);

	list_add_tail(&free->entry, &head->free);

	return head;
}

u32 lbuf_remain_len(struct lbuff_head *head, u32 len)
{
	struct hentry *entry;
	struct hfree *p;
	u32 res = 0;

    CPU_SR_ALLOC();

	len += sizeof(*entry);
	if (len&0x03){
		len += 4-(len&0x03);
	}

	CPU_INT_DIS();

	list_for_each_entry(p, &head->free, entry)
	{
		if(p->len < len)
		{
			continue;
		}
		else
		{
			res = 1;	
			break;
		}
	}

	CPU_INT_EN();

	return res;
}

/*
 * Function: lbuf_alloc
 *
 *  buf[                                                                    ]
 *       (struct hfree)|
 *       (lbuff_head)  |(free)|<------------------remain------------------>|
 *        head,free    |                   |
 *                     |                   |<----len--->|(new)|<--remain-->|
 *                     |                   |
 *
 */
void * lbuf_alloc(struct lbuff_head *head, u32 len)
{
	struct hentry *entry;
	struct hfree *p;
	struct hfree *new;
	void *ret = NULL;
    CPU_SR_ALLOC();


	len += sizeof(*entry);
	if (len&0x03){
		len += 4-(len&0x03);
	}

	CPU_INT_DIS();

	list_for_each_entry(p, &head->free, entry)
	{
		if (p->len < len){
			continue;
		}
		if (p->len > len+sizeof(struct hfree)){
			new = (struct hfree *)((u8 *)p + len);
			new->len = p->len - len;
			__list_add(&new->entry, p->entry.prev, p->entry.next);
		} else {
			__list_del_entry(&p->entry);
		}

		entry = (struct hentry*)p;
		entry->head = head;
		entry->state = 0;
		entry->len = len;
		entry->ref = 0;
		INIT_LIST_HEAD(&entry->entry);

		ret = entry+1;
		break;
	}
	if (ret == NULL){
		/*puts("alloc-err\n");*/
		putchar('#');
	}

	CPU_INT_EN();
	return ret;
}

void *lbuf_realloc(void *lbuf, int size)
{
	struct hentry *new;
	struct hentry *entry = __get_entry(lbuf);

	if (size&0x03){
		size += 4-(size&0x03);
	}
	ASSERT(size < entry->len);
	if (size >= entry->len){
		return NULL;
	}
	if (size + sizeof(*new) < entry->len){
		new = (struct hentry *)((u8 *)lbuf + size);
		new->head = entry->head;
		new->len = entry->len - sizeof(*entry) - size;
		new->ref = 0;
		INIT_LIST_HEAD(&new->entry);
		entry->len = sizeof(*entry)+size;

		lbuf_free(new+1);
	}
	return lbuf;
}

int lbuf_empty(struct lbuff_head *head)
{
	if (list_empty(&head->head)){
		return 1;
	}
	return 0;
}


void lbuf_clear(struct lbuff_head *head)
{
	struct hentry *p, *n;
	CPU_SR_ALLOC();

	CPU_INT_DIS();

	list_for_each_entry_safe(p, n, &head->head, entry) {
		lbuf_free(p+1);
	}

	CPU_INT_EN();
}

void lbuf_push(void *lbuf)
{
	struct hentry *p = __get_entry(lbuf);
	CPU_SR_ALLOC();

	CPU_INT_DIS();

	p->state = 0;
	if(p->ref++ == 0){
		list_add_tail(&p->entry, &p->head->head);
	}

	CPU_INT_EN();
}

u8 lbuf_have_next(struct lbuff_head *head)
{
	struct hentry *p;
	CPU_SR_ALLOC();

	CPU_INT_DIS();

	list_for_each_entry(p, &head->head, entry)
	{
		if (p->state == 0){
			CPU_INT_EN();
			return 1;
		}
	}

	CPU_INT_EN();
	return 0;
}

void *lbuf_pop(struct lbuff_head *head)
{
	struct hentry *p;
	CPU_SR_ALLOC();

	CPU_INT_DIS();
	/*if (list_empty(&head->head)){
		return NULL;
	}*/

	list_for_each_entry(p, &head->head, entry)
	{
		if (p->state == 0){
			p->state = 1;
			CPU_INT_EN();
			return p+1;
		}
	}

	CPU_INT_EN();
	return NULL;
}

void lbuf_free(void *lbuf)
{
	struct hfree *p;
	struct hfree *new;
	struct hfree *prev = NULL;
	struct hentry *entry;
	struct lbuff_head *head;
    CPU_SR_ALLOC();

	if(lbuf == NULL){
		return;
	}

	entry = __get_entry(lbuf);
	new = (struct hfree *)entry;
	head = entry->head;

	CPU_INT_DIS();

	/*if (--entry->ref > 0){*/
		/*puts("ref > 0\n");*/
		/*goto _exit;*/
	/*}*/

	__list_del_entry(&entry->entry);
	new->len = entry->len;

	list_for_each_entry(p, &head->free, entry)
	{
		if ((p <= new) && ((u8 *)p+p->len > (u8*)new)){
			printf("free-err: %x\n", lbuf);
			goto _exit;
		}
		if (p > new){
			__list_add(&new->entry, p->entry.prev, &p->entry);
			goto __free;
		}
	}
	list_add_tail(&new->entry, &head->free);

__free:
	list_for_each_entry(p, &head->free, entry)
	{
		if (prev == NULL){
			prev = p;
			continue;
		}

		if ((u32)prev+prev->len == (u32)p){
			prev->len += p->len;
			__list_del_entry(&p->entry);
		} else {
			prev = p;
		}
	}

_exit:
	CPU_INT_EN();
}

