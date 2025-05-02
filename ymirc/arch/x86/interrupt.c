#include "interrupt.h"

#include <stddef.h>

#include "arch.h"
#include "idt.h"
#include "isr.h"
#include "log.h"

/** Interrupt handler function signature. */
typedef void (*Handler)(Context *ctx);

/** Interrupt handlers. */
static Handler handlers[MAX_NUM_GATES] = {0};

/** Set the interrupt flag */
static inline void sti() { __asm__ volatile("sti"); }

static char *exception_name(uint64_t vector);
static void unhandled_handler(Context *ctx);

/** Initialize the IDT. */
void itr_init() {
  for (int i = 0; i < MAX_NUM_GATES; i++) {
    set_gate(i, interrupt64, isr_table[i]);
    handlers[i] = unhandled_handler;
  }

  idt_init();

  sti();
}

/** Called from the ISR stub. Dispatches the interrupt to the appropriate
 * handler. */
void itr_dispatch(Context *ctx) {
  uint64_t vector = ctx->vector;
  handlers[vector](ctx);
}

static void unhandled_handler(Context *ctx) {
  LOG_ERROR("============ Oops! ===================\n");
  LOG_ERROR("Unhandled interrupt: %s (%d)\n", exception_name(ctx->vector),
            ctx->vector);
  LOG_ERROR("Error Code: 0x%x\n", ctx->error_code);
  LOG_ERROR("RIP    : 0x%x\n", ctx->rip);
  LOG_ERROR("EFLAGS : 0x%x\n", ctx->rflags);
  LOG_ERROR("RAX    : 0x%x\n", ctx->registers.rax);
  LOG_ERROR("RBX    : 0x%x\n", ctx->registers.rbx);
  LOG_ERROR("RCX    : 0x%x\n", ctx->registers.rcx);
  LOG_ERROR("RDX    : 0x%x\n", ctx->registers.rdx);
  LOG_ERROR("RSI    : 0x%x\n", ctx->registers.rsi);
  LOG_ERROR("RDI    : 0x%x\n", ctx->registers.rdi);
  LOG_ERROR("RBP    : 0x%x\n", ctx->registers.rbp);
  LOG_ERROR("R8     : 0x%x\n", ctx->registers.r8);
  LOG_ERROR("R9     : 0x%x\n", ctx->registers.r9);
  LOG_ERROR("R10    : 0x%x\n", ctx->registers.r10);
  LOG_ERROR("R11    : 0x%x\n", ctx->registers.r11);
  LOG_ERROR("R12    : 0x%x\n", ctx->registers.r12);
  LOG_ERROR("R13    : 0x%x\n", ctx->registers.r13);
  LOG_ERROR("R14    : 0x%x\n", ctx->registers.r14);
  LOG_ERROR("R15    : 0x%x\n", ctx->registers.r15);
  LOG_ERROR("CS     : 0x%x\n", ctx->cs);

  endless_halt();
}

/** Exception vectors. */
#define DIVIDE_BY_ZERO 0
#define DEBUG 1
#define NON_MASKABLE_INTERRUPT 2
#define BREAKPOINT 3
#define OVERFLOW 4
#define BOUND_RANGE_EXCEEDED 5
#define INVALID_OPCODE 6
#define DEVICE_NOT_AVAILABLE 7
#define DOUBLE_FAULT 8
#define COPROCESSOR_SEGMENT_OVERRUN 9
#define INVALID_TSS 10
#define SEGMENT_NOT_PRESENT 11
#define STACK_SEGMENT_FAULT 12
#define GENERAL_PROTECTION_FAULT 13
#define PAGE_FAULT 14
#define FLOATING_POINT_EXCEPTION 16
#define ALIGNMENT_CHECK 17
#define MACHINE_CHECK 18
#define SIMD_EXCEPTION 19
#define VIRTUALIZATION_EXCEPTION 20
#define CONTROL_PROTECTION_EXCEPTON 21

/** Get the name of an exception. */
static char *exception_name(uint64_t vector) {
  switch (vector) {
    case DIVIDE_BY_ZERO:
      return "#DE: return Divide by zero";
    case DEBUG:
      return "#DB: return Debug";
    case NON_MASKABLE_INTERRUPT:
      return "NMI: return Non-maskable interrupt";
    case BREAKPOINT:
      return "#BP: return Breakpoint";
    case OVERFLOW:
      return "#OF: return Overflow";
    case BOUND_RANGE_EXCEEDED:
      return "#BR: return Bound range exceeded";
    case INVALID_OPCODE:
      return "#UD: return Invalid opcode";
    case DEVICE_NOT_AVAILABLE:
      return "#NM: return Device not available";
    case DOUBLE_FAULT:
      return "#DF: return Double fault";
    case COPROCESSOR_SEGMENT_OVERRUN:
      return "Coprocessor segment overrun";
    case INVALID_TSS:
      return "#TS: return Invalid TSS";
    case SEGMENT_NOT_PRESENT:
      return "#NP: return Segment not present";
    case STACK_SEGMENT_FAULT:
      return "#SS: return Stack-segment fault";
    case GENERAL_PROTECTION_FAULT:
      return "#GP: return General protection fault";
    case PAGE_FAULT:
      return "#PF: return Page fault";
    case FLOATING_POINT_EXCEPTION:
      return "#MF: return Floating-point exception";
    case ALIGNMENT_CHECK:
      return "#AC: return Alignment check";
    case MACHINE_CHECK:
      return "#MC: return Machine check";
    case SIMD_EXCEPTION:
      return "#XM: return SIMD exception";
    case VIRTUALIZATION_EXCEPTION:
      return "#VE: return Virtualization exception";
    case CONTROL_PROTECTION_EXCEPTON:
      return "#CP: return Control protection exception";
    default:
      return "Unknown exception";
  };
}
