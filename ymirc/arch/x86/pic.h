#pragma once

#include <stdbool.h>
#include <stdint.h>

/** Line numbers for the PIC. */
typedef enum {
  irq_timer = 0,
  irq_keyboard = 1,
  irq_secondary = 2,
  irq_serial2 = 3,
  irq_serial1 = 4,
  irq_parallel23 = 5,
  irq_floppy = 6,
  irq_parallel1 = 7,
  irq_rtc = 8,
  irq_acpi = 9,
  irq_open1 = 10,
  irq_open2 = 11,
  irq_mouse = 12,
  irq_cop = 13,
  irq_primary_ata = 14,
  irq_secondary_ata = 15,
} IrqLine;

/** Interrupt vector for the primary PIC. Must be divisible by 8. */
static const uint8_t primary_vector_offset = 32;
/** Interrupt vector for the secondary PIC. Must be divisible by 8. */
static const uint8_t secondary_vector_offset = primary_vector_offset + 8;

/** Return true if the IRQ belongs to the primary PIC. */
static inline bool is_primary(IrqLine irq) { return irq < 8; }

/** Get the offset of the IRQ within the PIC. */
static inline int delta(IrqLine irq) { return is_primary(irq) ? irq : irq - 8; }

/** Initialize the PIC remapping its interrupt vectors. All interrupts are
 * masked after initialization. You MUST call this function before using the
 * PIC. */
void pic_init();

/** Mask the given IRQ line. */
void set_mask(IrqLine irq);

/** Unset the mask of the given IRQ line. */
void unset_mask(IrqLine irq);

/** Notify the end of interrupt (EOI) to the PIC. This function uses
 * specific-EOI. */
void notify_eoi(IrqLine irq);

/** Read IRR in PIC. */
uint16_t pic_read_irr();
