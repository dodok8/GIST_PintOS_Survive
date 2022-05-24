#include "vm/frame.h"
#include "vm/page.h"
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

struct frame *
frame_alloc_and_lock(struct page *page)
{
	size_t cnt;

	// FIXME: This loop is useless
	for (cnt = 0; cnt < 3; cnt++)
	{
		struct frame *f = try_frame_alloc_and_lock(page);
		if (f != NULL)
		{
			ASSERT(lock_held_by_current_thread(&f->lock));
			return f;
		}
		timer_msleep(1000);
	}

	return NULL;
}

void frame_lock(struct page *page)
{
	struct frame *frm = page->frame;
	if (frm != NULL)
	{
		lock_acquire(&frm->lock);
		// NOTE: Why do we need to check this?
		if (frm != page->frame)
		{
			lock_release(&frm->lock);
			ASSERT(page->frame == NULL);
		}
	}
}

void frame_free(struct frame *frm)
{
	ASSERT(lock_held_by_current_thread(&frm->lock));

	frm->page = NULL;
	lock_release(&frm->lock);
}

void frame_unlock(struct frame *frm)
{
	ASSERT(lock_held_by_current_thread(&frm->lock));
	lock_release(&frm->lock);
}