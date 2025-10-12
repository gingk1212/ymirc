#ifdef CONFIG_GDBSTUB

#include "gdbstub.h"

#include <stddef.h>
#include <stdint.h>

#include "interrupt.h"
#include "log.h"
#include "serial.h"

/* BUFMAX defines the maximum number of characters in inbound/outbound buffers*/
/* at least NUMREGBYTES*2 are needed for register packets */
#define BUFMAX 400

static Serial gdb_serial;

/* boolean flag. != 0 means we've been initialized */
static char initialized;

/*  debug >  0 prints ill-formed commands in valid packets & checksum errors */
int remote_debug;

static const uint8_t hexchars[] = "0123456789abcdef";

/* Number of registers.  */
#define NUMREGS_64 17
#define NUMREGS_32 7

/* Number of bytes of registers.  */
#define NUMREGBYTES (NUMREGS_64 * 8 + NUMREGS_32 * 4)

enum regnames {
  // 64-bit
  RAX,
  RBX,
  RCX,
  RDX,
  RSI,
  RDI,
  RBP,
  RSP,
  R8,
  R9,
  R10,
  R11,
  R12,
  R13,
  R14,
  R15,
  RIP,
  // 32-bit
  EFLAGS,
  CS,
  SS,
  DS,
  ES,
  FS,
  GS
};

#define BREAKPOINT() __asm__ volatile("int $3");

static void handle_exception(Context *ctx);

/** Send a charcter to GDB. */
static void putDebugChar(uint8_t c) { serial_write(&gdb_serial, c); }

/** Read a charcter from GDB. */
static uint8_t getDebugChar() { return serial_read_blocking(&gdb_serial); }

/* Custom string copy function */
static uint8_t *strcpy_local(uint8_t *dest, const uint8_t *src) {
  uint8_t *d = dest;
  while ((*d++ = *src++) != '\0');
  return dest;
}

static int hex(uint8_t ch) {
  if ((ch >= 'a') && (ch <= 'f')) return (ch - 'a' + 10);
  if ((ch >= '0') && (ch <= '9')) return (ch - '0');
  if ((ch >= 'A') && (ch <= 'F')) return (ch - 'A' + 10);
  return (-1);
}

static uint8_t remcomInBuffer[BUFMAX];
static uint8_t remcomOutBuffer[BUFMAX];

/* Check if string starts with a given prefix. */
static int starts_with(const uint8_t *str, const uint8_t *prefix) {
  while (*prefix != '\0') {
    if (*str != *prefix) {
      return 0;
    }
    str++;
    prefix++;
  }
  return 1;
}

/* Convert an integer value to hexadecimal string and append to buffer. */
static void append_hex_value(uint8_t *dest, int value) {
  int i;
  int started = 0;
  uint8_t *buf_ptr = dest;

  // Find end of current string
  while (*buf_ptr) buf_ptr++;

  // Convert value to hex string (process 32-bit value, 4 bits at a time)
  for (i = 28; i >= 0; i -= 4) {
    int nibble = (value >> i) & 0xF;
    if (nibble != 0 || started || i == 0) {
      *buf_ptr++ = hexchars[nibble];
      started = 1;
    }
  }
  *buf_ptr = '\0';
}

/* scan for the sequence $<data>#<checksum> */
static uint8_t *getpacket(void) {
  uint8_t *buffer = &remcomInBuffer[0];
  uint8_t checksum;
  uint8_t xmitcsum;
  int count;
  uint8_t ch;

  while (1) {
    /* wait around for the start character, ignore all other characters */
    while ((ch = getDebugChar()) != '$');

  retry:
    checksum = 0;
    xmitcsum = -1;
    count = 0;

    /* now, read until a # or end of buffer is found */
    while (count < BUFMAX - 1) {
      ch = getDebugChar();
      if (ch == '$') goto retry;
      if (ch == '#') break;
      checksum = checksum + ch;
      buffer[count] = ch;
      count = count + 1;
    }
    buffer[count] = 0;

    if (ch == '#') {
      ch = getDebugChar();
      xmitcsum = hex(ch) << 4;
      ch = getDebugChar();
      xmitcsum += hex(ch);

      if (checksum != xmitcsum) {
        if (remote_debug) {
          LOG_ERROR("bad checksum.  My count = 0x%x, sent=0x%x. buf=%s\n",
                    checksum, xmitcsum, buffer);
        }
        putDebugChar('-'); /* failed checksum */
      } else {
        putDebugChar('+'); /* successful transfer */

        /* if a sequence char is present, reply the sequence ID */
        if (buffer[2] == ':') {
          putDebugChar(buffer[0]);
          putDebugChar(buffer[1]);

          return &buffer[3];
        }

        return &buffer[0];
      }
    }
  }
}

