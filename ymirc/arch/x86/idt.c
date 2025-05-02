#include "idt.h"

#include <stdint.h>

#include "bits.h"
#include "gdt.h"
#include "isr.h"

/** IDT Register. */
typedef struct {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed)) Idtr;

/** Entry in the Interrupt Descriptor Table. */
typedef struct {
  unsigned __int128 value;
} GateDescriptor;

// Lower 16 bits of the offset to the ISR.
#define GATE_OFFSET_LOW_MASK 0xFFFFULL << 0
#define GATE_OFFSET_LOW_WIDTH 16
// Segment Selector that must point to a valid code segment in the GDT.
#define GATE_SEG_SELECTOR_MASK 0xFFFFULL << 16
// Interrupt Stack Table. Not used.
#define GATE_IST_MASK 0b111ULL << 32
// Reserved.
#define GATE_RESERVED1_MASK 0b11111ULL << 35
// Gate Type.
#define GATE_TYPE_MASK 0xFULL << 40
// Reserved.
#define GATE_RESERVED2_MASK 0x1ULL << 44
// Descriptor Privilege Level is the required CPL to call the ISR via the INT
// inst. Hardware interrupts ignore this field.
#define GATE_DPL_MASK 0x11ULL << 45
// Present flag. Must be 1.
#define GATE_PRESENT_MASK 0x1ULL << 47
// Middle 16 bits of the offset to the ISR.
#define GATE_OFFSET_MIDDLE_MASK 0xFFFFULL << 48
#define GATE_OFFSET_MIDDLE_WIDTH 16
// Higher 32 bits of the offset to the ISR.
#define GATE_OFFSET_HIGH_MASK (unsigned __int128)0xFFFFFFFF << 64
#define GATE_OFFSET_HIGH_WIDTH 32
// Reserved.
#define GATE_RESERVED3_MASK (unsigned __int128)0xFFFFFFFF << 96

/** Interrupt Descriptor Table. */
__attribute__((aligned(4096))) static GateDescriptor idt[MAX_NUM_GATES] = {0};

static Idtr idtr = {.limit = sizeof(idt) - 1, .base = (uintptr_t)&idt};

/** Load Interrupt Descriptor Table Register */
static inline void lidt(Idtr *idtr) {
  __asm__ volatile("lidt (%0)" : : "r"(idtr));
}

/** Set a gate descriptor in the IDT. */
void set_gate(unsigned int index, GateType gate_type, Isr offset) {
  GateDescriptor *desc = &idt[index];
  set_masked_bits_128(
      &desc->value,
      get_lower_bits_128((uintptr_t)offset, GATE_OFFSET_LOW_WIDTH),
      GATE_OFFSET_LOW_MASK);
  set_masked_bits_128(&desc->value, SEGMENT_SELECTOR(KERNEL_CS_INDEX),
                      GATE_SEG_SELECTOR_MASK);
  set_masked_bits_128(&desc->value, gate_type, GATE_TYPE_MASK);
  set_masked_bits_128(
      &desc->value,
      get_lower_bits_128((uintptr_t)offset >> GATE_OFFSET_LOW_WIDTH,
                         GATE_OFFSET_MIDDLE_WIDTH),
      GATE_OFFSET_MIDDLE_MASK);
  set_masked_bits_128(
      &desc->value,
      get_lower_bits_128((uintptr_t)offset >>
                             (GATE_OFFSET_LOW_WIDTH + GATE_OFFSET_MIDDLE_WIDTH),
                         GATE_OFFSET_HIGH_WIDTH),
      GATE_OFFSET_HIGH_MASK);
  set_masked_bits_128(&desc->value, 1, GATE_PRESENT_MASK);
}

void idt_init() { lidt(&idtr); }
