#include "bmalloc.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define MIN_BLOCK_SIZE 16
#define INIT_BLOCK_SIZE 4096

static bm_header_ptr head = NULL;
static bm_header bm_list_head = {0, 0, 0};
int flag = PROT_READ | PROT_WRITE;
int map_flag = MAP_ANONYMOUS | MAP_PRIVATE;
bm_option opt;

int exponent(int n) {
  int exp = 0;
  while (n > 1) {
    exp++;
    n /= 2;
  }
  return exp;
}

void *sibling(void *h, int size) {
  if (h == NULL || size == 0)
    return NULL;
  intptr_t block_addr = (intptr_t)h;
  intptr_t block_size = 1 << size;
  intptr_t block_offset = block_addr - (intptr_t)head;
  intptr_t sibling_offset = block_offset ^ block_size;
  return (bm_header_ptr)((intptr_t)head + sibling_offset);
}


int fitting(size_t s) {
  int block_size = exponent(MIN_BLOCK_SIZE);
  while (block_size <= exponent(INIT_BLOCK_SIZE)) {
    if (s <= ((1 << block_size) - sizeof(bm_header))) {
      return (1 << block_size);
    }
    block_size++;
  }
  return -1; // no suitable block size found
}

void *find_best_fit(size_t s) {
  bm_header_ptr best_block = NULL;
  bm_header_ptr block = bm_list_head.next;
  while (block != NULL) {
    if (block->used == 0 && (1 << block->size) >= s) {
      if (best_block == NULL || (1 << block->size) < (1 << best_block->size)) {
        best_block = block;
      }
    }
    block = block->next;
  }
  return best_block;
}

void *find_first_fit(size_t s) {
  bm_header_ptr block = bm_list_head.next;
  bm_header_ptr best_block = NULL;
  while (block != NULL) {
    if (block->used == 0 && (1 << block->size) >= s) {
      if (best_block == NULL || (1 << block->size) < (1 << best_block->size)) {
        best_block = block;
      }
    }
    block = block->next;
  }
  return best_block;
}

void *split(bm_header_ptr block, size_t target_size) {
  while ((1 << block->size) >= (target_size + sizeof(bm_header))) {
    size_t old_size = 1 << block->size;
    block->size--;

    // Check if buddy block will fit within the allocated memory
    if ((intptr_t)block + (1 << block->size) >=
        (intptr_t)head + INIT_BLOCK_SIZE) {
      block->size++;
      break;
    }

    bm_header_ptr buddy = (bm_header_ptr)((intptr_t)block + (1 << block->size));
    buddy->used = 0;
    buddy->size = block->size;
    buddy->next = block->next;
    block->next = buddy;

    if (head == block) { // Update head pointer if block is the first block
      head = buddy;
    }
  }

  block->used = 1;
  return (void *)(block + 1);
}

void *jsiling(void *h) {

  size_t block_size = ((bm_header_ptr)h)->size;
  bm_header_ptr prev;
  bm_header_ptr i;

  if (block_size == 0 || block_size >= exponent(INIT_BLOCK_SIZE))
    return NULL;

  if (bm_list_head.next == (bm_header_ptr)h) {
    if (block_size == ((bm_header_ptr)h)->next->size) {
      return ((bm_header_ptr)h)->next;

    } else
      return NULL;
  }

  int sum = 1;
  // jaegorithm
  //  add up everything on the list
  //  divide the sum by inital val
  //  perform the modulo opp
  for (i = bm_list_head.next, prev = NULL; i != NULL; prev = i, i = i->next) {
    sum += (1 << i->size);

    if (i == (bm_header_ptr)h) {
      int idx = sum / (1 << i->size);

      if (idx % 2 == 0) {
        if (prev->size == i->size && prev != NULL) {
          return (void *)prev; // give right
        }

        break;
      } else {
        if (i->next->size == i->size && i->next != NULL) {
          return (void *)i->next; // give left
        }
        break;
      }
    }
  }
  return NULL;
}

unsigned int actual_block_size(int size) { return 1 << size; }

