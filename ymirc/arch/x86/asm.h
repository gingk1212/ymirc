#pragma once

#include <stdint.h>

typedef union {
  struct {
    unsigned int pe : 1;
    unsigned int mp : 1;
    unsigned int em : 1;
    unsigned int ts : 1;
    unsigned int et : 1;
    unsigned int ne : 1;
    unsigned int reserved1 : 10;
    unsigned int wp : 1;
    unsigned int reserved2 : 1;
    unsigned int am : 1;
    unsigned int reserved3 : 10;
    unsigned int nw : 1;
    unsigned int cd : 1;
    unsigned int pg : 1;
    unsigned int reserved4 : 32;
  };
  uint64_t value;
} __attribute__((packed)) Cr0;

typedef union {
  struct {
    unsigned int sce : 1;
    unsigned int reserved1 : 7;
    unsigned int lme : 1;
    unsigned int reserved2 : 1;
    unsigned int lma : 1;
    unsigned int nxe : 1;
    unsigned int svme : 1;
    unsigned int lmsle : 1;
    unsigned int ffxsr : 1;
    unsigned int tce : 1;
    unsigned int reserved3 : 1;
    unsigned int mcommit : 1;
    unsigned int intwb : 1;
    unsigned int reserved : 1;
    unsigned int uaie : 1;
    unsigned int aibrse : 1;
    uint64_t reserved4 : 42;
  };
  uint64_t value;
} __attribute__((packed)) Efer;

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
