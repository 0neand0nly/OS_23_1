#include "bmalloc.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define MIN_BLOCK_SIZE 16
#define INIT_BLOCK_SIZE 4096

bm_option bm_mode = BestFit;
static bm_header_ptr head = NULL;
static bm_header bm_list_head = {0, 0, 0};

unsigned int exponent(int n)
{
  int exp = 0;
  while (n > 1)
  {
    exp++;
    n /= 2;
  }
  return exp;
}

unsigned int actual_block_size(int size) { return 1 << size; }

int fitting(size_t s)
{

  int block_size = exponent(MIN_BLOCK_SIZE);
  while (block_size <= exponent(INIT_BLOCK_SIZE))
  {
    if (s <= ((1 << block_size) - sizeof(bm_header)))
    {
      break;
    }
    block_size++;
  }
  return block_size;
}

void *find_best_fit(size_t s)
{
  bm_header_ptr best_block = NULL;
  bm_header_ptr block = bm_list_head.next;
  while (block != NULL)
  {
    if (block->used == 0 && (1 << block->size) >= s)
    {
      if (best_block == NULL || (1 << block->size) < (1 << best_block->size))
      {
        best_block = block;
      }
    }
    block = block->next;
  }
  return best_block;
}

void *find_first_fit(size_t s)
{
  bm_header_ptr block = bm_list_head.next;
  while (block != NULL)
  {
    if (block->used == 0 && (1 << block->size) >= s)
    {
      return block;
    }
    block = block->next;
  }
  return NULL;
}

void *split(bm_header_ptr block, size_t target_size)
{
  while ((1 << block->size) >= (target_size + sizeof(bm_header)))
  {
    block->size--;

    // Check if buddy block will fit within the allocated memory
    if ((void *)block + (1 << block->size) >= (void *)head + INIT_BLOCK_SIZE)
    {
      block->size++;
      break;
    }

    bm_header_ptr buddy = (bm_header_ptr)((void *)block + (1 << block->size));
    buddy->used = 0;
    buddy->size = block->size;
    buddy->next = block->next;
    block->next = buddy;

    if (head == block)
    { // Update head pointer if block is the first block
      head = buddy;
    }
  }
  block->used = 1;

  return (void *)((bm_header_ptr)block + 1);
}

void *sibling(void *h)
{

  size_t block_size = ((bm_header_ptr)h)->size;
  bm_header_ptr prev;
  bm_header_ptr i;

  if (block_size == 0 || block_size >= exponent(INIT_BLOCK_SIZE))
    return NULL;

  if (bm_list_head.next == (bm_header_ptr)h)
  {
    if (block_size == ((bm_header_ptr)h)->next->size)
    {
      return ((bm_header_ptr)h)->next;
    }
    else
      return NULL;
  }

  int sum = 1;
  for (i = bm_list_head.next, prev = NULL; i != NULL; prev = i, i = i->next)
  {
    sum += (1 << i->size);

    if (i == (bm_header_ptr)h)
    {
      int idx = sum / (1 << i->size);

      if (idx % 2 == 0)
      {
        if (prev->size == i->size && prev != NULL)
        {
          return (void *)prev; // give right
        }

        break;
      }
      else
      {
        if (i->next->size == i->size && i->next != NULL)
        {
          return (void *)i->next; // give left
        }
        break;
      }
    }
  }
  return NULL;
}

void *bmalloc(size_t s)
{
  if (s < 1 || s > (INIT_BLOCK_SIZE - sizeof(bm_header) - 1))
  {
    printf("Error: The block size needs to be above 0 and below %zu.\n", (INIT_BLOCK_SIZE - sizeof(bm_header)));
    return NULL;
  }

  int block_size = fitting(s);

  bm_header_ptr best_block;

  if (bm_mode == BestFit)
  {
    best_block = find_best_fit(1 << block_size);
  }
  else
  {
    best_block = find_first_fit(1 << block_size);
  }

  if (best_block == NULL)
  {
    int flag = PROT_READ | PROT_WRITE;
    int map_flag = MAP_ANONYMOUS | MAP_PRIVATE;

    bm_header_ptr block = mmap(NULL, INIT_BLOCK_SIZE, flag, map_flag, -1, 0);
    if (block == MAP_FAILED)
    {
      return NULL;
    }
    block->used = 0;
    block->size = exponent(INIT_BLOCK_SIZE);
    block->next = NULL;

    if (head == NULL)
    {
      head = block;
      bm_list_head.next = block;
    }
    else
    {
      bm_header_ptr current = &bm_list_head;
      while (current->next != NULL)
      {
        current = current->next;
      }
      current->next = block;
    }

    best_block = block;
  }

  bm_header_ptr new_block = split(best_block, 1 << block_size);
  if (new_block != NULL)
  {
#ifdef DEBUG
    printf("Allocated Size: %d\n", actual_block_size(block_size));
    printf("Req Size: %ld\n", s);
#endif
  }

  return new_block;
}

