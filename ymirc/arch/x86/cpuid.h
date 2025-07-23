#pragma once

#include <assert.h>
#include <stdint.h>

/** Return value of CPUID. */
typedef struct {
  uint32_t eax;
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;
} CpuidRegisters;

/** Asm CPUID instruction. */
CpuidRegisters cpuid(uint32_t leaf, uint32_t subleaf);

/** CPUID Feature Flags bitfield for ECX.
 * Leaf=1, Sub-Leaf=null, */
typedef union {
  struct {
    unsigned int sse3 : 1;
    unsigned int pclmulqdq : 1;
    unsigned int _reserved1 : 1;
    unsigned int monitor : 1;
    unsigned int _reserved2 : 5;
    unsigned int ssse3 : 1;
    unsigned int _reserved3 : 2;
    unsigned int fma : 1;
    unsigned int cmpxchg16b : 1;
    unsigned int _reserved4 : 5;
    unsigned int sse41 : 1;
    unsigned int sse42 : 1;
    unsigned int x2apic : 1;
    unsigned int movbe : 1;
    unsigned int popcnt : 1;
    unsigned int _reserved5 : 1;
    unsigned int aes : 1;
    unsigned int xsave : 1;
    unsigned int osxsave : 1;
    unsigned int avx : 1;
    unsigned int f16c : 1;
    unsigned int rdrand : 1;
    unsigned int _reserved6 : 1;
  };
  uint32_t value;
} __attribute__((packed)) CpuidFeatureInfoEcx;

static_assert(sizeof(CpuidFeatureInfoEcx) == 4,
              "Unexpected CpuidFeatureInfoEcx size");

/** CPUID Feature Flags bitfield for EDX.
 * Leaf=1, Sub-Leaf=null, */
typedef union {
  struct {
    unsigned int fpu : 1;
    unsigned int vme : 1;
    unsigned int de : 1;
    unsigned int pse : 1;
    unsigned int tsc : 1;
    unsigned int msr : 1;
    unsigned int pae : 1;
    unsigned int mce : 1;
    unsigned int cmpxchg8b : 1;
    unsigned int apic : 1;
    unsigned int _reserved1 : 1;
    unsigned int sysentersysexit : 1;
    unsigned int mtrr : 1;
    unsigned int pge : 1;
    unsigned int mca : 1;
    unsigned int cmov : 1;
    unsigned int pat : 1;
    unsigned int pse36 : 1;
    unsigned int _reserved2 : 1;
    unsigned int clfsh : 1;
    unsigned int _reserved3 : 3;
    unsigned int mmx : 1;
    unsigned int fxsr : 1;
    unsigned int sse : 1;
    unsigned int sse2 : 1;
    unsigned int _reserved4 : 1;
    unsigned int htt : 1;
    unsigned int _reserved5 : 3;
  };
  uint32_t value;
} __attribute__((packed)) CpuidFeatureInfoEdx;

static_assert(sizeof(CpuidFeatureInfoEdx) == 4,
              "Unexpected CpuidFeatureInfoEdx size");

/** CPUID Extended Feature Flags bitfield for EBX.
 * Leaf=7, Sub-Leaf=0, */
typedef union {
  struct {
    unsigned int fsgsbase : 1;
    unsigned int tscadjust : 1;
    unsigned int _reserved1 : 1;
    unsigned int bmi1 : 1;
    unsigned int _reserved2 : 1;
    unsigned int avx2 : 1;
    unsigned int _reserved3 : 1;
    unsigned int smep : 1;
    unsigned int bmi2 : 1;
    unsigned int erms : 1;
    unsigned int invpcid : 1;
    unsigned int _reserved4 : 1;
    unsigned int pqm : 1;
    unsigned int _reserved5 : 2;
    unsigned int pqe : 1;
    unsigned int avx512f : 1;
    unsigned int avx512dq : 1;
    unsigned int rdseed : 1;
    unsigned int adx : 1;
    unsigned int smap : 1;
    unsigned int avx512_ifma : 1;
    unsigned int _reserved6 : 1;
    unsigned int clflushopt : 1;
    unsigned int clwb : 1;
    unsigned int _reserved7 : 3;
    unsigned int avx512cd : 1;
    unsigned int sha : 1;
    unsigned int avx512bw : 1;
    unsigned int avx512vl : 1;
  };
  uint32_t value;
} __attribute__((packed)) CpuidExtFeatureEbx0;

static_assert(sizeof(CpuidExtFeatureEbx0) == 4,
              "Unexpected CpuidExtFeatureEbx0 size");
