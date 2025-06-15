#pragma once

#include <stdint.h>

typedef struct {
  uint16_t sel;
  uint16_t attr;
  uint32_t limit;
  uint64_t base;
} __attribute__((packed)) VmcbSeg;

typedef struct {
  /* VMCB Control Area */
  /* 0x000 */
  uint16_t intercept_read_cr;
  uint16_t intercept_write_cr;
  /* 0x004 */
  uint16_t intercept_read_dr;
  uint16_t intercept_write_dr;
  /* 0x008 */
  uint32_t intercept_exception;
  /* 0x00C */
  unsigned int intercept_intr : 1; /* 0 */
  unsigned int intercept_nmi : 1;
  unsigned int intercept_smi : 1;
  unsigned int intercept_init : 1;
  unsigned int intercept_vintr : 1; /* 4 */
  unsigned int intercept_cr0_ts_or_mp : 1;
  unsigned int intercept_rd_idtr : 1;
  unsigned int intercept_rd_gdtr : 1;
  unsigned int intercept_rd_ldtr : 1; /* 8 */
  unsigned int intercept_rd_tr : 1;
  unsigned int intercept_wr_idtr : 1;
  unsigned int intercept_wr_gdtr : 1;
  unsigned int intercept_wr_ldtr : 1; /* 12 */
  unsigned int intercept_wr_tr : 1;
  unsigned int intercept_rdtsc : 1;
  unsigned int intercept_rdpmc : 1;
  unsigned int intercept_pushf : 1; /* 16 */
  unsigned int intercept_popf : 1;
  unsigned int intercept_cpuid : 1;
  unsigned int intercept_rsm : 1;
  unsigned int intercept_iret : 1; /* 20 */
  unsigned int intercept_int_n : 1;
  unsigned int intercept_invd : 1;
  unsigned int intercept_pause : 1;
  unsigned int intercept_hlt : 1; /* 24 */
  unsigned int intercept_invlpg : 1;
  unsigned int intercept_invlpga : 1;
  unsigned int intercept_ioio_prot : 1;
  unsigned int intercept_msr_prot : 1; /* 28 */
  unsigned int intercept_task_switches : 1;
  unsigned int intercept_ferr_freeze : 1;
  unsigned int intercept_shutdown : 1;
  /* 0x010 */
  unsigned int intercept_vmrun : 1; /* 0 */
  unsigned int intercept_vmmcall : 1;
  unsigned int intercept_vmload : 1;
  unsigned int intercept_vmsave : 1;
  unsigned int intercept_stgi : 1; /* 4 */
  unsigned int intercept_clgi : 1;
  unsigned int intercept_skinit : 1;
  unsigned int intercept_rdtscp : 1;
  unsigned int intercept_icebp : 1; /* 8 */
  unsigned int intercept_wbinvd : 1;
  unsigned int intercept_monitor : 1;
  unsigned int intercept_mwait_uncond : 1;
  unsigned int intercept_mwait : 1; /* 12 */
  unsigned int reserved010 : 19;    /* 31-13 */
  /* 0x014 */
  uint32_t reserved01[3]; /* 0x014 */
  uint32_t reserved02[4]; /* 0x020 */
  uint32_t reserved03[4]; /* 0x030 */
  uint64_t iopm_base_pa;  /* 0x040 */
  uint64_t msrpm_base_pa; /* 0x048 */
  uint64_t tsc_offset;    /* 0x050 */
  /* 0x058 */
  unsigned int guest_asid : 32;  /* 31-0 */
  unsigned int tlb_control : 8;  /* 39-32 */
  unsigned int reserved058 : 24; /* 63-40 */
  /* 0x060 */
  unsigned int v_tpr : 8;           /* 7-0 */
  unsigned int v_irq : 1;           /* 8 */
  unsigned int reserved060_9 : 7;   /* 15-9 */
  unsigned int v_intr_prio : 4;     /* 19-16 */
  unsigned int v_ign_tpr : 1;       /* 20 */
  unsigned int reserved060_21 : 3;  /* 23-21 */
  unsigned int v_intr_masking : 1;  /* 24 */
  unsigned int reserved060_25 : 7;  /* 31-25 */
  unsigned int v_intr_vector : 8;   /* 39-32 */
  unsigned int reserved060_40 : 24; /* 63-40 */
  /* 0x068 */
  unsigned int interrupt_shadow : 1; /* 0 */
  uint64_t reserved068 : 63;         /* 63-1 */
  /* 0x070 */
  uint64_t exitcode;    /* 0x070 */
  uint64_t exitinfo1;   /* 0x078 */
  uint64_t exitinfo2;   /* 0x080 */
  uint64_t exitintinfo; /* 0x088 */
  /* 0x090 */
  unsigned int np_enable : 1; /* 0 */
  uint64_t reserved090 : 63;  /* 63-1 */
  /* 0x098 */
  uint32_t reserved09[2]; /* 0x098 */
  uint32_t reserved0a[2]; /* 0x0A0 */
  uint64_t eventinj;      /* 0x0A8 */
  uint64_t n_cr3;         /* 0x0B0 */
  /* 0x0B8 */
  unsigned int lbr_virtualization_enable : 1; /* 0 */
  uint64_t reserved0b8 : 63;                  /* 63-1 */
  /* 0x0C0 */
  uint32_t reserved0c[2];              /* 0x0C0 */
  uint64_t nrip;                       /* 0x0C8 */
  uint8_t guest_instruction_bytes[16]; /* 0x0D0 */
  uint32_t reserved0e[4];              /* 0x0E0 */
  uint32_t reserved0f[4];              /* 0x0F0 */
  uint32_t reserved1[64];              /* 0x100 */
  uint32_t reserved2[64];              /* 0x200 */
  uint32_t reserved3[64];              /* 0x300 */
  /* VMCB State Save Area */
  VmcbSeg es;               /* 0x400 */
  VmcbSeg cs;               /* 0x410 */
  VmcbSeg ss;               /* 0x420 */
  VmcbSeg ds;               /* 0x430 */
  VmcbSeg fs;               /* 0x440 */
  VmcbSeg gs;               /* 0x450 */
  VmcbSeg gdtr;             /* 0x460 */
  VmcbSeg ldtr;             /* 0x470 */
  VmcbSeg idtr;             /* 0x480 */
  VmcbSeg tr;               /* 0x490 */
  uint32_t reserved4a[4];   /* 0x4A0 */
  uint32_t reserved4b[4];   /* 0x4B0 */
  uint32_t reserved4c_0[2]; /* 0x4C0 */
  uint8_t reserved4c_8[3];  /* 0x4C8 */
  uint8_t cpl;              /* 0x4CB */
  uint32_t reserved4c_c;    /* 0x4CC */
  uint64_t efer;            /* 0x4D0 */
  uint32_t reserved4d[2];   /* 0x4D8 */
  uint32_t reserved4e[4];   /* 0x4E0 */
  uint32_t reserved4f[4];   /* 0x4F0 */
  uint32_t reserved50[4];   /* 0x500 */
  uint32_t reserved51[4];   /* 0x510 */
  uint32_t reserved52[4];   /* 0x520 */
  uint32_t reserved53[4];   /* 0x530 */
  uint32_t reserved54[2];   /* 0x540 */
  uint64_t cr4;             /* 0x548 */
  uint64_t cr3;             /* 0x550 */
  uint64_t cr0;             /* 0x558 */
  uint64_t dr7;             /* 0x560 */
  uint64_t dr6;             /* 0x568 */
  uint64_t rflags;          /* 0x570 */
  uint64_t rip;             /* 0x578 */
  uint32_t reserved58[4];   /* 0x580 */
  uint32_t reserved59[4];   /* 0x590 */
  uint32_t reserved5a[4];   /* 0x5A0 */
  uint32_t reserved5b[4];   /* 0x5B0 */
  uint32_t reserved5c[4];   /* 0x5C0 */
  uint32_t reserved5d[2];   /* 0x5D0 */
  uint64_t rsp;             /* 0x5D8 */
  uint32_t reserved5e[4];   /* 0x5E0 */
  uint32_t reserved5f[2];   /* 0x5F0 */
  uint64_t rax;             /* 0x5F8 */
  uint64_t star;            /* 0x600 */
  uint64_t lstar;           /* 0x608 */
  uint64_t cstar;           /* 0x610 */
  uint64_t sfmask;          /* 0x618 */
  uint64_t kernel_gs_base;  /* 0x620 */
  uint64_t sysenter_cs;     /* 0x628 */
  uint64_t sysenter_esp;    /* 0x630 */
  uint64_t sysenter_eip;    /* 0x638 */
  uint64_t cr2;             /* 0x640 */
  uint32_t reserved64[2];   /* 0x648 */
  uint32_t reserved65[4];   /* 0x650 */
  uint32_t reserved66[2];   /* 0x660 */
  uint64_t g_pat;           /* 0x668 */
  uint64_t dbgctl;          /* 0x670 */
  uint64_t br_from;         /* 0x678 */
  uint64_t br_to;           /* 0x680 */
  uint64_t lastexcpfrom;    /* 0x688 */
  uint64_t lastexcpto;      /* 0x690 */
  uint32_t reserved69[2];   /* 0x698 */
  uint32_t reserved6a[4];   /* 0x6A0 */
  uint32_t reserved6b[4];   /* 0x6B0 */
  uint32_t reserved6c[4];   /* 0x6C0 */
  uint32_t reserved6d[4];   /* 0x6D0 */
  uint32_t reserved6e[4];   /* 0x6E0 */
  uint32_t reserved6f[4];   /* 0x6F0 */
  uint32_t reserved7[64];   /* 0x700 */
  uint32_t reserved8[64];   /* 0x800 */
  uint32_t reserved9[64];   /* 0x900 */
  uint32_t reserveda[64];   /* 0xA00 */
  uint32_t reservedb[64];   /* 0xB00 */
  uint32_t reservedc[64];   /* 0xC00 */
  uint32_t reservedd[64];   /* 0xD00 */
  uint32_t reservede[64];   /* 0xE00 */
  uint32_t reservedf[64];   /* 0xF00 */
} __attribute__((packed)) Vmcb;

