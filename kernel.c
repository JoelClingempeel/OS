#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096
#define MAX_PHYS_MEM 41943040
#define NUM_FRAMES (MAX_PHYS_MEM / PAGE_SIZE)
#define PAGE_ADDR_MASK 0xFFFFF000

#define FLAG_PRESENT 0x1
#define FLAG_RW      0x2
#define FLAG_PSE     0x80

uint32_t* pd_addr;
uint32_t* pt_addr;
uint8_t physical_memory_bitmap[NUM_FRAMES / 8];

typedef struct free_list_node {
    size_t size;
    struct free_list_node *next;
} free_list_node_t;
uint8_t free_list_buffer[4096];
free_list_node_t* free_list_start;

uint8_t next_char = 0;

uint32_t phys_alloc_frame(){
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

void* memcpy(void* dest, const void* src, size_t count) {
    char* dst8 = (char*)dest;
    const char* src8 = (const char*)src;
    while (count--) {
        *dst8++ = *src8++;
    }
    return dest;
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

uint32_t kmalloc(uint32_t num_bytes){
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
    // TODO Allocate pages when needed.
}

void printk(char* string, uint8_t format) {
    char *video_memory = (char *)0xb8000;
    int i = 0;
    while (string[i] != 0) {
        video_memory[2 * next_char] = string[i];
        video_memory[2 * next_char + 1] = format;
        i++;
        next_char++;
    }
}

void _kmain(void)
{
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

    while (1) {
        asm("hlt"); 
    }
}
