#include "memory.h"
#include "utils.h"


char temp_stack_storage[4096]; // A 4KB stack should be enough
extern char temp_stack_storage[];

struct IDTEntry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed));

struct IDTEntry idt[256];

struct IDTEntry idt[256];

struct IDTPointer {
    uint16_t limit;  // Occupies bytes 0 and 1
    uint32_t base;   // Occupies bytes 2 through 5
} __attribute__((packed));

extern void handle_interrupt(void);

void _idt80 (void) {
    char hello[] = "test string";
    printk(hello, 0x07);
}

void _kmain(void)
{
    init_mem();

    struct IDTEntry* entry = &idt[0x80];
    uint32_t idt80_addr = (uint32_t)(&handle_interrupt);
    entry->offset_low = idt80_addr & 0xFFFF;
    entry->offset_high = idt80_addr >> 16;
    entry->selector = 0x08;
    entry->zero = 0;
    entry->type_attr = 0xEF;

    struct IDTPointer idt_ptr;
    idt_ptr.limit = (sizeof(struct IDTEntry) * 256) - 1;
    idt_ptr.base = (uint32_t)&idt;

    asm volatile("lidt %0" : : "m" (idt_ptr));
    asm volatile ("int $0x80"); 

    while (1) {
        asm("hlt"); 
    }
}
