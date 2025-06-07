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
