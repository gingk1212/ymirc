#include "page_allocator.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "bits.h"
#include "log.h"

/** Page size. */
#define PAGE_SIZE (4096ULL)
/** Page mask. */
#define PAGE_MASK (PAGE_SIZE - 1)
/** Maximum physical memory size in bytes that can be managed by this allocator.
 */
#define MAX_PHYSICAL_SIZE (128ULL * 1024 * 1024 * 1024)  // 128GiB
/** Maximum page frame count. */
#define FRAME_COUNT (MAX_PHYSICAL_SIZE / PAGE_SIZE)  // 32Mi frames
/** Single unit of bitmap line. */
typedef uint64_t MapLineType;
/** Bits per map line. */
#define BITS_PER_MAPLINE (sizeof(MapLineType) * 8)  // 64 bits
/** Number of map lines. */
#define NUM_MAPLINES (FRAME_COUNT / BITS_PER_MAPLINE)  // 512Ki lines

/** Bitmap. */
static MapLineType bitmap[NUM_MAPLINES] = {0};

typedef uint64_t Phys;
typedef uint64_t Virt;
typedef uint64_t FrameId;

/** First frame ID. Frame ID 0 is reserved. */
static FrameId frame_begin = 1;
/** First frame ID that is not managed by this allocator. */
static FrameId frame_end;

typedef enum {
  // Page frame is in use.
  used,
  // Page frame is unused.
  unused,
} FrameStatus;

static FrameStatus frame_status_at(FrameId frame) {
  int line_index = frame / BITS_PER_MAPLINE;
  int bit_index = frame % BITS_PER_MAPLINE;
  return (bitmap[line_index] & tobit(bit_index)) ? used : unused;
}

static void set_frame_status(FrameId frame, FrameStatus status) {
  int line_index = frame / BITS_PER_MAPLINE;
  int bit_index = frame % BITS_PER_MAPLINE;
  if (status == used) {
    bitmap[line_index] |= tobit(bit_index);
  } else {
    bitmap[line_index] &= ~tobit(bit_index);
  }
}

static void mark_allocated(FrameId frame, uint64_t num_frames) {
  for (uint64_t i = 0; i < num_frames; i++) {
    set_frame_status(frame + i, used);
  }
}

static void mark_not_used(FrameId frame, uint64_t num_frames) {
  for (uint64_t i = 0; i < num_frames; i++) {
    set_frame_status(frame + i, unused);
  }
}

static inline FrameId phys2frame(Phys phys) { return phys / PAGE_SIZE; }
static inline Phys frame2phys(FrameId frame) { return frame * PAGE_SIZE; }
static inline Phys virt2phys(uintptr_t addr) { return addr; }
static inline Virt phys2virt(uintptr_t addr) { return addr; }

/** Check if the memory region described by the descriptor is usable for ymirc
 * kernel.
 * Note that these memory areas may contain crucial data for the kernel,
 * including page tables, stack, and GDT.
 * You MUST copy them before using the area. */
static inline bool is_usable_memory(EFI_MEMORY_DESCRIPTOR *desc) {
  return desc->Type == EfiConventionalMemory ||
         desc->Type == EfiBootServicesCode;
}

/** Initialize the allocator.
 * This function MUST be called before the direct mapping w/ offset 0x0 is
 * unmapped. */
void page_allocator_init(MemoryMap *map) {
  Phys phys_end = 0;
  Phys avail_end = 0;
  for (uint64_t i = 0; i < map->map_size / map->descriptor_size; i++) {
    EFI_MEMORY_DESCRIPTOR *desc =
        (EFI_MEMORY_DESCRIPTOR *)((uint8_t *)map->descriptors +
                                  i * map->descriptor_size);
    // Mark holes between regions as allocated (used).
    if (phys_end < desc->PhysicalStart) {
      if (desc->PhysicalStart >= MAX_PHYSICAL_SIZE) {
        mark_allocated(phys2frame(phys_end),
                       (MAX_PHYSICAL_SIZE - phys_end) / PAGE_SIZE);
        break;
      } else {
        mark_allocated(phys2frame(phys_end),
                       (desc->PhysicalStart - phys_end) / PAGE_SIZE);
      }
    }
    phys_end = desc->PhysicalStart + desc->NumberOfPages * PAGE_SIZE;
    // Mark the region described by the descriptor as used or unused.
    if (phys_end >= MAX_PHYSICAL_SIZE) {
      // If the end address exceeds the maximum physical memory, mark up to the
      // maximum as used.
      mark_allocated(phys2frame(desc->PhysicalStart),
                     (MAX_PHYSICAL_SIZE - desc->PhysicalStart) / PAGE_SIZE);
      break;
    } else if (is_usable_memory(desc)) {
      avail_end = phys_end;
      mark_not_used(phys2frame(desc->PhysicalStart), desc->NumberOfPages);
    } else {
      mark_allocated(phys2frame(desc->PhysicalStart), desc->NumberOfPages);
    }
  }
  frame_end = phys2frame(avail_end);
}

void *mem_alloc(size_t n) {
  size_t num_frames = (n + PAGE_SIZE - 1) / PAGE_SIZE;
  FrameId start_frame = frame_begin;
  while (1) {
    size_t i = 0;
    for (; i < num_frames; i++) {
      if (start_frame + i >= frame_end) return NULL;
      if (frame_status_at(start_frame + i) == used) break;
    }
    if (i == num_frames) {
      mark_allocated(start_frame, num_frames);
      return (void *)(uintptr_t)phys2virt(frame2phys(start_frame));
    }
    start_frame += i + 1;
  }
}

// FIXME: Size should not be passed.
// FIXME: Check if the memory region is usable for ymirc kernel.
void mem_free(void *ptr, size_t n) {
  size_t num_frames = (n + PAGE_SIZE - 1) / PAGE_SIZE;
  Virt start_frame_vaddr = (Virt)(uintptr_t)ptr & ~PAGE_MASK;
  FrameId start_frame = phys2frame(virt2phys(start_frame_vaddr));
  if (start_frame + num_frames > frame_end) {
    LOG_ERROR("Invalid free: %p\n", ptr);
    return;
  }
  mark_not_used(start_frame, num_frames);
}

/** Allocate physically contiguous and aligned pages. */
void *mem_alloc_pages(size_t num_pages, size_t align_size) {
  size_t num_frames = num_pages;
  size_t align_frame = (align_size + PAGE_SIZE - 1) / PAGE_SIZE;
  FrameId start_frame = align_frame;
  while (1) {
    size_t i = 0;
    for (; i < num_frames; i++) {
      if (start_frame + i >= frame_end) return NULL;
      if (frame_status_at(start_frame + i) == used) break;
    }
    if (i == num_frames) {
      mark_allocated(start_frame, num_frames);
      return (void *)(uintptr_t)phys2virt(frame2phys(start_frame));
    }
    start_frame += align_frame;
    if (start_frame + num_frames >= frame_end) return NULL;
  }
}
