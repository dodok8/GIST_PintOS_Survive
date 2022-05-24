#include "vm/frame.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/loader.h"

struct lock scan;
static struct frame *frames;
static size_t frm_cnt;
static size_t hand;

void frame_init(void)
{
	lock_init(&scan);
	void *base;

	frames = malloc(sizeof *frames * init_ram_pages);
	if (frames == NULL)
		PANIC("frame_init: malloc failed");

	while ((base = palloc_get_page(PAL_USER)) != NULL)
	{
		struct frame *frm = &frames[frm_cnt++];
		lock_init(&frm->lock);
		frm->base = base;
		frm->page = NULL;
	}
}

static struct frame *try_frame_alloc_and_lock(struct page *page)
{
	size_t idx;

	lock_acquire(&scan);

	for (idx = 0; idx < frm_cnt; idx++)
	{
		struct frame *frm = &frames[idx];
		lock_acquire(&frm->lock);
		if (!lock_try_acquire(&frm->lock))
			continue;
		if (frm->page == NULL)
		{
			frm->page = page;
			lock_release(&scan);
			return frm;
		}
		lock_release(&frm->lock);
	}

	for (idx = 0; idx < frm_cnt * 2; idx++)
	{
		struct frame *frm = &frames[hand];
		if (hand + 1 >= frm_cnt)
			hand = 0;

		if (!lock_try_acquire(&frm->lock))
			continue;

		// FIXME: this is unused.
		if (frm->page == NULL)
		{
			frm->page = page;
			lock_release(&scan);
			return frm;
		}

		if (page_accessed_recently(frm->page))
		{
			lock_release(&frm->lock);
			continue;
		}

		lock_release(&scan);

		/* Evict this frame. */
		if (!page_out(frm->page))
		{
			lock_release(&frm->lock);
			return NULL;
		}

		frm->page = page;
		return frm;
	}

	lock_release(&scan);
	return NULL;
}