/* send the packet in buffer.  */
static void putpacket(const uint8_t *buffer) {
  uint8_t checksum;
  int count;
  uint8_t ch;

  /*  $<packet info>#<checksum>.  */
  do {
    putDebugChar('$');
    checksum = 0;
    count = 0;

    while ((ch = buffer[count]) != '\0') {
      putDebugChar(ch);
      checksum += ch;
      count += 1;
    }

    putDebugChar('#');
    putDebugChar(hexchars[checksum >> 4]);
    putDebugChar(hexchars[checksum % 16]);

  } while (getDebugChar() != '+');
}

static void debug_error(const uint8_t *format, const uint8_t *parm) {
  if (remote_debug) LOG_ERROR((const char *)format, (const char *)parm);
}

/* Indicate to caller of mem2hex or hex2mem that there has been an
   error.  */
static volatile int mem_err = 0;

/* Fault address for debugging purposes. */
static volatile uint64_t gdb_fault_address = 0;

/* Memory fault handler for safe memory access during GDB operations. */
static void gdb_memory_fault_handler(Context *ctx) {
  mem_err = 1;

  // Get fault address from CR2 register for page faults.
  if (ctx->vector == 14) {
    __asm__ volatile("movq %%cr2, %0" : "=r"(gdb_fault_address));
  }

  // TODO: Skip the faulting instruction by advancing RIP. For simple MOV
  // instructions, we use a basic increment.
  // This is a simplified approach - in production, proper instruction decoding
  // would be required for accurate instruction length.
  ctx->rip += 1;
}

/* These are separate functions so that they are so short and sweet
   that the compiler won't save any registers (if there is a fault
   to mem_fault, they won't get restored, so there better not be any
   saved).  */
static uint8_t get_char(const uint8_t *addr) { return *addr; }

static void set_char(uint8_t *addr, uint8_t val) { *addr = val; }

/* Safe memory read function with fault detection */
static uint8_t safe_get_char(const uint8_t *addr) {
  // Reset error flag.
  mem_err = 0;

  // Temporarily register fault handlers for memory exceptions.
  register_handler(13, gdb_memory_fault_handler);  // General Protection Fault
  register_handler(14, gdb_memory_fault_handler);  // Page Fault

  uint8_t result = *addr;

  // If fault occurred, return 0 as safe default.
  if (mem_err) {
    result = 0;
  }

  // Restore original exception handler.
  register_handler(13, handle_exception);
  register_handler(14, handle_exception);

  return result;
}

/* Safe memory write function with fault detection. */
static void safe_set_char(uint8_t *addr, uint8_t val) {
  // Reset error flag.
  mem_err = 0;

  // Temporarily register fault handlers for memory exceptions.
  register_handler(13, gdb_memory_fault_handler);  // General Protection Fault
  register_handler(14, gdb_memory_fault_handler);  // Page Fault

  *addr = val;

  // Restore original exception handler.
  register_handler(13, handle_exception);
  register_handler(14, handle_exception);
}

/* convert the memory pointed to by mem into hex, placing result in buf */
/* return a pointer to the last char put in buf (null) */
/* If MAY_FAULT is non-zero, use safe memory access with fault detection;
   if zero treat a fault like any other fault in the stub.  */
static uint8_t *mem2hex(const uint8_t *mem, uint8_t *buf, int count,
                        int may_fault) {
  int i;
  uint8_t ch;

  for (i = 0; i < count; i++) {
    if (may_fault) {
      ch = safe_get_char(mem++);
      if (mem_err) return buf;  // Exit early on fault.
    } else {
      ch = get_char(mem++);
    }
    *buf++ = hexchars[ch >> 4];
    *buf++ = hexchars[ch % 16];
  }
  *buf = 0;
  return buf;
}

