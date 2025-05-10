#include "bin_allocator.h"

#include <assert.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "log.h"
#include "mem.h"
#include "page_allocator_if.h"

#define STUB_PAGE_CAP 16
static alignas(PAGE_SIZE) uint8_t arena[STUB_PAGE_CAP][PAGE_SIZE];
static int index = 0;

static void *stub_alloc_page(size_t n) {
  void *ret = arena[index];
  index += (n + PAGE_SIZE - 1) / PAGE_SIZE;
  return ret;
}

static void stub_free_page(void *ptr, size_t n) {
  // do nothing
  (void)ptr;
  (void)n;
}

static const page_allocator_ops_t stub_pa = {
    .alloc = stub_alloc_page,
    .free = stub_free_page,
    .alloc_aligned_pages = NULL,
};

void log_output(char c) { putchar(c); }

int main() {
  log_set_writefn(log_output);

  init_bin_allocator(&stub_pa);

  assert(bin_alloc(0x4) == arena[0] + 0x0);
  assert(bin_alloc(0x4) == arena[0] + 0x20);
  assert(bin_alloc(0x4) == arena[0] + 0x40);
  bin_free(arena[0] + 0x20, 0x4);
  assert(bin_alloc(0x4) == arena[0] + 0x20);
  assert(bin_alloc(0x800) == arena[1] + 0x0);
  assert(bin_alloc(0x800) == arena[1] + 0x800);
  assert(bin_alloc(0x800) == arena[2] + 0x0);
  assert(bin_alloc(0x1500) == arena[3] + 0x0);
  assert(bin_alloc(0x40) == arena[5] + 0x0);

  puts("PASS");

  return 0;
}
