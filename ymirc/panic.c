#include "panic.h"

#include <stdbool.h>

#include "arch.h"
#include "log.h"

/** Flag to indicate that a panic occurred. */
static bool panicked = false;

void panic(const char *msg) {
  disable_intr();
  LOG_ERROR("PANIC: %s\n", msg);

  if (panicked) {
    LOG_ERROR("Double panic detected. Halting.\n");
    endless_halt();
  }
  panicked = true;

  LOG_ERROR("=== Stack Trace ==============\n");
  print_stack_trace();

  endless_halt();
}
