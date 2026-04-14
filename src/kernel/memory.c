#include "memory.h"
#include "interrupts.h"


uint32_t* pd_addr;
uint32_t* pt_addr;
uint8_t physical_memory_bitmap[NUM_FRAMES / 8];
uint32_t vram_border;

uint8_t free_list_buffer[4096];
free_list_node_t* free_list_start;

extern uint8_t _kernel_end;

static void serial_putc(char c) {
    outb(0x3F8, c);
}

static void serial_print(const char* s) {
    for (int i = 0; s[i] != '\0'; i++) {
        serial_putc(s[i]);
    }
}

// Print a string literal safely without relying on .rodata.
// Use a local array so the string lives on the stack, not in .rodata.
#define SERIAL_PRINT(str) do { char _m[] = str; serial_print(_m); } while(0)

static void serial_print_uint(uint32_t num) {
    char digits[10];
    int i = 0;
    if (num == 0) { serial_putc('0'); return; }
    while (num > 0) {
        digits[i++] = '0' + (num % 10);
        num /= 10;
    }
    while (i > 0) {
        serial_putc(digits[--i]);
    }
}

void init_mem() {
    vram_border = 0x400000;

    __asm__ __volatile__(
        "movl %%cr3, %0\n"
        : "=r" (pd_addr)
        :
        :
    );

    for (int i = 1; i < 1024; i++) {
        pd_addr[i] = i * 0x400000;
        pd_addr[i] |= FLAG_PRESENT | FLAG_RW | FLAG_PSE;
    }

    // Mark all frames occupied by the kernel as used so they are never handed out.
    uint32_t first_free_frame = ((uint32_t)&_kernel_end + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint32_t frame = 0; frame < first_free_frame; frame++) {
        physical_memory_bitmap[frame / 8] |= 1 << (frame % 8);
    }



    free_list_start = (free_list_node_t*)free_list_buffer;
    free_list_start->size = 4096 - sizeof(free_list_node_t);
}

uint32_t phys_alloc_frame() {
    for (int i = 0; i < NUM_FRAMES / 8; i++) {
        for (int bit = 0; bit < 8; bit++) {
            if(!(physical_memory_bitmap[i] & (1 << bit))) {
                physical_memory_bitmap[i] |= 1 << bit;
                return (8 * i + bit) * PAGE_SIZE;
            }
        }
    }
    SERIAL_PRINT("phys_alloc_frame: OUT OF FRAMES\n");
    return 0;
}

void map_page(uint32_t virt_addr, uint32_t flags) {
    uint32_t frame_addr = phys_alloc_frame();
    SERIAL_PRINT("map_page: virt="); serial_print_uint(virt_addr); serial_putc('\n');
    SERIAL_PRINT("map_page: phys="); serial_print_uint(frame_addr); serial_putc('\n');
    uint32_t page_num = virt_addr / 4096;
    uint32_t pd_offset = page_num / 1024;
    uint32_t pt_offset = page_num % 1024;
    if (pd_addr[pd_offset] & FLAG_PSE) {
        // Split a 4MiB "superpage" into 4KiB pages.
        uint32_t* new_page_table = (uint32_t*)phys_alloc_frame();
        SERIAL_PRINT("map_page: split superpage, PT phys="); serial_print_uint((uint32_t)new_page_table); serial_putc('\n');
        uint32_t superpage_start = pd_addr[pd_offset] & PAGE_ADDR_MASK;
        for (int i = 0; i < 1024; i++) {
            new_page_table[i] = i * 4096 + superpage_start;
            new_page_table[i] |= flags;
        }
        pd_addr[pd_offset] = (uint32_t)new_page_table | FLAG_PRESENT | FLAG_RW | FLAG_USER;
    }
    uint32_t* cur_page_table = (uint32_t*)(pd_addr[pd_offset] & PAGE_ADDR_MASK);
    cur_page_table[pt_offset] = frame_addr | flags;
}

void unmap_page(uint32_t virt_addr) {
    uint32_t page_num = virt_addr / PAGE_SIZE;
    uint32_t pd_offset = page_num / 1024;
    uint32_t pt_offset = page_num % 1024;
    uint32_t* cur_page_table = (uint32_t*)(pd_addr[pd_offset] & PAGE_ADDR_MASK);
    uint32_t frame_addr = cur_page_table[pt_offset] & PAGE_ADDR_MASK;
    uint32_t frame_num = frame_addr / PAGE_SIZE;
    physical_memory_bitmap[frame_num / 8] &= ~(1 << (frame_num % 8));
    cur_page_table[pt_offset] = 0;
}

void map_pages(uint32_t virt_addr, uint32_t num_pages, uint32_t flags) {
    for (uint32_t i = 0; i < num_pages; i++) {
        map_page(virt_addr, flags);
    }
}

uint32_t kmalloc(uint32_t num_bytes) {
    free_list_node_t* prev_node = NULL;
    free_list_node_t* node = free_list_start;

    while (node != NULL) {
        if (node->size >= num_bytes) {
            uint32_t remaining = node->size - num_bytes;
            if (remaining > sizeof(free_list_node_t)) {
                // Split: carve off num_bytes, leave a new free node for the rest.
                free_list_node_t* new_node = (free_list_node_t*)((uint8_t*)node + sizeof(free_list_node_t) + num_bytes);
                new_node->size = remaining - sizeof(free_list_node_t);
                new_node->next = node->next;
                if (prev_node != NULL) {
                    prev_node->next = new_node;
                } else {
                    free_list_start = new_node;
                }
            } else {
                // Not enough room to split — use the whole block.
                if (prev_node != NULL) {
                    prev_node->next = node->next;
                } else {
                    free_list_start = node->next;
                }
            }
            node->size = num_bytes;
            node->next = NULL;
            return (uint32_t)node + sizeof(free_list_node_t);
        }
        prev_node = node;
        node = node->next;
    }

    // No suitable block — map new pages.
    uint32_t total_needed = sizeof(free_list_node_t) + num_bytes;
    uint32_t num_pages = (total_needed + PAGE_SIZE - 1) / PAGE_SIZE;
    uint32_t new_mem = vram_border;
    map_pages(new_mem, num_pages, FLAG_PRESENT | FLAG_RW);
    vram_border += num_pages * PAGE_SIZE;

    // Add any leftover space at the end of the new pages to the free list.
    uint32_t leftover_size = num_pages * PAGE_SIZE - total_needed;
    if (leftover_size > sizeof(free_list_node_t)) {
        free_list_node_t* leftover = (free_list_node_t*)(new_mem + total_needed);
        leftover->size = leftover_size - sizeof(free_list_node_t);
        leftover->next = NULL;
        if (prev_node != NULL) {
            prev_node->next = leftover;
        } else {
            free_list_start = leftover;
        }
    }

    free_list_node_t* alloc_node = (free_list_node_t*)new_mem;
    alloc_node->size = num_bytes;
    alloc_node->next = NULL;
    return new_mem + sizeof(free_list_node_t);
}
