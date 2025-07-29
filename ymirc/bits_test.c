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

  assert(concat_64(0x123456789abcdef0ULL, 0x9abcdef012345678ULL) ==
         0x9abcdef012345678ULL);
  assert(concat_64(1, 0) == 0x0000000100000000);

  uint64_t val = 0b01010101;
  set_masked_bits(&val, 0b11001100, 0b00001111);
  assert(val == 0b01011100);

  assert(get_lower_bits(0x123456789abcdef0, 16) == 0xdef0);

  unsigned __int128 val128 = 0b01010101;
  set_masked_bits_128(&val128, 0b11001100, 0b00001111);
  assert(val == 0b01011100);

  assert(get_lower_bits_128(0x123456789abcdef0, 16) == 0xdef0);

  puts("PASS");

  return 0;
}
