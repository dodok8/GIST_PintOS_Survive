#include "vm/frame.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/loader.h"

struct lock scan;
static struct frame *frames;
static size_t frm_cnt;

void frame_init(void)
{
	lock_init(&scan);
	void *base;

	frames = malloc(sizeof *frames * init_ram_pages);
	if (frames == NULL)
		PANIC("frame_init: malloc failed");

	while ((base = palloc_get_page(PAL_USER)) != NULL)
	{
		struct frame *f = &frames[frm_cnt++];
		lock_init(&f->lock);
		f->base = base;
		f->page = NULL;
	}
}