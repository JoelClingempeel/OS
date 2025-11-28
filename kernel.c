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

    char hello[] = "Hello world!";
    char *video_memory = (char *)0xb8000;
    int i = 0;
    while (hello[i] != 0) {
        video_memory[2*i] = hello[i];
        video_memory[2*i+1] = 0x07;
        i++;
    }
    while (i < 80) {
        video_memory[2*i] = 0;
        i++;
    }

    char *byte = (char *)(1<<22 - 1);
    byte[0] = 'X';
    while (1) {
        asm("hlt"); 
    }
}
