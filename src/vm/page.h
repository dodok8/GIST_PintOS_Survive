#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include "devices/block.h"
#include "filesys/off_t.h"
#include "threads/synch.h"

struct page
{
  void *addr;
  bool read_only;
  struct thread *thread;
  
  struct hash_elem hash_elem;
  
  struct frame *frame;
  
  block_sector_t sector;

  bool private;

  struct file *file;
  off_t file_offset;
  off_t file_bytes;
};

void page_exit(void);

struct page *page_allocate(void *, bool);
void page_deallocate(void *);

bool page_in(void *);
bool page_out(struct page *);
bool page_accessed_recently(struct page *);

bool page_lock(const void *, bool);
void page_unblock(const void *);

unsigned page_hash(const struct hash_elem *e, void *aux);
bool page_less(const struct hash_elem *a,
                             const struct hash_elem *b,
                             void *aux);

#endif
