#pragma once

#include <assert.h>
#include <stdint.h>
#include <string.h>

/** Where the kernel boot parameters are loaded, known as "zero page". Must be
 * initialized with zeros. */
#define LINUX_LAYOUT_BOOTPARAM 0x00010000
/** Where the kernel cmdline is located. */
#define LINUX_LAYOUT_CMDLINE 0x00020000
/** Where the protected-mode kernel code is loaded. */
#define LINUX_LAYOUT_KERNEL_BASE 0x00100000
/** Where the initrd is loaded. */
#define LINUX_LAYOUT_INITRD 0x06000000

/** The offset where the header starts in the bzImage. */
#define SETUP_HEADER_OFFSET 0x1F1

/** Maximum number of entries in the E820 map. */
#define E820MAX 128

/** Bitfield for loadflags. */
typedef struct {
  /** If true, the protected-mode code is loaded at 0x100000. */
  uint8_t loaded_high : 1;
  /** If true, KASLR enabled. */
  uint8_t kaslr_flag : 1;
  /** Unused. */
  uint8_t _unused : 3;
  /** If false, print early messages. */
  uint8_t quiet_flag : 1;
  /** If false, reload the segment registers in the 32 bit entry point. */
  uint8_t keep_segments : 1;
  /** Set true to indicate that the value entered in the `heap_end_ptr` is
   * valid. */
  uint8_t can_use_heap : 1;
} __attribute__((packed)) LoadflagBitfield;

/** Representation of the linux kernel header. This header compiles with
 * protocol v2.15. */
typedef struct {
  /** RO. The number of setup sectors. */
  uint8_t setup_sects;
  uint16_t root_flags;
  uint32_t syssize;
  uint16_t ram_size;
  uint16_t vid_mode;
  uint16_t root_dev;
  uint16_t boot_flag;
  uint16_t jump;
  uint32_t header;
  /** RO. Boot protocol version supported. */
  uint16_t version;
  uint32_t realmode_swtch;
  uint16_t start_sys_seg;
  uint16_t kernel_version;
  /** M. The type of loader. Specify 0xFF if no ID is assigned. */
  uint8_t type_of_loader;
  /** M. Bitmask. */
  LoadflagBitfield loadflags;
  uint16_t setup_move_size;
  uint32_t code32_start;
  /** M. The 32-bit linear address of initial ramdisk or ramfs. Specify 0 if
   * there is no ramdisk or ramfs. */
  uint32_t ramdisk_image;
  /** M. The size of the initial ramdisk or ramfs. */
  uint32_t ramdisk_size;
  uint32_t bootsect_kludge;
  /** W. Offset of the end of the setup/heap minus 0x200. */
  uint16_t heap_end_ptr;
  /** W(opt). Extension of the loader ID. */
  uint8_t ext_loader_ver;
  uint8_t ext_loader_type;
  /** W. The 32-bit linear address of the kernel command line. */
  uint32_t cmd_line_ptr;
  /** R. Highest address that can be used for initrd. */
  uint32_t initrd_addr_max;
  uint32_t kernel_alignment;
  uint8_t relocatable_kernel;
  uint8_t min_alignment;
  uint16_t xloadflags;
  /** R. Maximum size of the cmdline. */
  uint32_t cmdline_size;
  uint32_t hardware_subarch;
  uint64_t hardware_subarch_data;
  uint32_t payload_offset;
  uint32_t payload_length;
  uint64_t setup_data;
  uint64_t pref_address;
  uint32_t init_size;
  uint32_t handover_offset;
  uint32_t kernel_info_offset;
} __attribute__((packed)) SetupHeader;

static_assert(sizeof(SetupHeader) == 0x7B, "Unexpected SetupHeader size");

/** Instantiate a header from bzImage. */
static inline void setupheader_from_bzimage(SetupHeader *hdr,
                                            const void *bytes) {
  memcpy(hdr, (uint8_t *)bytes + SETUP_HEADER_OFFSET, sizeof(SetupHeader));
  if (hdr->setup_sects == 0) {
    hdr->setup_sects = 4;
  }
}

/** Get the offset of the protected-mode kernel code. Real-mode code consists of
 * the boot sector (1 sector == 512 bytes) plus the setup code (`setup_sects`
 * sectors). */
static inline size_t protected_code_offset(const SetupHeader *hdr) {
  return ((size_t)hdr->setup_sects + 1) * 512;
}

typedef enum {
  E820_TYPE_RAM = 1,       // RAM
  E820_TYPE_RESERVED = 2,  // Reserved
  E820_TYPE_ACPI = 3,      // ACPI reclaimable memory
  E820_TYPE_NVS = 4,       // ACPI NVS memory
  E820_TYPE_UNUSABLE = 5   // Unusable memory region
} E820Type;

typedef struct {
  uint64_t addr;
  uint64_t size;
  E820Type type;
} __attribute__((packed)) E820Entry;

static_assert(sizeof(E820Entry) == 0x14, "E820Entry must be 0x14 bytes");

/** Port of struct boot_params in linux kernel.
 * Note that fields prefixed with `_` are not implemented and have incorrect
 * types. */
typedef struct {
  uint8_t _screen_info[0x40];
  uint8_t _apm_bios_info[0x14];
  uint8_t _pad2[4];
  uint64_t tboot_addr;
  uint8_t ist_info[0x10];
  uint8_t _pad3[0x10];
  uint8_t hd0_info[0x10];
  uint8_t hd1_info[0x10];
  uint8_t _sys_desc_table[0x10];
  uint8_t _olpc_ofw_header[0x10];
  uint8_t _pad4[0x80];
  uint8_t _edid_info[0x80];
  uint8_t _efi_info[0x20];
  uint32_t alt_mem_k;
  uint32_t scratch;
  /** Number of entries in the E820 map. */
  uint8_t e820_entries;
  uint8_t eddbuf_entries;
  uint8_t edd_mbr_sig_buf_entries;
  uint8_t kbd_status;
  uint8_t _pad6[5];
  /** Setup header. */
  SetupHeader hdr;
  uint8_t _pad7[0x290 - SETUP_HEADER_OFFSET - sizeof(SetupHeader)];
  uint32_t _edd_mbr_sig_buffer[0x10];
  /** System memory map that can be retrieved by INT 15, E820h. */
  E820Entry e820_map[E820MAX];
  uint8_t _unimplemented[0x330];
} __attribute__((packed)) BootParams;

static_assert(sizeof(BootParams) == 0x1000, "Unexpected BootParams size.");

/** Instantiate boot params from bzImage. */
static inline void bootparams_from_bzimage(BootParams *bp, const void *bytes) {
  memcpy(bp, bytes, sizeof(BootParams));
}

/** Add an entry to the E820 map. */
static inline void bootparams_add_e820_entry(BootParams *bp, uint64_t addr,
                                             uint64_t size, E820Type type) {
  bp->e820_map[bp->e820_entries].addr = addr;
  bp->e820_map[bp->e820_entries].size = size;
  bp->e820_map[bp->e820_entries].type = type;
  bp->e820_entries += 1;
}