void bfree(void *p)
{
  if (p == NULL)
  {
    return;
  }

  bm_header_ptr block = (bm_header_ptr)((char *)p - sizeof(bm_header));
  int found = 0;

  // Search for the block in the linked list
  bm_header_ptr prev = NULL;
  bm_header_ptr current = bm_list_head.next;
  while (current != NULL)
  {
    if (current == block)
    {
      found = 1;
      break;
    }
    prev = current;
    current = current->next;
  }

  if (!found)
  {
    // The requested memory is not in the linked list
    printf("Error: The requested memory is not found in the linked list.\n");
    return;
  }

  block->used = 0;
  memset(((char *)block) + sizeof(bm_header), 0, (1 << block->size) - sizeof(bm_header));

  // Coalesce blocks if possible
  while (block->size != exponent(INIT_BLOCK_SIZE))
  {
    bm_header_ptr buddy = sibling(block);
    if (buddy == NULL || buddy->used == 1 || buddy->size != block->size)
    {
      break;
    }

    if (block > buddy)
    {
      bm_header_ptr temp = block;
      block = buddy;
      buddy = temp;
    }

    // Remove buddy from the linked list
    if (prev == NULL)
    {
      bm_list_head.next = buddy->next;
    }
    else
    {
      prev->next = buddy->next;
    }

    // Coalesce block and buddy
    block->size++;

    // Check if the block size has reached the maximum size and munmap
    if (block->size == exponent(INIT_BLOCK_SIZE))
    {
      // If the block is the only one left and is not in use, unmap it
      if (bm_list_head.next == NULL && block->used == 0)
      {
        munmap(block, INIT_BLOCK_SIZE);
      }
      break;
    }
  }
}

void *brealloc(void *p, size_t s)
{
  if (p == NULL)
  {
    return bmalloc(s);
  }

  if (s == 0)
  {
    bfree(p);
    return NULL;
  }

  bm_header_ptr block = (bm_header_ptr)p - 1;
  size_t block_size = 1 << block->size;
  size_t min_required_size = s + sizeof(bm_header);

  if (block_size >= min_required_size)
  {
    // If the current block can be split to fit the new size
    if (block_size >= (min_required_size << 1) + sizeof(bm_header))
    {
      bfree(p);
      void *new_ptr = bmalloc(s);
      memcpy(new_ptr, p, s);
      return new_ptr;
    }
    return p;
  }

  void *new_ptr = bmalloc(s);
  if (new_ptr == NULL)
  {
    return NULL;
  }

  memcpy(new_ptr, p, block_size - sizeof(bm_header));
  bfree(p);

  return new_ptr;
}

void bmconfig(bm_option opt)
{
  bm_mode = opt;
}

void bmprint()
{
  bm_header_ptr itr;
  int i;

  // Calculate statistics
  size_t total_mem = 0, user_mem = 0, avail_mem = 0;
  for (itr = bm_list_head.next, i = 0; itr != NULL; itr = itr->next, i++)
  {
    total_mem += actual_block_size(itr->size);
    if (itr->used)
    {
      size_t allocated_size = actual_block_size(itr->size);
      user_mem += allocated_size;
      size_t req_size = 1 << (itr->size - 1);
    }
    else
    {
      avail_mem += actual_block_size(itr->size);
    }
  }

  printf("==================== bm_list ====================\n");
  for (itr = bm_list_head.next, i = 0; itr != 0x0; itr = itr->next, i++)
  {
    size_t payload_size = (1 << itr->size) - sizeof(bm_header);
    printf("%3d:%p:%1d %8d %8zu:", i, ((void *)itr) + sizeof(bm_header),
           (int)itr->used, (int)itr->size, payload_size);

    int j;
    char *s = ((char *)itr) + sizeof(bm_header);
    for (j = 0; j < (itr->size >= 8 ? 8 : itr->size); j++)
      printf("%02x ", s[j]);
    printf("\n");
  }
  printf("=================================================\n");

  // Print footer
  printf("=================================================\n");

  // Print statistics
  printf("===================== stats =====================\n");
  printf("total given memory:             %zu\n", total_mem);
  printf("total given memory to user:     %zu\n", user_mem);
  printf("total available memory:         %zu\n", avail_mem);
  // printf("total internal fragmentation:   %u\n", total_internal_frag);
  printf("=================================================\n");
}
