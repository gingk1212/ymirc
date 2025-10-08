#include <stdint.h>
#include <stdio.h>

/* BUFMAX defines the maximum number of characters in inbound/outbound buffers*/
/* at least NUMREGBYTES*2 are needed for register packets */
#define BUFMAX 400

static char initialized; /* boolean flag. != 0 means we've been initialized */

int remote_debug;
/*  debug >  0 prints ill-formed commands in valid packets & checksum errors */

static const uint8_t hexchars[] = "0123456789abcdef";

/* Number of registers.  */
#define NUMREGS 16

/* Number of bytes of registers.  */
#define NUMREGBYTES (NUMREGS * 4)

enum regnames {
  EAX,
  ECX,
  EDX,
  EBX,
  ESP,
  EBP,
  ESI,
  EDI,
  PC /* also known as eip */,
  PS /* also known as eflags */,
  CS,
  SS,
  DS,
  ES,
  FS,
  GS
};

#define BREAKPOINT() __asm__ volatile("int $3");

/* Custom string copy function */
static uint8_t *strcpy_local(uint8_t *dest, const uint8_t *src) {
  uint8_t *d = dest;
  while ((*d++ = *src++) != '\0');
  return dest;
}

int hex(uint8_t ch) {
  if ((ch >= 'a') && (ch <= 'f')) return (ch - 'a' + 10);
  if ((ch >= '0') && (ch <= '9')) return (ch - '0');
  if ((ch >= 'A') && (ch <= 'F')) return (ch - 'A' + 10);
  return (-1);
}

static uint8_t remcomInBuffer[BUFMAX];
static uint8_t remcomOutBuffer[BUFMAX];

/* scan for the sequence $<data>#<checksum>     */

