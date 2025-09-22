#include "serial.h"

#include <stdint.h>
#include <sys/io.h>

#include "bits.h"
#include "panic.h"

/** Available serial ports. */
#define COM1 0x3F8
#define COM2 0x2F8
#define COM3 0x3E8

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

/** Write a single byte to the serial console. */
static void write_byte(uint8_t byte, uint16_t addr) {
  // Wait for the transmitter holding register to be empty
  while (!isset_8(inb(addr + lsr), 5)) {
    __asm__ __volatile__("rep; nop");
  }
  outb(byte, addr);
}

void write_byte_com1(uint8_t byte) { write_byte(byte, COM1); }
void write_byte_com2(uint8_t byte) { write_byte(byte, COM2); }
void write_byte_com3(uint8_t byte) { write_byte(byte, COM3); }

/** Read a byte from Rx buffer. If Rx buffer is empty, return -1. */
static int try_read_byte(uint16_t addr) {
  // Check if Rx buffer is not empty.
  if (!isset_8(inb(addr + lsr), 0)) {
    return -1;
  }

  // read char from the receiver buffer.
  return inb(addr);
}

int try_read_byte_com1() { return try_read_byte(COM1); }
int try_read_byte_com2() { return try_read_byte(COM2); }
int try_read_byte_com3() { return try_read_byte(COM3); }

void serial_init(Serial *serial, SerialPort port, uint32_t baud) {
  switch (port) {
    case SERIAL_PORT_COM1:
      serial->addr = COM1;
      break;
    case SERIAL_PORT_COM2:
      serial->addr = COM2;
      break;
    case SERIAL_PORT_COM3:
      serial->addr = COM3;
      break;
    default:
      panic("Unsupported serial port.");
  }

  outb(0x3, serial->addr + lcr);  // 8 bits, no parity, 1 stop bit
  outb(0x0, serial->addr + ier);  // Disable interrupts
  outb(0x0, serial->addr + fcr);  // Disable FIFO

  // Set baud rate
  int divisor = divisor_latch_numerator / baud;
  uint8_t c = inb(serial->addr + lcr);
  outb(c | 0b10000000, serial->addr + lcr);         // Enable DLAB
  outb(divisor & 0xFF, serial->addr + dll);         // Set divisor low byte
  outb((divisor >> 8) & 0xFF, serial->addr + dlh);  // Set divisor high byte
  outb(c & 0b01111111, serial->addr + lcr);         // Disable DLAB

  // Set write-function
  switch (serial->addr) {
    case COM1:
      serial->write_fn = write_byte_com1;
      break;
    case COM2:
      serial->write_fn = write_byte_com2;
      break;
    case COM3:
      serial->write_fn = write_byte_com3;
      break;
  }

  // Set read-function
  switch (serial->addr) {
    case COM1:
      serial->read_fn = try_read_byte_com1;
      break;
    case COM2:
      serial->read_fn = try_read_byte_com2;
      break;
    case COM3:
      serial->read_fn = try_read_byte_com3;
      break;
  }
}

void enable_serial_interrupt(const Serial *serial) {
  uint8_t ie = inb(serial->addr + ier);
  ie |= 0b00000011;  // Rx-available, Tx-empty
  outb(ie, serial->addr + ier);
}