/* convert the hex array pointed to by buf into binary to be placed in mem */
/* return a pointer to the character AFTER the last byte written */
/* If MAY_FAULT is non-zero, use safe memory access with fault detection */
static uint8_t *hex2mem(const uint8_t *buf, uint8_t *mem, int count,
                        int may_fault) {
  int i;
  uint8_t ch;

  for (i = 0; i < count; i++) {
    ch = hex(*buf++) << 4;
    ch = ch + hex(*buf++);
    if (may_fault) {
      safe_set_char(mem++, ch);
      if (mem_err) return mem;  // Exit early on fault.
    } else {
      set_char(mem++, ch);
    }
  }
  return mem;
}

/* this function takes the 386 exception vector and attempts to
   translate this number into a unix compatible signal value */
static int computeSignal(uint64_t exceptionVector) {
  int sigval;
  switch (exceptionVector) {
    case 0:
      sigval = 8;
      break; /* divide by zero */
    case 1:
      sigval = 5;
      break; /* debug exception */
    case 3:
      sigval = 5;
      break; /* breakpoint */
    case 4:
      sigval = 16;
      break; /* into instruction (overflow) */
    case 5:
      sigval = 16;
      break; /* bound instruction */
    case 6:
      sigval = 4;
      break; /* Invalid opcode */
    case 7:
      sigval = 8;
      break; /* coprocessor not available */
    case 8:
      sigval = 7;
      break; /* double fault */
    case 9:
      sigval = 11;
      break; /* coprocessor segment overrun */
    case 10:
      sigval = 11;
      break; /* Invalid TSS */
    case 11:
      sigval = 11;
      break; /* Segment not present */
    case 12:
      sigval = 11;
      break; /* stack exception */
    case 13:
      sigval = 11;
      break; /* general protection */
    case 14:
      sigval = 11;
      break; /* page fault */
    case 16:
      sigval = 7;
      break; /* coprocessor error */
    default:
      sigval = 7; /* "software generated" */
  }
  return (sigval);
}

/**********************************************/
/* WHILE WE FIND NICE HEX CHARS, BUILD AN INT */
/* RETURN NUMBER OF CHARS PROCESSED           */
/**********************************************/
static int hexToNum(const uint8_t **ptr, uint64_t *num) {
  int numChars = 0;
  int hexValue;

  *num = 0;

  while (**ptr) {
    hexValue = hex(**ptr);
    if (hexValue >= 0) {
      *num = (*num << 4) | hexValue;
      numChars++;
    } else
      break;

    (*ptr)++;
  }

  return (numChars);
}

/* Pack CPU registers from Context into a buffer following regnames order */
static void pack_registers(Context *ctx, uint8_t *buffer) {
  uint8_t *buf_ptr = buffer;

  /* 64-bit registers (8 bytes each) - following regnames order */
  *(uint64_t *)buf_ptr = ctx->registers.rax;
  buf_ptr += 8; /* RAX */
  *(uint64_t *)buf_ptr = ctx->registers.rbx;
  buf_ptr += 8; /* RBX */
  *(uint64_t *)buf_ptr = ctx->registers.rcx;
  buf_ptr += 8; /* RCX */
  *(uint64_t *)buf_ptr = ctx->registers.rdx;
  buf_ptr += 8; /* RDX */
  *(uint64_t *)buf_ptr = ctx->registers.rsi;
  buf_ptr += 8; /* RSI */
  *(uint64_t *)buf_ptr = ctx->registers.rdi;
  buf_ptr += 8; /* RDI */
  *(uint64_t *)buf_ptr = ctx->registers.rbp;
  buf_ptr += 8; /* RBP */
  *(uint64_t *)buf_ptr = ctx->registers.rsp;
  buf_ptr += 8; /* RSP */
  *(uint64_t *)buf_ptr = ctx->registers.r8;
  buf_ptr += 8; /* R8 */
  *(uint64_t *)buf_ptr = ctx->registers.r9;
  buf_ptr += 8; /* R9 */
  *(uint64_t *)buf_ptr = ctx->registers.r10;
  buf_ptr += 8; /* R10 */
  *(uint64_t *)buf_ptr = ctx->registers.r11;
  buf_ptr += 8; /* R11 */
  *(uint64_t *)buf_ptr = ctx->registers.r12;
  buf_ptr += 8; /* R12 */
  *(uint64_t *)buf_ptr = ctx->registers.r13;
  buf_ptr += 8; /* R13 */
  *(uint64_t *)buf_ptr = ctx->registers.r14;
  buf_ptr += 8; /* R14 */
  *(uint64_t *)buf_ptr = ctx->registers.r15;
  buf_ptr += 8; /* R15 */
  *(uint64_t *)buf_ptr = ctx->rip;
  buf_ptr += 8; /* RIP */

  /* 32-bit registers (4 bytes each) */
  *(uint32_t *)buf_ptr = (uint32_t)(ctx->rflags & 0xFFFFFFFF);
  buf_ptr += 4; /* EFLAGS */
  *(uint32_t *)buf_ptr = (uint32_t)(ctx->cs & 0xFFFF);
  buf_ptr += 4; /* CS */
  *(uint32_t *)buf_ptr = 0;
  buf_ptr += 4; /* TODO: SS */
  *(uint32_t *)buf_ptr = 0;
  buf_ptr += 4; /* TODO: DS */
  *(uint32_t *)buf_ptr = 0;
  buf_ptr += 4; /* TODO: ES */
  *(uint32_t *)buf_ptr = 0;
  buf_ptr += 4; /* TODO: FS */
  *(uint32_t *)buf_ptr = 0;
  buf_ptr += 4; /* TODO: GS */
}