/** SVM Intercept Exit Codes */
typedef enum {
  SVM_EXIT_CODE_CR0_READ = 0x0,
  SVM_EXIT_CODE_CR1_READ = 0x1,
  SVM_EXIT_CODE_CR2_READ = 0x2,
  SVM_EXIT_CODE_CR3_READ = 0x3,
  SVM_EXIT_CODE_CR4_READ = 0x4,
  SVM_EXIT_CODE_CR5_READ = 0x5,
  SVM_EXIT_CODE_CR6_READ = 0x6,
  SVM_EXIT_CODE_CR7_READ = 0x7,
  SVM_EXIT_CODE_CR8_READ = 0x8,
  SVM_EXIT_CODE_CR9_READ = 0x9,
  SVM_EXIT_CODE_CR10_READ = 0xA,
  SVM_EXIT_CODE_CR11_READ = 0xB,
  SVM_EXIT_CODE_CR12_READ = 0xC,
  SVM_EXIT_CODE_CR13_READ = 0xD,
  SVM_EXIT_CODE_CR14_READ = 0xE,
  SVM_EXIT_CODE_CR15_READ = 0xF,
  SVM_EXIT_CODE_CR0_WRITE = 0x10,
  SVM_EXIT_CODE_CR1_WRITE = 0x11,
  SVM_EXIT_CODE_CR2_WRITE = 0x12,
  SVM_EXIT_CODE_CR3_WRITE = 0x13,
  SVM_EXIT_CODE_CR4_WRITE = 0x14,
  SVM_EXIT_CODE_CR5_WRITE = 0x15,
  SVM_EXIT_CODE_CR6_WRITE = 0x16,
  SVM_EXIT_CODE_CR7_WRITE = 0x17,
  SVM_EXIT_CODE_CR8_WRITE = 0x18,
  SVM_EXIT_CODE_CR9_WRITE = 0x19,
  SVM_EXIT_CODE_CR10_WRITE = 0x1A,
  SVM_EXIT_CODE_CR11_WRITE = 0x1B,
  SVM_EXIT_CODE_CR12_WRITE = 0x1C,
  SVM_EXIT_CODE_CR13_WRITE = 0x1D,
  SVM_EXIT_CODE_CR14_WRITE = 0x1E,
  SVM_EXIT_CODE_CR15_WRITE = 0x1F,
  SVM_EXIT_CODE_DR0_READ = 0x20,
  SVM_EXIT_CODE_DR1_READ = 0x21,
  SVM_EXIT_CODE_DR2_READ = 0x22,
  SVM_EXIT_CODE_DR3_READ = 0x23,
  SVM_EXIT_CODE_DR4_READ = 0x24,
  SVM_EXIT_CODE_DR5_READ = 0x25,
  SVM_EXIT_CODE_DR6_READ = 0x26,
  SVM_EXIT_CODE_DR7_READ = 0x27,
  SVM_EXIT_CODE_DR8_READ = 0x28,
  SVM_EXIT_CODE_DR9_READ = 0x29,
  SVM_EXIT_CODE_DR10_READ = 0x2A,
  SVM_EXIT_CODE_DR11_READ = 0x2B,
  SVM_EXIT_CODE_DR12_READ = 0x2C,
  SVM_EXIT_CODE_DR13_READ = 0x2D,
  SVM_EXIT_CODE_DR14_READ = 0x2E,
  SVM_EXIT_CODE_DR15_READ = 0x2F,
  SVM_EXIT_CODE_DR0_WRITE = 0x30,
  SVM_EXIT_CODE_DR1_WRITE = 0x31,
  SVM_EXIT_CODE_DR2_WRITE = 0x32,
  SVM_EXIT_CODE_DR3_WRITE = 0x33,
  SVM_EXIT_CODE_DR4_WRITE = 0x34,
  SVM_EXIT_CODE_DR5_WRITE = 0x35,
  SVM_EXIT_CODE_DR6_WRITE = 0x36,
  SVM_EXIT_CODE_DR7_WRITE = 0x37,
  SVM_EXIT_CODE_DR8_WRITE = 0x38,
  SVM_EXIT_CODE_DR9_WRITE = 0x39,
  SVM_EXIT_CODE_DR10_WRITE = 0x3A,
  SVM_EXIT_CODE_DR11_WRITE = 0x3B,
  SVM_EXIT_CODE_DR12_WRITE = 0x3C,
  SVM_EXIT_CODE_DR13_WRITE = 0x3D,
  SVM_EXIT_CODE_DR14_WRITE = 0x3E,
  SVM_EXIT_CODE_DR15_WRITE = 0x3F,
  SVM_EXIT_CODE_EXCP0 = 0x40,
  SVM_EXIT_CODE_EXCP1 = 0x41,
  SVM_EXIT_CODE_EXCP2 = 0x42,
  SVM_EXIT_CODE_EXCP3 = 0x43,
  SVM_EXIT_CODE_EXCP4 = 0x44,
  SVM_EXIT_CODE_EXCP5 = 0x45,
  SVM_EXIT_CODE_EXCP6 = 0x46,
  SVM_EXIT_CODE_EXCP7 = 0x47,
  SVM_EXIT_CODE_EXCP8 = 0x48,
  SVM_EXIT_CODE_EXCP9 = 0x49,
  SVM_EXIT_CODE_EXCP10 = 0x4A,
  SVM_EXIT_CODE_EXCP11 = 0x4B,
  SVM_EXIT_CODE_EXCP12 = 0x4C,
  SVM_EXIT_CODE_EXCP13 = 0x4D,
  SVM_EXIT_CODE_EXCP14 = 0x4E,
  SVM_EXIT_CODE_EXCP15 = 0x4F,
  SVM_EXIT_CODE_EXCP16 = 0x50,
  SVM_EXIT_CODE_EXCP17 = 0x51,
  SVM_EXIT_CODE_EXCP18 = 0x52,
  SVM_EXIT_CODE_EXCP19 = 0x53,
  SVM_EXIT_CODE_EXCP20 = 0x54,
  SVM_EXIT_CODE_EXCP21 = 0x55,
  SVM_EXIT_CODE_EXCP22 = 0x56,
  SVM_EXIT_CODE_EXCP23 = 0x57,
  SVM_EXIT_CODE_EXCP24 = 0x58,
  SVM_EXIT_CODE_EXCP25 = 0x59,
  SVM_EXIT_CODE_EXCP26 = 0x5A,
  SVM_EXIT_CODE_EXCP27 = 0x5B,
  SVM_EXIT_CODE_EXCP28 = 0x5C,
  SVM_EXIT_CODE_EXCP29 = 0x5D,
  SVM_EXIT_CODE_EXCP30 = 0x5E,
  SVM_EXIT_CODE_EXCP31 = 0x5F,
  SVM_EXIT_CODE_INTR = 0x60,
  SVM_EXIT_CODE_NMI = 0x61,
  SVM_EXIT_CODE_SMI = 0x62,
  SVM_EXIT_CODE_INIT = 0x63,
  SVM_EXIT_CODE_VINTR = 0x64,
  SVM_EXIT_CODE_CR0_SEL_WRITE = 0x65,
  SVM_EXIT_CODE_IDTR_READ = 0x66,
  SVM_EXIT_CODE_GDTR_READ = 0x67,
  SVM_EXIT_CODE_LDTR_READ = 0x68,
  SVM_EXIT_CODE_TR_READ = 0x69,
  SVM_EXIT_CODE_IDTR_WRITE = 0x6A,
  SVM_EXIT_CODE_GDTR_WRITE = 0x6B,
  SVM_EXIT_CODE_LDTR_WRITE = 0x6C,
  SVM_EXIT_CODE_TR_WRITE = 0x6D,
  SVM_EXIT_CODE_RDTSC = 0x6E,
  SVM_EXIT_CODE_RDPMC = 0x6F,
  SVM_EXIT_CODE_PUSHF = 0x70,
  SVM_EXIT_CODE_POPF = 0x71,
  SVM_EXIT_CODE_CPUID = 0x72,
  SVM_EXIT_CODE_RSM = 0x73,
  SVM_EXIT_CODE_IRET = 0x74,
  SVM_EXIT_CODE_SWINT = 0x75,
  SVM_EXIT_CODE_INVD = 0x76,
  SVM_EXIT_CODE_PAUSE = 0x77,
  SVM_EXIT_CODE_HLT = 0x78,
  SVM_EXIT_CODE_INVLPG = 0x79,
  SVM_EXIT_CODE_INVLPGA = 0x7A,
  SVM_EXIT_CODE_IOIO = 0x7B,
  SVM_EXIT_CODE_MSR = 0x7C,
  SVM_EXIT_CODE_TASK_SWITCH = 0x7D,
  SVM_EXIT_CODE_FERR_FREEZE = 0x7E,
  SVM_EXIT_CODE_SHUTDOWN = 0x7F,
  SVM_EXIT_CODE_VMRUN = 0x80,
  SVM_EXIT_CODE_VMMCALL = 0x81,
  SVM_EXIT_CODE_VMLOAD = 0x82,
  SVM_EXIT_CODE_VMSAVE = 0x83,
  SVM_EXIT_CODE_STGI = 0x84,
  SVM_EXIT_CODE_CLGI = 0x85,
  SVM_EXIT_CODE_SKINIT = 0x86,
  SVM_EXIT_CODE_RDTSCP = 0x87,
  SVM_EXIT_CODE_ICEBP = 0x88,
  SVM_EXIT_CODE_WBINVD = 0x89,
  SVM_EXIT_CODE_MONITOR = 0x8A,
  SVM_EXIT_CODE_MWAIT = 0x8B,
  SVM_EXIT_CODE_MWAIT_CONDITIONAL = 0x8C,
  SVM_EXIT_CODE_NPF = 0x400,
  SVM_EXIT_CODE_INVALID = -1,
} SvmExitCode;
