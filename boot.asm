bits 16
SECTION .text.stage1
stage1_start:
    mov ah, 0x02       ; BIOS read disk function
    mov al, 0x63       ; Read 99 sectors.
    mov cl, 0x02       ; Start at sector 2.
    mov ch, 0x00       ; Cylinder 0
    mov dh, 0x00       ; Head 0

    xor bx, bx
    mov es, bx
    mov bx, 0x7e00
    
    int 0x13
    
    jmp 0x0000:stage2_entry

times 510-($-stage1_start) db 0
dw 0xAA55

SECTION .text
global stage2_entry
stage2_entry:
CODE_SEG equ GDT_code - GDT_start
DATA_SEG equ GDT_data - GDT_start                         

cli
lgdt [GDT_descriptor]
mov eax, 1
mov cr0, eax
jmp CODE_SEG:start_protected_mode

                                   
GDT_start:
    GDT_null:
        dd 0x0
        dd 0x0

    GDT_code:
        dw 0xffff
        dw 0x0
        db 0x0
        db 0b10011010
        db 0b11001111
        db 0x0

    GDT_data:
        dw 0xffff
        dw 0x0
        db 0x0
        db 0b10010010
        db 0b11001111
        db 0x0

GDT_end:

GDT_descriptor:
    dw GDT_end - GDT_start - 1
    dd GDT_start


bits 32
extern _kmain
start_protected_mode:
    call get_position
get_position:
    pop ebx
    sub ebx, get_position
    add ebx, 0x7E00

    ; Fill in one PD entry.
    mov eax, (pt - stage2_entry)
    add eax, ebx
    or eax, 3
    mov edi, (pd - stage2_entry)
    add edi, ebx
    mov [edi], eax

    ; Fill in each PT entry.
    mov eax, 3  ; PT data
    mov ecx, 0x1000  ; Counter
    mov edi, (pt - stage2_entry)  ; Target (PT entry)
    add edi, ebx
pt_fill_loop:
    mov [edi], eax
    add eax, 0x1000
    add edi, 4
    dec ecx
    cmp ecx, 0
    jg pt_fill_loop

    ; Enable paging by loading pd into cr3 and flipping bit 31 in cr0
    mov eax, (pd - stage2_entry)
    add eax, ebx
    mov cr3, eax
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
        
    jmp 0x08:paged_mode_start
paged_mode_start:
    ; mov esp, 0x7000  ; temp_stack_storage + 4096
    ; mov esp, STACK_TOP
    mov esp, 0x40000
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    call _kmain

extern _idt80
global handle_interrupt
handle_interrupt:
    pushad
    call _idt80
    popad
    iretd

extern _idt_timer
global handle_timer_int
handle_timer_int:
    pushad
    call _idt_timer
    popad
    iretd

extern _idt_double_fault
global handle_double_fault
handle_double_fault:
    pushad
    push dword [esp + 32]
    call _idt_double_fault
    add esp, 4
    popad
    add esp, 4
    iretd

extern _idt_gpf
global handle_gpf
handle_gpf:
    pushad
    push dword [esp + 32]
    call _idt_gpf
    add esp, 4
    popad
    add esp, 4
    iretd

extern _idt_page_fault
global handle_page_fault
handle_page_fault:
    pushad
    push dword [esp + 32]
    call _idt_page_fault
    add esp, 4
    popad
    add esp, 4
    iretd

SECTION .bss
ALIGN 4096
pd:
    RESB 4096
ALIGN 4096
pt:
    RESB 4096
    

; Add the stack buffer after everything else
KERNEL_STACK_SIZE equ 16384 
KERNEL_STACK:
    RESB KERNEL_STACK_SIZE 
STACK_TOP: ; Use this label for 'mov esp, STACK_TOP'