/* Send initial exception notification to GDB. */
static void send_exception_notification(Context *ctx) {
  int sigval = computeSignal(ctx->vector);
  uint8_t *out_ptr = remcomOutBuffer;

  *out_ptr++ = 'T'; /* notify gdb with signo, PC, FP and SP */
  *out_ptr++ = hexchars[sigval >> 4];
  *out_ptr++ = hexchars[sigval & 0xf];

  *out_ptr++ = hexchars[RSP];
  *out_ptr++ = ':';
  out_ptr = mem2hex((const uint8_t *)&ctx->registers.rsp, out_ptr, 8, 0);
  *out_ptr++ = ';';

  *out_ptr++ = hexchars[RBP];
  *out_ptr++ = ':';
  out_ptr = mem2hex((const uint8_t *)&ctx->registers.rbp, out_ptr, 8, 0);
  *out_ptr++ = ';';

  *out_ptr++ = hexchars[RIP];
  *out_ptr++ = ':';
  out_ptr = mem2hex((const uint8_t *)&ctx->rip, out_ptr, 8, 0);
  *out_ptr++ = ';';

  *out_ptr = '\0';

  putpacket(remcomOutBuffer);
  LOG_DEBUG("gdbstub >> %s\n", remcomOutBuffer);
}

/* Handle '?' command - report signal. */
static int handle_query_signal(Context *ctx) {
  int sigval = computeSignal(ctx->vector);
  remcomOutBuffer[0] = 'S';
  remcomOutBuffer[1] = hexchars[sigval >> 4];
  remcomOutBuffer[2] = hexchars[sigval % 16];
  remcomOutBuffer[3] = 0;
  return 0;
}

/* Handle 'd' command - toggle debug mode. */
static int handle_toggle_debug(void) {
  remote_debug = !(remote_debug);
  return 0;
}

/* Handle 'g' command - read all registers. */
static int handle_read_registers(Context *ctx) {
  uint8_t register_buffer[NUMREGBYTES];
  pack_registers(ctx, register_buffer);
  mem2hex(register_buffer, remcomOutBuffer, NUMREGBYTES, 0);
  return 0;
}

