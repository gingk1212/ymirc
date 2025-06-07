#pragma once

#include <stdint.h>

uintptr_t read_cr0(void);
uintptr_t read_cr3(void);
void load_cr3(uintptr_t value);
uintptr_t read_cr4(void);
uint64_t read_msr(uint32_t msr);
void write_msr(uint32_t msr, uint64_t value);

typedef enum {
  SEGMENT_CS,
  SEGMENT_SS,
  SEGMENT_DS,
  SEGMENT_ES,
  SEGMENT_FS,
  SEGMENT_GS,
  SEGMENT_TR,
  SEGMENT_LDTR,
} Segment;

uint16_t read_seg_selector(Segment seg);

typedef struct {
  uint16_t limit;
  uint64_t base;
} __attribute((packed)) SgdtRet;

SgdtRet sgdt();

typedef struct {
  uint16_t limit;
  uint64_t base;
} __attribute((packed)) SidtRet;

SidtRet sidt();
