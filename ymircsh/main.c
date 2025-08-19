#include <stdint.h>

void asm_vmmcall(uint64_t nr) {
  __asm__ volatile("vmmcall" : : "a"(nr) : "memory");
}

int main() {
  asm_vmmcall(0);
  return 0;
}