/* Handle 'p' command - read single register. */
static int handle_read_register(Context *ctx, const uint8_t *ptr) {
  uint64_t regno;

  if (hexToNum(&ptr, &regno)) {
    uint32_t reg_value_32;

    switch (regno) {
      case RAX:
        mem2hex((const uint8_t *)&ctx->registers.rax, remcomOutBuffer, 8, 0);
        break;
      case RBX:
        mem2hex((const uint8_t *)&ctx->registers.rbx, remcomOutBuffer, 8, 0);
        break;
      case RCX:
        mem2hex((const uint8_t *)&ctx->registers.rcx, remcomOutBuffer, 8, 0);
        break;
      case RDX:
        mem2hex((const uint8_t *)&ctx->registers.rdx, remcomOutBuffer, 8, 0);
        break;
      case RSI:
        mem2hex((const uint8_t *)&ctx->registers.rsi, remcomOutBuffer, 8, 0);
        break;
      case RDI:
        mem2hex((const uint8_t *)&ctx->registers.rdi, remcomOutBuffer, 8, 0);
        break;
      case RBP:
        mem2hex((const uint8_t *)&ctx->registers.rbp, remcomOutBuffer, 8, 0);
        break;
      case RSP:
        mem2hex((const uint8_t *)&ctx->registers.rsp, remcomOutBuffer, 8, 0);
        break;
      case R8:
        mem2hex((const uint8_t *)&ctx->registers.r8, remcomOutBuffer, 8, 0);
        break;
      case R9:
        mem2hex((const uint8_t *)&ctx->registers.r9, remcomOutBuffer, 8, 0);
        break;
      case R10:
        mem2hex((const uint8_t *)&ctx->registers.r10, remcomOutBuffer, 8, 0);
        break;
      case R11:
        mem2hex((const uint8_t *)&ctx->registers.r11, remcomOutBuffer, 8, 0);
        break;
      case R12:
        mem2hex((const uint8_t *)&ctx->registers.r12, remcomOutBuffer, 8, 0);
        break;
      case R13:
        mem2hex((const uint8_t *)&ctx->registers.r13, remcomOutBuffer, 8, 0);
        break;
      case R14:
        mem2hex((const uint8_t *)&ctx->registers.r14, remcomOutBuffer, 8, 0);
        break;
      case R15:
        mem2hex((const uint8_t *)&ctx->registers.r15, remcomOutBuffer, 8, 0);
        break;
      case RIP:
        mem2hex((const uint8_t *)&ctx->rip, remcomOutBuffer, 8, 0);
        break;
      case EFLAGS:
        reg_value_32 = (uint32_t)(ctx->rflags & 0xFFFFFFFF);
        mem2hex((const uint8_t *)&reg_value_32, remcomOutBuffer, 4, 0);
        break;
      case CS:
        reg_value_32 = (uint32_t)(ctx->cs & 0xFFFF);
        mem2hex((const uint8_t *)&reg_value_32, remcomOutBuffer, 4, 0);
        break;
      case SS:
      case DS:
      case ES:
      case FS:
      case GS:
        reg_value_32 = 0;
        mem2hex((const uint8_t *)&reg_value_32, remcomOutBuffer, 4, 0);
        break;
      default:
        strcpy_local(remcomOutBuffer, (const uint8_t *)"E01");
        break;
    }
  } else {
    strcpy_local(remcomOutBuffer, (const uint8_t *)"E01");
  }
  return 0;
}

/* Handle 'm' command - read memory. */
static int handle_read_memory(const uint8_t *ptr) {
  uint64_t addr, length;

  if (hexToNum(&ptr, &addr) && *(ptr++) == ',' && hexToNum(&ptr, &length)) {
    mem2hex((const uint8_t *)addr, remcomOutBuffer, length, 1);
    if (mem_err) {
      strcpy_local(remcomOutBuffer, (const uint8_t *)"E03");
      debug_error((const uint8_t *)"memory fault", NULL);
    }
  } else {
    strcpy_local(remcomOutBuffer, (const uint8_t *)"E01");
  }
  return 0;
}

/* Handle 'M' command - write memory. */
static int handle_write_memory(const uint8_t *ptr) {
  uint64_t addr, length;

  if (hexToNum(&ptr, &addr) && *(ptr++) == ',' && hexToNum(&ptr, &length) &&
      *(ptr++) == ':') {
    hex2mem(ptr, (uint8_t *)addr, length, 1);
    if (mem_err) {
      strcpy_local(remcomOutBuffer, (const uint8_t *)"E03");
      debug_error((const uint8_t *)"memory fault", NULL);
    } else {
      strcpy_local(remcomOutBuffer, (const uint8_t *)"OK");
    }
  } else {
    strcpy_local(remcomOutBuffer, (const uint8_t *)"E02");
  }
  return 0;
}

/* Handle 'q' command - query commands. */
static int handle_query(const uint8_t *ptr) {
  if (starts_with(ptr, (const uint8_t *)"Supported")) {
    strcpy_local(remcomOutBuffer, (const uint8_t *)"PacketSize=");
    append_hex_value(remcomOutBuffer, BUFMAX);
  } else if (starts_with(ptr, (const uint8_t *)"C")) {
    strcpy_local(remcomOutBuffer, (const uint8_t *)"QC1");
  } else if (starts_with(ptr, (const uint8_t *)"fThreadInfo")) {
    strcpy_local(remcomOutBuffer, (const uint8_t *)"m1");
  } else if (starts_with(ptr, (const uint8_t *)"sThreadInfo")) {
    strcpy_local(remcomOutBuffer, (const uint8_t *)"l");
  } else {
    remcomOutBuffer[0] = '\0';
  }
  return 0;
}

