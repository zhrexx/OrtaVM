
        ; Wait seconds
        ; rbx = seconds
wait:
    mov 1 rax   ; xcall opcode
    push 1000 
    push rbx 
    mul  ; Convert to xsleep second     
    pop rbx
    xcall
    ret
        
        ; Simple system command
        ; rbx = command
_system:
    mov 2 rax ; xcall cmd opcode 
    xcall
    ret

        ; cast latest stack element to different type
        ; rbx = type as str
cast:
    mov 3 rax 
    xcall
    ret

;
; REGISTERS
;

        ; Push all regs to stack
pusha: 
    push rax 
    push rbx 
    push rcx 
    push rdx 
    push rsi 
    push rdi 
    push r8 
    push r9 
    ret 

popa:
    pop rax 
    pop rbx 
    pop rcx 
    pop rdx 
    pop rsi 
    pop rdi 
    pop r8
    pop r9
    ret

        ; Clear Registers
cregs:
    mov 0 rax 
    mov 0 rbx
    mov 0 rcx 
    mov 0 rdx 
    mov 0 rsi 
    mov 0 rdi
    mov 0 r8 
    mov 0 r9
    ret

; Reserver n bytes
; rax = n
; returns rbx a string with n bytes (char sequence and char is 1 byte)
rb: 
    mov 0 r8
    mov "" rbx 
    rb_loop:
    ; body
    push rbx 
    push " "
    merge 
    pop rbx
    ; body end 
    inc r8
    push r8
    push rax
    lt
    jmpif rb_loop
    ret







