#include "svm_ioio.h"

#include <stdint.h>
#include <sys/io.h>

#include "log.h"
#include "serial.h"

/** EXITINFO1 for IOIO Intercept */
typedef union {
  struct {
    unsigned int type : 1;
    unsigned int _reserved1 : 1;
    unsigned int str : 1;
    unsigned int rep : 1;
    unsigned int sz8 : 1;
    unsigned int sz16 : 1;
    unsigned int sz32 : 1;
    unsigned int a16 : 1;
    unsigned int a32 : 1;
    unsigned int a64 : 1;
    unsigned int seg : 3;
    unsigned int _reserved2 : 3;
    unsigned int port : 16;
  };
  uint32_t value;
} __attribute__((packed)) IOIOInterceptInfo;

static void handle_serial_in(SvmVcpu *vcpu, IOIOInterceptInfo info);
static void handle_serial_out(SvmVcpu *vcpu, IOIOInterceptInfo info);
static void handle_pit_in(SvmVcpu *vcpu, IOIOInterceptInfo info);
static void handle_pit_out(SvmVcpu *vcpu, IOIOInterceptInfo info);

static void handle_ioio_in(SvmVcpu *vcpu, IOIOInterceptInfo info) {
  Vmcb *vmcb = vcpu->vmcb;

  switch (info.port) {
    case 0x0040 ... 0x0047:
      handle_pit_in(vcpu, info);
      break;
    case 0x02E8 ... 0x02EF:  // Fourth serial port. Ignore.
    case 0x02F8 ... 0x02FF:  // Second serial port. Ignore.
    case 0x03E8 ... 0x03EF:  // Third serial port. Ignore.
      break;
    case 0x03F8 ... 0x03FF:
      handle_serial_in(vcpu, info);
      break;
    case 0xC000 ... 0xCFFF:  // Old PCI. Ignore.
      break;
    case 0x0CF8 ... 0x0CFF:  // PCI. Unimplemented.
      vmcb->rax = 0;
      break;
    default:
      LOG_ERROR("Unhandled I/O-in port: 0x%x\n", info.port);
      svm_vcpu_abort(vcpu);
  }
}

static void handle_ioio_out(SvmVcpu *vcpu, IOIOInterceptInfo info) {
  switch (info.port) {
    case 0x0040 ... 0x0047:
      handle_pit_out(vcpu, info);
      break;
    case 0x02E8 ... 0x02EF:  // Fourth serial port. Ignore.
    case 0x02F8 ... 0x02FF:  // Second serial port. Ignore.
    case 0x03E8 ... 0x03EF:  // Third serial port. Ignore.
      break;
    case 0x03F8 ... 0x03FF:
      handle_serial_out(vcpu, info);
      break;
    case 0xC000 ... 0xCFFF:  // Old PCI. Ignore.
      break;
    case 0x0CF8 ... 0x0CFF:  // PCI. Unimplemented.
      break;
    default:
      LOG_ERROR("Unhandled I/O-out port: 0x%x\n", info.port);
      svm_vcpu_abort(vcpu);
  }
}

void handle_svm_ioio_exit(SvmVcpu *vcpu) {
  uint32_t exitinfo1 = (uint32_t)(vcpu->vmcb->exitinfo1 & 0xFFFFFFFF);
  IOIOInterceptInfo info = {.value = exitinfo1};

  // 0 = OUT instruction, 1 = IN instruction
  switch (info.type) {
    case 0:
      handle_ioio_out(vcpu, info);
      break;
    case 1:
      handle_ioio_in(vcpu, info);
      break;
  }
}

// =============================================================================

static void handle_serial_in(SvmVcpu *vcpu, IOIOInterceptInfo info) {
  Vmcb *vmcb = vcpu->vmcb;

  switch (info.port) {
    // Receive buffer.
    case 0x3F8:
      vmcb->rax = inb(info.port);  // pass-through
      break;
    // Interrupt Enable Register (DLAB=1) / Divisor Latch High Register
    // (DLAB=0)
    case 0x3F9:
      vmcb->rax = vcpu->g_serial.ier;
      break;
    // Interrupt Identification Register.
    case 0x3FA:
      vmcb->rax = inb(info.port);  // pass-through
      break;
    // Line Control Register (MSB is DLAB).
    case 0x3FB:
      vmcb->rax = 0x00;
      break;
    // Modem Control Register.
    case 0x3FC:
      vmcb->rax = vcpu->g_serial.mcr;
      break;
    // Line Status Register.
    case 0x3FD:
      vmcb->rax = inb(info.port);  // pass-through
      break;
    // Modem Status Register.
    case 0x3FE:
      vmcb->rax = inb(info.port);  // pass-through
      break;
    // Scratch Register.
    case 0x3FF:
      vmcb->rax = 0x00;  // 8250
      break;
    default:
      LOG_ERROR("Unsupported I/O-in to the first serial port: 0x%x\n",
                info.port);
      svm_vcpu_abort(vcpu);
  }
}

static void handle_serial_out(SvmVcpu *vcpu, IOIOInterceptInfo info) {
  Vmcb *vmcb = vcpu->vmcb;

  switch (info.port) {
    // Transmit buffer.
    case 0x3F8:
      serial_write(vcpu->serial, (uint8_t)(vmcb->rax & 0xFF));
      break;
    // Interrupt Enable Register.
    case 0x3F9:
      vcpu->g_serial.ier = (uint8_t)(vmcb->rax & 0xFF);
      break;
    // FIFO control registers.
    case 0x3FA:
      // ignore
      break;
    // Line Control Register (MSB is DLAB).
    case 0x3FB:
      // ignore
      break;
    // Modem Control Register.
    case 0x3FC:
      vcpu->g_serial.mcr = (uint8_t)(vmcb->rax & 0xFF);
      break;
    // Scratch Register.
    case 0x3FF:
      // ignore
      break;
    default:
      LOG_ERROR("Unsupported I/O-out to the first serial port: 0x%x\n",
                info.port);
      svm_vcpu_abort(vcpu);
  }
}

static void handle_pit_in(SvmVcpu *vcpu, IOIOInterceptInfo info) {
  Vmcb *vmcb = vcpu->vmcb;

  // Pass-through.
  if (info.sz8) {
    vmcb->rax = inb(info.port);
  } else if (info.sz16) {
    vmcb->rax = inw(info.port);
  } else if (info.sz32) {
    vmcb->rax = inl(info.port);
  }
}

static void handle_pit_out(SvmVcpu *vcpu, IOIOInterceptInfo info) {
  Vmcb *vmcb = vcpu->vmcb;

  // Pass-through.
  if (info.sz8) {
    outb((uint8_t)(vmcb->rax & 0xFF), info.port);
  } else if (info.sz16) {
    outw((uint16_t)(vmcb->rax & 0xFFFF), info.port);
  } else if (info.sz32) {
    outl((uint32_t)(vmcb->rax & 0xFFFFFFFF), info.port);
  }
}
