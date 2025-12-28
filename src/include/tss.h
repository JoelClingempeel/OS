#ifndef TSS_H
#define TSS_H

#include <stddef.h>
#include <stdint.h>

struct tss_entry {
    uint32_t prev_tss;   // Not used (for hardware multitasking)
    uint32_t esp0;       // The stack pointer the CPU loads when moving to Ring 0
    uint32_t ss0;        // The stack segment the CPU loads when moving to Ring 0
    uint32_t esp1;       // Unused
    uint32_t ss1;        // Unused
    uint32_t esp2;       // Unused
    uint32_t ss2;        // Unused
    uint32_t cr3;        // Unused
    uint32_t eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi; // Unused
    uint32_t es, cs, ss, ds, fs, gs; // Unused
    uint32_t ldt;        // Unused
    uint16_t trap, iomap_base;
} __attribute__((packed));

void init_tss();
void update_tss_esp0(uint32_t kstack_top);

#endif  // TSS_H
