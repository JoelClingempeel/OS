#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

#include "utils.h"

#define PAGE_SIZE 4096
#define MAX_PHYS_MEM 41943040
#define NUM_FRAMES (MAX_PHYS_MEM / PAGE_SIZE)
#define PAGE_ADDR_MASK 0xFFFFF000

#define FLAG_PRESENT 0x1
#define FLAG_RW      0x2
#define FLAG_PSE     0x80


extern uint32_t* pd_addr;
extern uint32_t* pt_addr;
extern uint8_t physical_memory_bitmap[NUM_FRAMES / 8];
extern uint32_t vram_border;

typedef struct free_list_node {
    size_t size;
    struct free_list_node *next;
} free_list_node_t;
extern uint8_t free_list_buffer[4096];
extern free_list_node_t* free_list_start;

// Initialize memory management data structures.
void init_mem();

// Allocate a new frame in physical memory, and return the address.
uint32_t phys_alloc_frame();

// Map a page at a given virtual address.
void map_page(uint32_t virt_addr, uint32_t flags);

// Map a specified number of pages at a given virtual address.
void map_pages(uint32_t virt_addr, uint32_t num_pages, uint32_t flags);

// Allocate a specified number of bytes.
uint32_t kmalloc(uint32_t num_bytes);

#endif  // MEMORY_H
