#include "serial_impl.h"

#include <stdint.h>

#include "serial.h"

/// Transmitter Holding Buffer: DLAB=0, W
static const int txr __attribute__((unused)) = 0;
/// Receiver Buffer: DLAB=0, R
static const int rxr __attribute__((unused)) = 0;
/// Divisor Latch Low Byte: DLAB=1, R/W
static const int dll __attribute__((unused)) = 0;
/// Interrupt Enable Register: DLAB=0, R/W
static const int ier __attribute__((unused)) = 1;
/// Divisor Latch High Byte: DLAB=1, R/W
static const int dlh __attribute__((unused)) = 1;
/// Interrupt Identification Register: DLAB=X, R
static const int iir __attribute__((unused)) = 2;
/// FIFO Control Register: DLAB=X, W
static const int fcr __attribute__((unused)) = 2;
/// Line Control Register: DLAB=X, R/W
static const int lcr __attribute__((unused)) = 3;
/// Modem Control Register: DLAB=X, R/W
static const int mcr __attribute__((unused)) = 4;
/// Line Status Register: DLAB=X, R
static const int lsr __attribute__((unused)) = 5;
/// Modem Status Register: DLAB=X, R
static const int msr __attribute__((unused)) = 6;
/// Scratch Register: DLAB=X, R/W
static const int sr __attribute__((unused)) = 7;

static const int divisor_latch_numerator = 115200;

static uint8_t inb(int port) {
  uint8_t ret;
  __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

static void outb(uint8_t value, int port) {
  __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

static void write_byte(uint8_t byte, Ports port) {
  // Wait for the transmitter holding register to be empty
  while ((inb(port + lsr) & 0b00100000) == 0) {
    __asm__ __volatile__("rep; nop");
  }
  outb(byte, port);
}

void write_byte_com1(uint8_t byte) { write_byte(byte, com1); }
void write_byte_com2(uint8_t byte) { write_byte(byte, com2); }
void write_byte_com3(uint8_t byte) { write_byte(byte, com3); }

/** Initialize a serial console. */
void serial_impl_init(Serial *serial, Ports port, uint32_t baud) {
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

  // Set write-function
  switch (port) {
    case com1:
      serial->write_fn = write_byte_com1;
      break;
    case com2:
      serial->write_fn = write_byte_com2;
      break;
    case com3:
      serial->write_fn = write_byte_com3;
      break;
  }
}
