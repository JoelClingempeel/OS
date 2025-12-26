#include "memory.h"
#include "utils.h"


struct IDTEntry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed));

int x;
struct IDTEntry idt[256];

struct IDTPointer {
    uint16_t limit;  // Occupies bytes 0 and 1
    uint32_t base;   // Occupies bytes 2 through 5
} __attribute__((packed));

extern void handle_interrupt(void);
extern void handle_timer_int(void);

void _idt80(void) {
    char hello[] = "AAAAA";
    printk(hello, 0x07);
}

void _idt_timer(void) {
    char *video_memory = (char *)0xb8000;
    video_memory[0] = 'T';
}

struct IDTPointer init_idt() {
    struct IDTEntry* entry = &idt[0x80];
    uint32_t idt80_addr = (uint32_t)(&handle_interrupt);
    entry->offset_low = idt80_addr & 0xFFFF;
    entry->offset_high = idt80_addr >> 16;
    entry->selector = 0x08;
    entry->zero = 0;
    entry->type_attr = 0xEF;

    // Timer Interrupt (IRQ 0 -> Vector 32)
    struct IDTEntry* timer_entry = &idt[0x20];
    uint32_t timer_addr = (uint32_t)(&handle_timer_int);
    timer_entry->offset_low = timer_addr & 0xFFFF;
    timer_entry->offset_high = timer_addr >> 16;
    timer_entry->selector = 0x08;
    timer_entry->zero = 0;
    // Change type_attr to 0x8E for Hardware/Kernel interrupts
    // 0x8E = 1 (Present) | 00 (Ring 0) | 01110 (32-bit Interrupt Gate)
    timer_entry->type_attr = 0x8E;

    struct IDTPointer idt_ptr;
    idt_ptr.limit = (sizeof(struct IDTEntry) * 256) - 1;
    idt_ptr.base = (uint32_t)&idt;
    return idt_ptr;
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

void pic_remap(int offset1, int offset2) {
    // Start initialization sequence (ICW1)
    outb(0x20, 0x11); 
    outb(0xA0, 0x11);

    // Set Vector Offsets (ICW2)
    outb(0x21, offset1); // Master offset (32)
    outb(0xA1, offset2); // Slave offset (40)

    // Tell Master there is a Slave at IRQ 2 (ICW3)
    outb(0x21, 0x04);
    // Tell Slave its identity is 2 (ICW3)
    outb(0xA1, 0x02);

    // Set 8086 mode (ICW4)
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    // Mask everything to start
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
}

void _kmain(void)
{
    init_mem();

    struct IDTPointer idt_ptr = init_idt();
    pic_remap(0x20, 0x28);
    asm volatile("lidt %0" : : "m" (idt_ptr));
    outb(0x21, 0xFE);  // Unmask IRQ 0 (Timer) only
    asm volatile("sti");

    asm volatile ("int $0x80");  // Test software interrupt.

    while (1) {
        asm("hlt"); 
    }
}