void *bmalloc(size_t s) {
  int block_size = exponent(MIN_BLOCK_SIZE);
  while (block_size <= exponent(INIT_BLOCK_SIZE)) {
    if (s <= ((1 << block_size) - sizeof(bm_header))) {
      break;
    }
    block_size++;
  }

  bm_header_ptr best_block;

  if (opt == BestFit) {
    best_block = find_best_fit(1 << block_size);
  } else {
    best_block = find_first_fit(1 << block_size);
  }

  if (best_block == NULL) {
    bm_header_ptr block = mmap(NULL, INIT_BLOCK_SIZE, flag, map_flag, -1, 0);
    if (block == MAP_FAILED) {
      return NULL;
    }
    block->used = 0;
    block->size = exponent(INIT_BLOCK_SIZE);
    block->next = NULL;

    if (head == NULL) {
      head = block;
      bm_list_head.next = block;
    } else {
      block->next = bm_list_head.next;
      bm_list_head.next = block;
    }

    best_block = block;
  }

  return split(best_block, 1 << block_size);
}

void bfree(void *p) {
  if (p == NULL)
    return;

  bm_header_ptr block = (bm_header_ptr)p - 1;
  block->used = 0;

  while (1) {
    bm_header_ptr buddy = jsiling(block);
    if (buddy == NULL || buddy->used == 1 || buddy->size != block->size) {
      break;
    }

    if (block > buddy) {
      bm_header_ptr temp = block;
      block = buddy;
      buddy = temp;
    }

    // Remove buddy from the list
    bm_header_ptr prev = &bm_list_head;
    while (prev->next != NULL && prev->next != buddy) {
      prev = prev->next;
    }
    if (prev->next == buddy) {
      
      prev->next = buddy->next;
    }

    // Coalesce block and buddy
    block->size++;
  }

  
}

void *brealloc(void *p, size_t s) {
  // TODO
  if (p == NULL) {
    return bmalloc(s);
  }

  if (s == 0) {
    bfree(p);
    return NULL;
  }

  bm_header_ptr block = (bm_header_ptr)p - 1;
  size_t block_size = 1 << block->size;
  if (block_size >= s + sizeof(bm_header)) {
    return p;
  }

  void *new_ptr = bmalloc(s);
  if (new_ptr == NULL) {
    return NULL;
  }

  memcpy(new_ptr, p, block_size - sizeof(bm_header));
  bfree(p);

  return new_ptr;
}

void bmconfig(bm_option option) {
  // TODO
  opt = option;
}

void bmprint() {
  bm_header_ptr itr;
  int i;

  // Calculate statistics
  size_t total_mem = 0, user_mem = 0, avail_mem = 0, frag_mem = 0;
  for (itr = bm_list_head.next, i = 0; itr != NULL; itr = itr->next, i++) {
    total_mem += actual_block_size(itr->size);
    if (itr->used) {
      size_t allocated_size = actual_block_size(itr->size) - sizeof(bm_header);
      user_mem += allocated_size;
      size_t req_size = 1 << (itr->size - 1);
      frag_mem += allocated_size - req_size;
    } else {
      avail_mem += actual_block_size(itr->size);
    }
  }

  // Print list header
  printf("==================== bm_list ====================\n");

  // Print block information
  for (itr = bm_list_head.next, i = 0; itr != NULL; itr = itr->next, i++) {
    void *block_addr =
        (void *)((intptr_t)itr + sizeof(bm_header)); // <-- Change this line
    printf("  %d:%p:%d", i, block_addr, itr->used);
    printf(" %8d:%02x %02x %02x %02x %02x %02x %02x %02x\n", itr->size, 0, 0, 0,
           0, 0, 0, 0, 0);
  }

  // Print footer
  printf("=================================================\n");

  // Print statistics
  printf("===================== stats =====================\n");
  printf("total given memory:             %zu\n", total_mem);
  printf("total given memory to user:     %zu\n", user_mem);
  printf("total available memory:         %zu\n", avail_mem);
  printf("total internal fragmentation:   %zu\n", frag_mem);
  printf("=================================================\n");
}
