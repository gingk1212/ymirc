#include "page_allocator.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "log.h"

void log_output(char c) { putchar(c); }
void log_no_output(char c) { (void)c; }

MemoryMap* create_memory_map() {
  MemoryMap* map = malloc(sizeof(MemoryMap));
  map->descriptor_size = sizeof(EFI_MEMORY_DESCRIPTOR);
  map->map_size = map->descriptor_size * 5;  // Example size
  map->descriptors = malloc(map->map_size);
  map->descriptors[0] = (EFI_MEMORY_DESCRIPTOR){
      .Type = EfiConventionalMemory,
      .PhysicalStart = 0x0,
      .NumberOfPages = 1,
  };
  map->descriptors[1] = (EFI_MEMORY_DESCRIPTOR){
      .Type = EfiConventionalMemory,
      .PhysicalStart = 0x1000,
      .NumberOfPages = 1,
  };
  map->descriptors[2] = (EFI_MEMORY_DESCRIPTOR){
      .Type = EfiConventionalMemory,
      .PhysicalStart = 0x3000,
      .NumberOfPages = 1,
  };
  map->descriptors[3] = (EFI_MEMORY_DESCRIPTOR){
      .Type = EfiReservedMemoryType,
      .PhysicalStart = 0x4000,
      .NumberOfPages = 1,
  };
  map->descriptors[4] = (EFI_MEMORY_DESCRIPTOR){
      .Type = EfiConventionalMemory,
      .PhysicalStart = 0x5000,
      .NumberOfPages = 10,
  };
  return map;
}

int main() {
  log_set_writefn(log_output);
  MemoryMap* map = create_memory_map();
  page_allocator_init(map);

  assert(pa_ops.alloc(0x1000) == (void*)0x1000);  // Frame ID 0 is reserved.
  assert(pa_ops.alloc(0x1000) == (void*)0x3000);
  assert(pa_ops.alloc(0x1000) == (void*)0x5000);
  pa_ops.free((void*)0x1000, 0x1000);
  assert(pa_ops.alloc(0x1000) == (void*)0x1000);
  pa_ops.free((void*)0x1000, 0x1000);
  pa_ops.free((void*)0x3000, 0x1000);
  pa_ops.free((void*)0x5000, 0x1000);

  assert(pa_ops.alloc(0x4000) == (void*)0x5000);
  assert(pa_ops.alloc(0x4000) == (void*)0x9000);
  assert(pa_ops.alloc(0x4000) == NULL);
  pa_ops.free((void*)0x5000, 0x4000);
  pa_ops.free((void*)0x9000, 0x4000);

  assert(pa_ops.alloc_aligned_pages(1, 0x2000) == (void*)0x6000);
  pa_ops.free((void*)0x6000, 0x1000);

  log_set_writefn(log_no_output);
  // align size must be multiple of page size.
  assert(pa_ops.alloc_aligned_pages(1, 0x1100) == NULL);

  return 0;
}
