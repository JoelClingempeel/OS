#include "interrupts.h"
#include "memory.h"
#include "utils.h"


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

void user_test_program() {
    char* video_memory = (char*)0xb8050;
    video_memory[0] = 'U';
    video_memory[1] = 0x0a; // Green color

    // Uncomment to trigger GPF:
    // __asm__ volatile("mov %cr0, %eax");

    while(1) {}
}

extern void jump_to_userland(uint32_t address);

void _kmain(void)
{
    init_mem();
    configure_interrupts();

    init_tss();

    // Uncomment to test software interrupt:
    // asm volatile ("int $0x80");

    jump_to_userland((uint32_t)user_test_program);

    while (1) {
        asm("hlt"); 
    }
}
