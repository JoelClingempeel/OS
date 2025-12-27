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

global GDT_start
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
    
    GDT_user_code:   ; 0x18 - User Code (Ring 3)
        dw 0xffff
        dw 0x0
        db 0x0
        db 0b11111010 ; Access: P=1, DPL=11 (User), S=1, Type=Code (0xFA)
        db 0b11001111
        db 0x0

    GDT_user_data:   ; 0x20 - User Data (Ring 3)
        dw 0xffff
        dw 0x0
        db 0x0
        db 0b11110010 ; Access: P=1, DPL=11 (User), S=1, Type=Data (0xF2)
        db 0b11001111
        db 0x0

    GDT_tss:         ; 0x28 - Task State Segment (Ring 0)
        dw 103       ; Limit (sizeof TSS - 1)
        dw 0x0       ; Base 0:15 (To be patched in C)
        db 0x0       ; Base 16:23 (To be patched in C)
        db 0b10001001 ; Access: P=1, DPL=00, S=0, Type=32-bit TSS (0x89)
        db 0b01000000 ; Flags: G=0, D=1 (32-bit)
        db 0x0       ; Base 24:31 (To be patched in C)

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
    or eax, 7
    mov edi, (pd - stage2_entry)
    add edi, ebx
    mov [edi], eax

    ; Fill in each PT entry.
    mov eax, 7  ; PT data
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
    push esp
    call _idt80
    add esp, 4
    popad
    iretd

extern _idt_timer
extern schedule
extern current_task_ptr
global handle_timer_int
extern update_tss_esp0
handle_timer_int:
    ; --- PART 1: SAVE OLD TASK ---
    pushad 
    push ds
    push es
    push fs
    push gs

    ; Save the current stack pointer into the current_task_struct
    ; current_task_ptr is the C variable we talked about
    mov eax, [current_task_ptr] 
    mov [eax], esp              ; Assumes 'esp' is the first field in the struct

    ; --- PART 2: SWITCH ---
    mov ax, 0x10                ; Kernel Data Segment
    mov ds, ax
    mov es, ax

    call _idt_timer
    call schedule               ; C function updates current_task_ptr

    ; --- PART 3: LOAD NEW TASK ---
    mov eax, [current_task_ptr]
    mov esp, [eax]              ; CPU ESP now points to the NEW task's stack!

    ; Update TSS esp0 so the NEXT interrupt comes back to this new kstack
    ; [eax + 4] is the 'kstack_top' field in your C struct
    push dword [eax + 4]        
    call update_tss_esp0
    add esp, 4

    pop gs
    pop fs
    pop es
    pop ds
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