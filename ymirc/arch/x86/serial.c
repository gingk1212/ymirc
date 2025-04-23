#include "serial.h"

#include <stdint.h>

/// Transmitter Holding Buffer: DLAB=0, W
static const int txr = 0;
/// Receiver Buffer: DLAB=0, R
static const int rxr = 0;
/// Divisor Latch Low Byte: DLAB=1, R/W
static const int dll = 0;
/// Interrupt Enable Register: DLAB=0, R/W
static const int ier = 1;
/// Divisor Latch High Byte: DLAB=1, R/W
static const int dlh = 1;
/// Interrupt Identification Register: DLAB=X, R
static const int iir = 2;
/// FIFO Control Register: DLAB=X, W
static const int fcr = 2;
/// Line Control Register: DLAB=X, R/W
static const int lcr = 3;
/// Modem Control Register: DLAB=X, R/W
static const int mcr = 4;
/// Line Status Register: DLAB=X, R
static const int lsr = 5;
/// Modem Status Register: DLAB=X, R
static const int msr = 6;
/// Scratch Register: DLAB=X, R/W
static const int sr = 7;

static uint8_t inb(int port) {
  uint8_t ret;
  __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

static void outb(uint8_t value, int port) {
  __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

static const int divisor_latch_numerator = 115200;

/** Initialize a serial console. */
void init_serial(Ports port, uint32_t baud) {
  outb(0x3, port + lcr);  // 8 bits, no parity, 1 stop bit
  outb(0x0, port + ier);  // Disable interrupts
  outb(0x0, port + fcr);  // Disable FIFO

  // Set baud rate
  int divisor = divisor_latch_numerator / baud;
  uint8_t c = inb(port + lcr);
  outb(c | 0b10000000, port + lcr);         // Enable DLAB
  outb(divisor & 0xFF, port + dll);         // Set divisor low byte
  outb((divisor >> 8) & 0xFF, port + dlh);  // Set divisor high byte
  outb(c & 0b01111111, port + lcr);         // Disable DLAB
}

void write_byte(uint8_t byte, Ports port) {
  // Wait for the transmitter holding register to be empty
  while ((inb(port + lsr) & 0b00100000) == 0) {
    __asm__ __volatile__("rep; nop");
  }
  outb(byte, port);
}
