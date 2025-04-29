#include "bits.h"

#include <assert.h>
#include <stdio.h>

int main() {
  assert(tobit(0) == 0b00000001);
  assert(tobit(1) == 0b00000010);
  assert(tobit(10) == 1024);
  assert(tobit(64) == 0);

  assert(isset(0b00011110, 1));
  assert(!isset(0b00011110, 0));
  assert(isset(1024, 10));
  assert(!isset(1, 64));

  assert(concat(0x12345678, 0x9abcdef0) == 0x123456789abcdef0);
  assert(concat(1, 0) == 0x0000000100000000);

  puts("PASS");

  return 0;
}
