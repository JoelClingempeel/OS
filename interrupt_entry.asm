bits 32
SECTION .text

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