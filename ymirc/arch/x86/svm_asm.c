#include "svm_asm.h"

#include <stddef.h>

#include "svm_common.h"
#include "svm_vcpu.h"

__attribute__((naked)) void asm_vmrun() {
  // Save callee saved registers.
  __asm__ volatile(
      "push %rbp\n\t"
      "push %r15\n\t"
      "push %r14\n\t"
      "push %r13\n\t"
      "push %r12\n\t"
      "push %rbx");

  // Store the physical address of VMCB to RAX for VMRUN.
  // Note: RAX must not be used until VMRUN is executed.
  __asm__ volatile("mov %c[offset](%%rdi), %%rax\n\t"
                   :
                   : [offset] "i"(offsetof(SvmVcpu, vmcb_phys)));

  // Save a pointer to guest registers.
  __asm__ volatile(
      "lea %c[offset](%%rdi), %%rbx\n\t"
      "push %%rbx"
      :
      : [offset] "i"(offsetof(SvmVcpu, guest_regs)));

  // Restore guest registers.
  __asm__ volatile(
      "mov %c[rcx](%%rbx), %%rcx\n\t"
      "mov %c[rdx](%%rbx), %%rdx\n\t"
      "mov %c[rsi](%%rbx), %%rsi\n\t"
      "mov %c[rdi](%%rbx), %%rdi\n\t"
      "mov %c[rbp](%%rbx), %%rbp\n\t"
      "mov %c[r8](%%rbx), %%r8\n\t"
      "mov %c[r9](%%rbx), %%r9\n\t"
      "mov %c[r10](%%rbx), %%r10\n\t"
      "mov %c[r11](%%rbx), %%r11\n\t"
      "mov %c[r12](%%rbx), %%r12\n\t"
      "mov %c[r13](%%rbx), %%r13\n\t"
      "mov %c[r14](%%rbx), %%r14\n\t"
      "mov %c[r15](%%rbx), %%r15\n\t"
      "movaps %c[xmm0](%%rbx), %%xmm0\n\t"
      "movaps %c[xmm1](%%rbx), %%xmm1\n\t"
      "movaps %c[xmm2](%%rbx), %%xmm2\n\t"
      "movaps %c[xmm3](%%rbx), %%xmm3\n\t"
      "movaps %c[xmm4](%%rbx), %%xmm4\n\t"
      "movaps %c[xmm5](%%rbx), %%xmm5\n\t"
      "movaps %c[xmm6](%%rbx), %%xmm6\n\t"
      "movaps %c[xmm7](%%rbx), %%xmm7\n\t"
      "mov %c[rbx](%%rbx), %%rbx\n\t"
      :
      : [rcx] "i"(offsetof(GuestRegisters, rcx)),
        [rdx] "i"(offsetof(GuestRegisters, rdx)),
        [rbx] "i"(offsetof(GuestRegisters, rbx)),
        [rsi] "i"(offsetof(GuestRegisters, rsi)),
        [rdi] "i"(offsetof(GuestRegisters, rdi)),
        [rbp] "i"(offsetof(GuestRegisters, rbp)),
        [r8] "i"(offsetof(GuestRegisters, r8)),
        [r9] "i"(offsetof(GuestRegisters, r9)),
        [r10] "i"(offsetof(GuestRegisters, r10)),
        [r11] "i"(offsetof(GuestRegisters, r11)),
        [r12] "i"(offsetof(GuestRegisters, r12)),
        [r13] "i"(offsetof(GuestRegisters, r13)),
        [r14] "i"(offsetof(GuestRegisters, r14)),
        [r15] "i"(offsetof(GuestRegisters, r15)),
        [xmm0] "i"(offsetof(GuestRegisters, xmm0)),
        [xmm1] "i"(offsetof(GuestRegisters, xmm1)),
        [xmm2] "i"(offsetof(GuestRegisters, xmm2)),
        [xmm3] "i"(offsetof(GuestRegisters, xmm3)),
        [xmm4] "i"(offsetof(GuestRegisters, xmm4)),
        [xmm5] "i"(offsetof(GuestRegisters, xmm5)),
        [xmm6] "i"(offsetof(GuestRegisters, xmm6)),
        [xmm7] "i"(offsetof(GuestRegisters, xmm7)));

  // VMLOAD
  __asm__ volatile("vmload");

  // VMRUN
  __asm__ volatile("vmrun");

  // #VMEXIT

  // VMSAVE
  __asm__ volatile("vmsave");

  // Get &guest_regs.
  __asm__ volatile("pop %rax");

  // Save guest registers.
  __asm__ volatile(
      "mov %%rcx, %c[rcx](%%rax)\n\t"
      "mov %%rdx, %c[rdx](%%rax)\n\t"
      "mov %%rbx, %c[rbx](%%rax)\n\t"
      "mov %%rsi, %c[rsi](%%rax)\n\t"
      "mov %%rdi, %c[rdi](%%rax)\n\t"
      "mov %%rbp, %c[rbp](%%rax)\n\t"
      "mov %%r8, %c[r8](%%rax)\n\t"
      "mov %%r9, %c[r9](%%rax)\n\t"
      "mov %%r10, %c[r10](%%rax)\n\t"
      "mov %%r11, %c[r11](%%rax)\n\t"
      "mov %%r12, %c[r12](%%rax)\n\t"
      "mov %%r13, %c[r13](%%rax)\n\t"
      "mov %%r14, %c[r14](%%rax)\n\t"
      "mov %%r15, %c[r15](%%rax)\n\t"
      "movaps %%xmm0, %c[xmm0](%%rax)\n\t"
      "movaps %%xmm1, %c[xmm1](%%rax)\n\t"
      "movaps %%xmm2, %c[xmm2](%%rax)\n\t"
      "movaps %%xmm3, %c[xmm3](%%rax)\n\t"
      "movaps %%xmm4, %c[xmm4](%%rax)\n\t"
      "movaps %%xmm5, %c[xmm5](%%rax)\n\t"
      "movaps %%xmm6, %c[xmm6](%%rax)\n\t"
      "movaps %%xmm7, %c[xmm7](%%rax)\n\t"
      :
      : [rcx] "i"(offsetof(GuestRegisters, rcx)),
        [rdx] "i"(offsetof(GuestRegisters, rdx)),
        [rbx] "i"(offsetof(GuestRegisters, rbx)),
        [rsi] "i"(offsetof(GuestRegisters, rsi)),
        [rdi] "i"(offsetof(GuestRegisters, rdi)),
        [rbp] "i"(offsetof(GuestRegisters, rbp)),
        [r8] "i"(offsetof(GuestRegisters, r8)),
        [r9] "i"(offsetof(GuestRegisters, r9)),
        [r10] "i"(offsetof(GuestRegisters, r10)),
        [r11] "i"(offsetof(GuestRegisters, r11)),
        [r12] "i"(offsetof(GuestRegisters, r12)),
        [r13] "i"(offsetof(GuestRegisters, r13)),
        [r14] "i"(offsetof(GuestRegisters, r14)),
        [r15] "i"(offsetof(GuestRegisters, r15)),
        [xmm0] "i"(offsetof(GuestRegisters, xmm0)),
        [xmm1] "i"(offsetof(GuestRegisters, xmm1)),
        [xmm2] "i"(offsetof(GuestRegisters, xmm2)),
        [xmm3] "i"(offsetof(GuestRegisters, xmm3)),
        [xmm4] "i"(offsetof(GuestRegisters, xmm4)),
        [xmm5] "i"(offsetof(GuestRegisters, xmm5)),
        [xmm6] "i"(offsetof(GuestRegisters, xmm6)),
        [xmm7] "i"(offsetof(GuestRegisters, xmm7)));

  // Restore callee saved registers.
  __asm__ volatile(
      "pop %rbx\n\t"
      "pop %r12\n\t"
      "pop %r13\n\t"
      "pop %r14\n\t"
      "pop %r15\n\t"
      "pop %rbp\n\t");

  __asm__ volatile("ret");
}
