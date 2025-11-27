nasm -f elf32 boot.asm -o boot.o
x86_64-elf-gcc -m32 -ffreestanding -c kernel.c -o kernel.o
x86_64-elf-ld -m elf_i386 -T link.ld -o kernel.elf boot.o kernel.o -nostdlib
x86_64-elf-objcopy -O binary kernel.elf kernel.bin
