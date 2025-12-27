#include "interrupts.h"
#include "tty.h"
#include "utils.h"

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

    // --- Double Fault (Vector 8) ---
    struct IDTEntry* df_entry = &idt[8];
    uint32_t df_addr = (uint32_t)(&handle_double_fault);
    df_entry->offset_low = df_addr & 0xFFFF;
    df_entry->offset_high = df_addr >> 16;
    df_entry->selector = 0x08;
    df_entry->zero = 0;
    df_entry->type_attr = 0x8E; 

    // --- General Protection Fault (Vector 13) ---
    struct IDTEntry* gpf_entry = &idt[13];
    uint32_t gpf_addr = (uint32_t)(&handle_gpf);
    gpf_entry->offset_low = gpf_addr & 0xFFFF;
    gpf_entry->offset_high = gpf_addr >> 16;
    gpf_entry->selector = 0x08;
    gpf_entry->zero = 0;
    gpf_entry->type_attr = 0x8E;

    // --- Page Fault (Vector 14) ---
    struct IDTEntry* pf_entry = &idt[14];
    uint32_t pf_addr = (uint32_t)(&handle_page_fault);
    pf_entry->offset_low = pf_addr & 0xFFFF;
    pf_entry->offset_high = pf_addr >> 16;
    pf_entry->selector = 0x08;
    pf_entry->zero = 0;
    pf_entry->type_attr = 0x8E;

    // --- Keyboard Interrupt (IRQ 1 -> Vector 33) ---
    struct IDTEntry* kbd_entry = &idt[0x21];
    uint32_t kbd_addr = (uint32_t)(&handle_keyboard_int);
    kbd_entry->offset_low = kbd_addr & 0xFFFF;
    kbd_entry->offset_high = kbd_addr >> 16;
    kbd_entry->selector = 0x08;
    kbd_entry->zero = 0;
    kbd_entry->type_attr = 0x8E;

    struct IDTPointer idt_ptr;
    idt_ptr.limit = (sizeof(struct IDTEntry) * 256) - 1;
    idt_ptr.base = (uint32_t)&idt;
    return idt_ptr;
}

void _idt80(struct registers* regs) {
    do_syscall(regs);
}

volatile uint32_t timer_ticks = 0;

void _idt_timer(void) {
    timer_ticks++;
    outb(0x20, 0x20);  // Restore interrupts.
}

void _idt_double_fault(uint32_t error_code) {
    char *video_memory = (char *)0xb8000;
    video_memory[0] = 'D';
    video_memory[2] = 'F';
}

void _idt_gpf(uint32_t error_code) {
    char *video_memory = (char *)0xb8000;
    video_memory[0] = 'G';
    video_memory[2] = 'P';
    video_memory[4] = 'F';
}

void _idt_page_fault(uint32_t error_code) {
    char *video_memory = (char *)0xb8000;
    video_memory[0] = 'P';
    video_memory[2] = 'F';
}

void _keyboard_int(){
    uint8_t scancode = inb(0x60);
    if (!(scancode & 0x80)) {
        if (scancode < 128) {
            tty_handle_keyboard(scancode);
        }
    }
    outb(0x20, 0x20);
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
    // Unmask timer and keyboard.
    outb(0x21, 0xFC);
    // outb(0x21, 0x0);
    asm volatile("sti");
}
