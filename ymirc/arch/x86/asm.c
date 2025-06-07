#include "asm.h"

uintptr_t read_cr0(void) {
  uintptr_t cr0;
  __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
  return cr0;
}

uintptr_t read_cr3(void) {
  uintptr_t cr3;
  __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
  return cr3;
}

void load_cr3(uintptr_t value) {
  __asm__ volatile("mov %0, %%cr3" : : "r"(value));
}

uintptr_t read_cr4(void) {
  uintptr_t cr4;
  __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
  return cr4;
}

uint64_t read_msr(uint32_t msr) {
  uint32_t eax;
  uint32_t edx;
  __asm__ volatile("rdmsr" : "=a"(eax), "=d"(edx) : "c"(msr));
  return ((uint64_t)edx << 32) | eax;
}

void write_msr(uint32_t msr, uint64_t value) {
  __asm__ volatile("wrmsr"
                   :
                   : "c"(msr), "a"((uint32_t)value),
                     "d"((uint32_t)(value >> 32)));
}

uint16_t read_seg_selector(Segment seg) {
  uint16_t ret;

  switch (seg) {
    case SEGMENT_CS:
      __asm__ volatile("mov %%cs, %0" : "=r"(ret));
      break;
    case SEGMENT_SS:
      __asm__ volatile("mov %%ss, %0" : "=r"(ret));
      break;
    case SEGMENT_DS:
      __asm__ volatile("mov %%ds, %0" : "=r"(ret));
      break;
    case SEGMENT_ES:
      __asm__ volatile("mov %%es, %0" : "=r"(ret));
      break;
    case SEGMENT_FS:
      __asm__ volatile("mov %%fs, %0" : "=r"(ret));
      break;
    case SEGMENT_GS:
      __asm__ volatile("mov %%gs, %0" : "=r"(ret));
      break;
    case SEGMENT_TR:
      __asm__ volatile("str %0" : "=r"(ret));
      break;
    case SEGMENT_LDTR:
      __asm__ volatile("sldt %0" : "=r"(ret));
      break;
  }

  return ret;
}

SgdtRet sgdt() {
  SgdtRet gdtr;
  __asm__ volatile("sgdt %0" : "=m"(gdtr));
  return gdtr;
}

SidtRet sidt() {
  SidtRet idtr;
  __asm__ volatile("sidt %0" : "=m"(idtr));
  return idtr;
}
