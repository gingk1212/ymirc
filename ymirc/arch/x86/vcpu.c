#include "vcpu.h"

#include "asm.h"
#include "log.h"

Vcpu vcpu_new(uint16_t asid) { return (Vcpu){.id = 0, .asid = asid}; }

void vcpu_virtualize(Vcpu *vcpu) {
  // Extended Feature Enable Register (EFER)
  uint64_t efer = read_msr(0xC0000080);
  // EFFR.SVME bit
  efer |= 1ULL << 12;
  write_msr(0xC0000080, efer);
}
