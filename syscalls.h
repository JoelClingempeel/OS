#include <stdint.h>

struct registers {
    // Pushed by pushad in this order:
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
};

// Main syscall function which routes to appropriate syscall.
void do_syscall(struct registers* regs);
