#include "memory.h"

uint32_t* pd_addr;
uint32_t* pt_addr;
uint8_t physical_memory_bitmap[NUM_FRAMES / 8];
uint32_t vram_border = 0x400000;

uint8_t free_list_buffer[4096];
free_list_node_t* free_list_start;

void init_mem() {
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
    return 0;
}

void map_page(uint32_t virt_addr, uint32_t flags) {
    uint32_t frame_addr = phys_alloc_frame();
    uint32_t page_num = virt_addr / 4096;
    uint32_t pd_offset = page_num / 1024;
    uint32_t pt_offset = page_num % 1024;
    if (pd_addr[pd_offset] & FLAG_PSE) {
        // Split a 4MiB "superpage" into 4KiB pages.
        uint32_t* new_page_table = (uint32_t*)phys_alloc_frame();
        uint32_t superpage_start = pd_addr[pd_offset] & PAGE_ADDR_MASK;
        for (int i = 1; i < 1024; i++) {
            new_page_table[i] = i * 4096 + superpage_start;
            new_page_table[i] |= flags;
        }
        pd_addr[pd_offset] = (uint32_t)new_page_table | flags;
    }
    uint32_t* cur_page_table = (uint32_t*)(pd_addr[pd_offset] & PAGE_ADDR_MASK);
    cur_page_table[pt_offset] = frame_addr | flags;
}

void map_pages(uint32_t virt_addr, uint32_t num_pages, uint32_t flags) {
    for (int i = 0; i < num_pages; i++) {
        map_page(virt_addr, flags);
    }
}

uint32_t kmalloc(uint32_t num_bytes) {
    free_list_node_t* new_node;
    free_list_node_t* prev_node = NULL;
    free_list_node_t scratch_node;

    free_list_node_t* node = (free_list_node_t*)free_list_start;
    while (node != NULL) {
        if (node->size >= num_bytes) {
            // TODO Remove nodes when size reaches 0.
            new_node = (free_list_node_t*)((uint8_t*)node + num_bytes + sizeof(free_list_node_t));
            scratch_node = *node;
            *new_node = scratch_node;
            if (prev_node != NULL) {
                prev_node->next = new_node;
            } else {
                free_list_start = new_node;
            }
            node->size = num_bytes;
            new_node->size = scratch_node.size - num_bytes - sizeof(free_list_node_t);
            return (uint32_t)(node) + sizeof(free_list_node_t);
        }
        prev_node = node;
        node = node->next;
    }
    uint32_t total_needed_bytes = num_bytes + 2 * sizeof(free_list_node_t);
    uint32_t num_pages = total_needed_bytes / PAGE_SIZE + 1;
    uint32_t new_alloc_mem_start = vram_border;
    uint32_t new_unalloc_mem_start = vram_border + num_bytes + sizeof(free_list_node_t);
    map_pages(vram_border, FLAG_PRESENT | FLAG_RW, num_pages);
    vram_border += num_pages * PAGE_SIZE;
    free_list_node_t* new_unalloc_node = (free_list_node_t*)new_unalloc_mem_start;
    prev_node->next = new_unalloc_node;
    new_unalloc_node->size = num_pages * PAGE_SIZE - total_needed_bytes;
    new_unalloc_node->next = NULL;
    free_list_node_t* new_alloc_node = (free_list_node_t*)new_alloc_mem_start;
    new_alloc_node->size = num_bytes;
    return new_alloc_mem_start + sizeof(free_list_node_t);
}
