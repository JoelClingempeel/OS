/* link.ld - Linker script for the second stage bootloader/kernel (FIXED) */

OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
ENTRY(stage2_entry) /* The entry point in your assembly code */

SECTIONS {
    /* The entire binary is loaded to physical address 0x7C00 by the BIOS/bootloader */
    . = 0x7C00; 

    /* Stage 1 is the first 512 bytes */
    .stage1 : {
        *(.text.stage1)  /* Assuming you put the stage 1 code in a specific section */
        *(.text)         /* Catches the default section if you didn't rename it */
    } /* Removed the "> 0x7C00" which caused the error */

    /* 
     * The rest of the binary (stage 2 and C code) is loaded at 0x7E00.
     * We skip the 512 bytes difference.
     */
    . = 0x7E00; 
    
    /* We don't need an explicit section here because the .text, .data, .bss 
       sections below will start writing at this current location counter address (0x7E00). */

    .data 0x100000 : {           /* Initialized data */
        *(.data)
    }

    .rodata 0x102000 : {         /* Read-only data (like your "Hello World" string) */
        *(.rodata)
    }

    .bss 0x104000 : {            /* Uninitialized data (PT and PD) */
        *(.bss)
    }

    /* 
     * This is crucial: Ensure the output binary size matches what BIOS reads (51200 bytes total)
     * We align to a total size of 51200 bytes (50KB) as defined in your assembly RESB padding
     */
    . = 0x7C00 + 51200;
    /DISCARD/ : { *(.note.GNU-stack) } /* Optional: discard standard junk we don't need */
}
