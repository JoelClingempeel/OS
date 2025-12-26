#include "utils.h"

struct IDTEntry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed));

struct IDTPointer {
    uint16_t limit;  // Occupies bytes 0 and 1
    uint32_t base;   // Occupies bytes 2 through 5
} __attribute__((packed));

extern void handle_interrupt(void);
extern void handle_timer_int(void);
extern void handle_double_fault(void);
extern void handle_gpf(void);
extern void handle_page_fault(void);

// Get value from port.
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

// Write value to port.
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

// Initialize all interrupts in IDT, and return a pointer to its start.
struct IDTPointer init_idt();

// Handle software interrupt.
void _idt80(void);

// Handle timer interrupt.
void _idt_timer(void);

// Handle double fault interrupt.
void _idt_double_fault(uint32_t error_code);

// Handle GPF interrupt.
void _idt_gpf(uint32_t error_code);

// Handle page fault interrupt.
void _idt_page_fault(uint32_t error_code);

// Remap PIC so hardware interrupts can be properly handled.
void pic_remap(int offset1, int offset2);

// Main function to configure interrupts.
void configure_interrupts();
