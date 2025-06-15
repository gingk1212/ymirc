#pragma once

/** Save host state on the stack, restore guest stete, then execute VMRUN. */
__attribute__((naked)) void asm_vmrun();