uint8_t *getpacket(void) {
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
          fprintf(stderr, "bad checksum.  My count = 0x%x, sent=0x%x. buf=%s\n",
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

void putpacket(const uint8_t *buffer) {
  uint8_t checksum;
  int count;
  uint8_t ch;

  /*  $<packet info>#<checksum>.  */
  do {
    putDebugChar('$');
    checksum = 0;
    count = 0;

    while (ch = buffer[count]) {
      putDebugChar(ch);
      checksum += ch;
      count += 1;
    }

    putDebugChar('#');
    putDebugChar(hexchars[checksum >> 4]);
    putDebugChar(hexchars[checksum % 16]);

  } while (getDebugChar() != '+');
}

void debug_error(const uint8_t *format, const uint8_t *parm) {
  if (remote_debug) fprintf(stderr, (const char *)format, (const char *)parm);
}

/* Address of a routine to RTE to if we get a memory fault.  */
static void (*volatile mem_fault_routine)() = NULL;

/* Indicate to caller of mem2hex or hex2mem that there has been an
   error.  */
static volatile int mem_err = 0;

void set_mem_err(void) { mem_err = 1; }

/* These are separate functions so that they are so short and sweet
   that the compiler won't save any registers (if there is a fault
   to mem_fault, they won't get restored, so there better not be any
   saved).  */
uint8_t get_char(const uint8_t *addr) { return *addr; }

void set_char(uint8_t *addr, uint8_t val) { *addr = val; }

/* convert the memory pointed to by mem into hex, placing result in buf */
/* return a pointer to the last char put in buf (null) */
/* If MAY_FAULT is non-zero, then we should set mem_err in response to
   a fault; if zero treat a fault like any other fault in the stub.  */
uint8_t *mem2hex(const uint8_t *mem, uint8_t *buf, int count, int may_fault) {
  int i;
  uint8_t ch;

  if (may_fault) mem_fault_routine = set_mem_err;
  for (i = 0; i < count; i++) {
    ch = get_char(mem++);
    if (may_fault && mem_err) return (buf);
    *buf++ = hexchars[ch >> 4];
    *buf++ = hexchars[ch % 16];
  }
  *buf = 0;
  if (may_fault) mem_fault_routine = NULL;
  return (buf);
}

/* convert the hex array pointed to by buf into binary to be placed in mem */
/* return a pointer to the character AFTER the last byte written */
uint8_t *hex2mem(const uint8_t *buf, uint8_t *mem, int count, int may_fault) {
  int i;
  uint8_t ch;

  if (may_fault) mem_fault_routine = set_mem_err;
  for (i = 0; i < count; i++) {
    ch = hex(*buf++) << 4;
    ch = ch + hex(*buf++);
    set_char(mem++, ch);
    if (may_fault && mem_err) return (mem);
  }
  if (may_fault) mem_fault_routine = NULL;
  return (mem);
}

/* this function takes the 386 exception vector and attempts to
   translate this number into a unix compatible signal value */
int computeSignal(int exceptionVector) {
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
int hexToInt(const uint8_t **ptr, int *intValue) {
  int numChars = 0;
  int hexValue;

  *intValue = 0;

  while (**ptr) {
    hexValue = hex(**ptr);
    if (hexValue >= 0) {
      *intValue = (*intValue << 4) | hexValue;
      numChars++;
    } else
      break;

    (*ptr)++;
  }

  return (numChars);
}

/*
 * This function does all command procesing for interfacing to gdb.
 */
void handle_exception(int exceptionVector) {
  int sigval, stepping;
  int addr, length;
  const uint8_t *ptr;
  int newPC;

  if (remote_debug) {
    printf("vector=%d, sr=0x%x, pc=0x%x\n", exceptionVector, registers[PS],
           registers[PC]);
  }

  /* reply to host that an exception has occurred */
  sigval = computeSignal(exceptionVector);

  uint8_t *out_ptr = remcomOutBuffer;

  *out_ptr++ = 'T'; /* notify gdb with signo, PC, FP and SP */
  *out_ptr++ = hexchars[sigval >> 4];
  *out_ptr++ = hexchars[sigval & 0xf];

  *out_ptr++ = hexchars[ESP];
  *out_ptr++ = ':';
  out_ptr = mem2hex((const uint8_t *)&registers[ESP], out_ptr, 4, 0); /* SP */
  *out_ptr++ = ';';

  *out_ptr++ = hexchars[EBP];
  *out_ptr++ = ':';
  out_ptr = mem2hex((const uint8_t *)&registers[EBP], out_ptr, 4, 0); /* FP */
  *out_ptr++ = ';';

  *out_ptr++ = hexchars[PC];
  *out_ptr++ = ':';
  out_ptr = mem2hex((const uint8_t *)&registers[PC], out_ptr, 4, 0); /* PC */
  *out_ptr++ = ';';

  *out_ptr = '\0';

  putpacket(remcomOutBuffer);

  stepping = 0;

  while (1 == 1) {
    remcomOutBuffer[0] = 0;
    ptr = getpacket();

    switch (*ptr++) {
      case '?':
        remcomOutBuffer[0] = 'S';
        remcomOutBuffer[1] = hexchars[sigval >> 4];
        remcomOutBuffer[2] = hexchars[sigval % 16];
        remcomOutBuffer[3] = 0;
        break;
      case 'd':
        remote_debug = !(remote_debug); /* toggle debug flag */
        break;
      case 'g': /* return the value of the CPU registers */
        mem2hex((const uint8_t *)registers, remcomOutBuffer, NUMREGBYTES, 0);
        break;
      case 'G': /* set the value of the CPU registers - return OK */
        hex2mem(ptr, (uint8_t *)registers, NUMREGBYTES, 0);
        strcpy_local(remcomOutBuffer, (const uint8_t *)"OK");
        break;
      case 'P': /* set the value of a single CPU register - return OK */
      {
        int regno;

        if (hexToInt(&ptr, &regno) && *ptr++ == '=')
          if (regno >= 0 && regno < NUMREGS) {
            hex2mem(ptr, (uint8_t *)&registers[regno], 4, 0);
            strcpy_local(remcomOutBuffer, (const uint8_t *)"OK");
            break;
          }

        strcpy_local(remcomOutBuffer, (const uint8_t *)"E01");
        break;
      }

        /* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
      case 'm':
        /* TRY TO READ %x,%x.  IF SUCCEED, SET PTR = 0 */
        if (hexToInt(&ptr, &addr))
          if (*(ptr++) == ',')
            if (hexToInt(&ptr, &length)) {
              ptr = 0;
              mem_err = 0;
              mem2hex((const uint8_t *)addr, remcomOutBuffer, length, 1);
              if (mem_err) {
                strcpy_local(remcomOutBuffer, (const uint8_t *)"E03");
                debug_error((const uint8_t *)"memory fault", NULL);
              }
            }

        if (ptr) {
          strcpy_local(remcomOutBuffer, (const uint8_t *)"E01");
        }
        break;

        /* MAA..AA,LLLL: Write LLLL bytes at address AA.AA return OK */
      case 'M':
        /* TRY TO READ '%x,%x:'.  IF SUCCEED, SET PTR = 0 */
        if (hexToInt(&ptr, &addr))
          if (*(ptr++) == ',')
            if (hexToInt(&ptr, &length))
              if (*(ptr++) == ':') {
                mem_err = 0;
                hex2mem(ptr, (uint8_t *)addr, length, 1);

                if (mem_err) {
                  strcpy_local(remcomOutBuffer, (const uint8_t *)"E03");
                  debug_error((const uint8_t *)"memory fault", NULL);
                } else {
                  strcpy_local(remcomOutBuffer, (const uint8_t *)"OK");
                }

                ptr = 0;
              }
        if (ptr) {
          strcpy_local(remcomOutBuffer, (const uint8_t *)"E02");
        }
        break;

        /* cAA..AA    Continue at address AA..AA(optional) */
        /* sAA..AA   Step one instruction from AA..AA(optional) */
      case 's':
        stepping = 1;
      case 'c':
        /* try to read optional parameter, pc unchanged if no parm */
        if (hexToInt(&ptr, &addr)) registers[PC] = addr;

        newPC = registers[PC];

        /* clear the trace bit */
        registers[PS] &= 0xfffffeff;

        /* set the trace bit if we're stepping */
        if (stepping) registers[PS] |= 0x100;

        _returnFromException(); /* this is a jump */
        break;

        /* kill the program */
      case 'k': /* do nothing */
#if 0
        /* Huh? This doesn't look like "nothing".
           m68k-stub.c and sparc-stub.c don't have it.  */
        BREAKPOINT();
#endif
        break;
    } /* switch */

    /* reply to the request */
    putpacket(remcomOutBuffer);
  }
}

/* this function is used to set up exception handlers for tracing and
   breakpoints */
void set_debug_traps(void) { initialized = 1; }

/* This function will generate a breakpoint exception.  It is used at the
   beginning of a program to sync up with a debugger and can be used
   otherwise as a quick means to stop program execution and "break" into
   the debugger.  */

void breakpoint(void) {
  if (initialized) BREAKPOINT();
}
