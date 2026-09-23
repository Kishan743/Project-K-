BITS 32

section .text

global context_switch

;
; void context_switch(uint32_t* old_esp,
;                     uint32_t new_esp);
;
; Saves the callee-saved registers of the current task,
; stores ESP into *old_esp, loads the next task's ESP,
; restores its registers and returns into that task.
;
context_switch:

    ; Save registers belonging to the current task.
    push ebp
    push ebx
    push esi
    push edi

    ; First argument:
    ; [esp + 20] = old_esp
    mov eax, [esp + 20]
    mov [eax], esp

    ; Second argument:
    ; [esp + 24] = new_esp
    mov esp, [esp + 24]

    ; Restore next task's saved registers.
    pop edi
    pop esi
    pop ebx
    pop ebp

    ; Continue execution from the next task.
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