/* Handle 'H' command - set thread. */
static int handle_set_thread(const uint8_t *ptr) {
  if (*ptr == 'g' || *ptr == 'c') {
    int is_negative = 0;
    uint64_t num;
    ptr++;

    if (*ptr == '-') {
      is_negative = 1;
      ptr++;
    }

    if (hexToNum(&ptr, &num) > 0 &&
        ((is_negative && num == 1) || (!is_negative && num == 0) ||
         (!is_negative && num == 1))) {
      strcpy_local(remcomOutBuffer, (const uint8_t *)"OK");
    } else {
      strcpy_local(remcomOutBuffer, (const uint8_t *)"E01");
    }
  } else {
    strcpy_local(remcomOutBuffer, (const uint8_t *)"E01");
  }
  return 0;
}

/* Handle continue/step commands - returns 1 to exit command loop. */
static int handle_continue_step(Context *ctx, const uint8_t *ptr,
                                int stepping) {
  uint64_t addr;

  // try to read optional parameter, pc unchanged if no parm.
  if (hexToNum(&ptr, &addr)) {
    ctx->rip = addr;
  }

  // clear the trace bit.
  ctx->rflags &= 0xfffffffffffffeff;

  // set the trace bit if we're stepping.
  if (stepping) {
    ctx->rflags |= 0x100;
  }

  return 1;  // Exit command loop.
}

/*
 * This function does all command procesing for interfacing to gdb.
 */
static void handle_exception(Context *ctx) {
  const uint8_t *ptr;
  int should_exit = 0;

  if (remote_debug) {
    LOG_DEBUG("vector=%d, sr=0x%x, pc=0x%x\n", ctx->vector, ctx->rflags,
              ctx->rip);
  }

  // reply to host that an exception has occurred.
  send_exception_notification(ctx);

  while (1 == 1) {
    remcomOutBuffer[0] = 0;
    ptr = getpacket();
    LOG_DEBUG("gdbstub << %s\n", remcomInBuffer);

    uint8_t cmd = *ptr++;

    switch (cmd) {
      case '?':
        should_exit = handle_query_signal(ctx);
        break;
      case 'd':
        should_exit = handle_toggle_debug();
        break;
      case 'g':
        should_exit = handle_read_registers(ctx);
        break;
      case 'p':
        should_exit = handle_read_register(ctx, ptr);
        break;
      case 'm':
        should_exit = handle_read_memory(ptr);
        break;
      case 'M':
        should_exit = handle_write_memory(ptr);
        break;
      case 's':
        should_exit = handle_continue_step(ctx, ptr, 1);
        break;
      case 'c':
        should_exit = handle_continue_step(ctx, ptr, 0);
        break;
      case 'k':
        /* kill the program - do nothing */
        break;
      case 'q':
        should_exit = handle_query(ptr);
        break;
      case 'H':
        should_exit = handle_set_thread(ptr);
        break;
      default:
        /* Unknown command - return empty response */
        remcomOutBuffer[0] = '\0';
        break;
    }

    if (should_exit) return;

    /* reply to the request */
    putpacket(remcomOutBuffer);
    LOG_DEBUG("gdbstub >> %s\n", remcomOutBuffer);
  }
}

void gdbstub_init(void) {
  // Initialize COM2 for GDB communication
  serial_init(&gdb_serial, SERIAL_PORT_COM2, 115200);

  // Set up exception handlers for tracing and breakpoints
  register_handler(0, handle_exception);
  register_handler(1, handle_exception);
  register_handler(3, handle_exception);
  register_handler(4, handle_exception);
  register_handler(5, handle_exception);
  register_handler(6, handle_exception);
  register_handler(7, handle_exception);
  register_handler(8, handle_exception);
  register_handler(9, handle_exception);
  register_handler(10, handle_exception);
  register_handler(11, handle_exception);
  register_handler(12, handle_exception);
  register_handler(13, handle_exception);
  register_handler(14, handle_exception);
  register_handler(16, handle_exception);

  initialized = 1;
}

void gdbstub_breakpoint(void) {
  if (initialized) BREAKPOINT();
}

#endif /* CONFIG_GDBSTUB */
