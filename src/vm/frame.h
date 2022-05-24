#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <stdbool.h>
#include "threads/synch.h"

struct frame
{
	struct lock lock;
	struct page *page;
	void *base;
};

void frame_init(void);
static struct frame *try_frame_alloc_and_lock(struct page *);

void frame_lock(struct page *);
void frame_unlock(struct frame *);
void frame_free(struct frame *);

#endif