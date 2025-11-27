void _kmain(void)
{
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
    while (1) {
        asm("hlt"); 
    }
}
