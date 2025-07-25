#pragma once

#define KERNEL_DS_INDEX 0x01
#define KERNEL_CS_INDEX 0x02

/** RPL and TI is 0. */
#define SEGMENT_SELECTOR(index) ((index) << 3)

void gdt_init();
