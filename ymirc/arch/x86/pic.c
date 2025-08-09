#include "pic.h"

#include <stdint.h>
#include <sys/io.h>

#include "arch.h"
#include "bits.h"

#define ICW1_ICW4 0x01      /* Indicates that ICW4 will be present */
#define ICW1_SINGLE 0x02    /* Single (cascade) mode */
#define ICW1_INTERVAL4 0x04 /* Call address interval 4 (8) */
#define ICW1_LEVEL 0x08     /* Level triggered (edge) mode */
#define ICW1_INIT 0x10      /* Initialization - required! */

#define ICW4_8086 0x01       /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO 0x02       /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE 0x08  /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C /* Buffered mode/master */
#define ICW4_SFNM 0x10       /* Special fully nested (not) */

#define OCW2_EOI 0x20 /** EOI */
#define OCW2_SL 0x40  /** If set, specific EOI. */

#define OCW3_READ_IRR 0x02
#define OCW3_READ_ISR 0x03
#define OCW3_REQUIRED 0x08

static const uint16_t primary_command_port = 0x20;
static const uint16_t primary_data_port = primary_command_port + 1;
static const uint16_t secondary_command_port = 0xA0;
static const uint16_t secondary_data_port = secondary_command_port + 1;

void pic_init() {
  // We have to disable interrupts to prevent PIC-driven interrupts before
  // registering handlers.
  disable_intr();

  // Start initialization sequence. (ICW1)
  outb(ICW1_INIT | ICW1_ICW4, primary_command_port);
  outb(ICW1_INIT | ICW1_ICW4, secondary_command_port);

  // Set the vector offsets. (ICW2)
  outb(primary_vector_offset, primary_data_port);
  outb(secondary_vector_offset, secondary_data_port);

  // Tell primary PIC that there is a slave PIC at IRQ2. (ICW3)
  outb(0b100, primary_data_port);
  // Tell secondary PIC its cascade identity. (ICW3)
  outb(0b10, secondary_data_port);

  // Set the mode. (ICW4)
  outb(ICW4_8086, primary_data_port);
  outb(ICW4_8086, secondary_data_port);

  // Mask all IRQ lines. (OCW1)
  outb(0xFF, primary_data_port);
  outb(0xFF, secondary_data_port);

  // Enable interrupts.
  enable_intr();
}

/** Get the command port for this IRQ. */
static inline uint16_t command_port(IrqLine irq) {
  return is_primary(irq) ? primary_command_port : secondary_command_port;
}

/** Get the data port for this IRQ. */
static inline uint16_t data_port(IrqLine irq) {
  return is_primary(irq) ? primary_data_port : secondary_data_port;
}

void set_mask(IrqLine irq) {
  // OCW1
  uint16_t port = data_port(irq);
  outb(inb(port) | tobit(delta(irq)), port);
}

void unset_mask(IrqLine irq) {
  // OCW1
  uint16_t port = data_port(irq);
  outb(inb(port) & ~tobit(delta(irq)), port);
}

void notify_eoi(IrqLine irq) {
  // OCW2
  outb(OCW2_EOI | OCW2_SL | delta(irq), command_port(irq));
  // If the IRQ came from the Slave PIC, it is necessary to issue the command to
  // both PIC chips.
  if (!is_primary(irq)) {
    outb(OCW2_EOI | OCW2_SL | irq_secondary, command_port(irq));
  }
}

uint16_t pic_read_irr() {
  // OCW3
  outb(OCW3_READ_IRR | OCW3_REQUIRED, primary_command_port);
  outb(OCW3_READ_IRR | OCW3_REQUIRED, secondary_command_port);

  uint8_t primary_irr = inb(primary_command_port);
  uint8_t secondary_irr = inb(secondary_command_port);

  return ((uint16_t)secondary_irr << 8) | primary_irr;
}
