CC = x86_64-elf-gcc
LD = x86_64-elf-ld
NASM = nasm
OBJCOPY = x86_64-elf-objcopy

TARGET_ELF = kernel.elf
TARGET_BIN = kernel.bin
ASM_SRC = boot.asm
C_SRCS = kernel.c interrupts.c memory.c utils.c syscalls.c user.c scheduler.c

OBJ_FILES = $(ASM_SRC:.asm=.o) $(C_SRCS:.c=.o)

CFLAGS = -m32 -ffreestanding -c -Wall -Wextra -nostdlib -mno-sse -mno-sse2

LDFLAGS = -m elf_i386 -T link.ld -nostdlib

.PHONY: all clean

all: $(TARGET_BIN)

$(TARGET_BIN): $(TARGET_ELF)
	$(OBJCOPY) -O binary $< $@
	@echo "--- Build successful: $@ created ---"

$(TARGET_ELF): $(OBJ_FILES)
	$(LD) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

%.o: %.asm
	$(NASM) -f elf32 $< -o $@

clean:
	rm -f $(OBJ_FILES) $(TARGET_ELF) $(TARGET_BIN)
