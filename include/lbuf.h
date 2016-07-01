#ifndef LBUF_H
#define LBUF_H

#include "typedef.h"
#include "list.h"


struct lbuff_head{
	struct list_head head;
	struct list_head free;
	//u16 index;
};



struct lbuff_head *lbuf_init(void *buf, u32 buf_len);

void *lbuf_alloc(struct lbuff_head *head, u32 len);

void *lbuf_realloc(void *lbuf, int size);

void lbuf_push(void *lbuf);

void *lbuf_pop(struct lbuff_head *head);

void lbuf_free(void *lbuf);

int lbuf_empty(struct lbuff_head *head);

void lbuf_clear(struct lbuff_head *head);

u8 lbuf_have_next(struct lbuff_head *head);

#endif

