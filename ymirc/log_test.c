#include "log.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static char log_buffer[1024];
static size_t log_index = 0;

static void reset_buffer() {
  log_index = 0;
  log_buffer[0] = '\0';
}

static void test_log_writefn(char c) {
  if (log_index < sizeof(log_buffer) - 1) {
    log_buffer[log_index++] = c;
    log_buffer[log_index] = '\0';
  }
}

int main() {
  log_set_writefn(test_log_writefn);

  log_printf(LOG_LEVEL_DEBUG, "Test message\n");
  assert(strcmp(log_buffer, "[DEBUG] Test message\n") == 0);
  reset_buffer();

  log_printf(LOG_LEVEL_INFO, "Test message: %d\n", 42);
  assert(strcmp(log_buffer, "[INFO ] Test message: 42\n") == 0);
  reset_buffer();

  log_printf(LOG_LEVEL_WARN, "Test message: 0x%x\n", 42);
  assert(strcmp(log_buffer, "[WARN ] Test message: 0x000000000000002A\n") == 0);
  reset_buffer();

  void *ptr = (void *)0x123456789A;
  log_printf(LOG_LEVEL_DEBUG, "Test message: %p\n", ptr);
  assert(strcmp(log_buffer, "[DEBUG] Test message: 0x000000123456789A\n") == 0);
  reset_buffer();

  log_printf(LOG_LEVEL_ERROR, "Test message: %s\n", "Hello");
  assert(strcmp(log_buffer, "[ERROR] Test message: Hello\n") == 0);
  reset_buffer();

  log_printf(LOG_LEVEL_ERROR, "Test message: %%\n");
  assert(strcmp(log_buffer, "[ERROR] Test message: %\n") == 0);
  reset_buffer();

  puts("PASS");

  return 0;
}
