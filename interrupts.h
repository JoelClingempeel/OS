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

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

struct IDTPointer init_idt();

void _idt80(void);

void _idt_timer(void);

void pic_remap(int offset1, int offset2);

void configure_interrupts();
