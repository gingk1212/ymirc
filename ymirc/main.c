__attribute__((naked)) void kernel_entry(void) {
    while(1) {
        __asm__ __volatile__ ("hlt");
    }
}
