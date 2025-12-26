#include "interrupts.h"

struct IDTEntry idt[256];

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

void _idt80(void) {
    char hello[] = "AAAAA";
    printk(hello, 0x07);
}

void _idt_timer(void) {
    char *video_memory = (char *)0xb8000;
    video_memory[0] = 'T';
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

void configure_interrupts() {
    struct IDTPointer idt_ptr = init_idt();
    pic_remap(0x20, 0x28);
    asm volatile("lidt %0" : : "m" (idt_ptr));
    outb(0x21, 0xFE);  // Unmask IRQ 0 (Timer) only
    asm volatile("sti");
}
