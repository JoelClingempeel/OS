#include "tss.h"

struct tss_entry kernel_tss;

extern uint8_t GDT_start[]; // From your assembly file

void init_tss() {
    // 1. Clear the TSS
    for(size_t i = 0; i < sizeof(struct tss_entry); i++) {
        ((uint8_t*)&kernel_tss)[i] = 0;
    }

    // 2. Set the Ring 0 Stack
    // When an interrupt happens in Userland, the CPU will switch to this stack
    kernel_tss.ss0  = 0x10;      // Your Kernel Data Segment (GDT index 2)
    kernel_tss.esp0 = 0x40000;   // The top of your kernel stack (from your boot code)

    // 3. Patch the GDT Entry (at offset 0x28)
    uint32_t base = (uint32_t)&kernel_tss;
    uint32_t limit = sizeof(struct tss_entry) - 1;
    uint8_t* desc = &GDT_start[0x28];

    // Limit
    desc[0] = (limit & 0xFF);
    desc[1] = (limit >> 8) & 0xFF;

    // Base Address (Split into 3 parts)
    desc[2] = (base & 0xFF);
    desc[3] = (base >> 8) & 0xFF;
    desc[4] = (base >> 16) & 0xFF;
    desc[7] = (base >> 24) & 0xFF;

    // Type/Access (0x89 = Present, Ring 0, 32-bit TSS)
    desc[5] = 0x89;

    // Flags (0x40 = 32-bit size)
    desc[6] = 0x40;

    // 4. Final Step: Load the Task Register
    // This tells the hardware "The TSS is now at GDT index 5 (0x28)"
    __asm__ volatile("ltr %%ax" : : "a"(0x28));
}

void update_tss_esp0(uint32_t kstack_top) {
    kernel_tss.esp0 = kstack_top;
}