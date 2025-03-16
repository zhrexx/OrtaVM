BITS 64

section .text
    global malloc
    global free
    global printf

malloc:
    push rbp
    mov rbp, rsp
    
    mov r8, rdi
    
    mov rax, 12
    xor rdi, rdi
    syscall
    
    mov r12, rax
    
    mov rdi, rax
    add rdi, r8
    mov rax, 12
    syscall
    
    cmp rax, r12
    je .malloc_failed
    
    mov rax, r12
    
    mov rsp, rbp
    pop rbp
    ret
    
.malloc_failed:
    xor rax, rax
    mov rsp, rbp
    pop rbp
    ret

free:
    ret

printf:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    
    push rbx
    push r12
    push r13
    push r14
    
    mov r12, rdi
    mov r13, rsi
    mov r14, rdx
    
.process_format:
    movzx rax, byte [r12]
    test rax, rax
    jz .done
    
    cmp rax, '%'
    je .format_specifier
    
    call .output_char
    inc r12
    jmp .process_format
    
.format_specifier:
    inc r12
    movzx rax, byte [r12]
    
    cmp rax, 'd'
    je .print_int
    cmp rax, 's'
    je .print_str
    
    mov rax, '%'
    call .output_char
    dec r12
    inc r12
    jmp .process_format
    
.print_int:
    mov rax, r13
    mov rcx, 10
    lea rdi, [rbp-16]
    add rdi, 15
    mov byte [rdi], 0
    
    test rax, rax
    jns .convert_int
    neg rax
    push rax
    mov rax, '-'
    call .output_char
    pop rax
    
.convert_int:
    xor rdx, rdx
    div rcx
    add rdx, '0'
    dec rdi
    mov [rdi], dl
    test rax, rax
    jnz .convert_int
    
    mov rsi, rdi
    call .print_raw_str
    
    inc r12
    jmp .process_format
    
.print_str:
    mov rsi, r13
    call .print_raw_str
    
    inc r12
    jmp .process_format
    
.print_raw_str:
    mov rbx, rsi
.str_loop:
    movzx rax, byte [rbx]
    test rax, rax
    jz .str_done
    call .output_char
    inc rbx
    jmp .str_loop
.str_done:
    ret
    
.output_char:
    push rax
    mov rdx, 1
    mov rsi, rsp
    mov rdi, 1
    mov rax, 1
    syscall
    pop rax
    ret
    
.done:
    pop r14
    pop r13
    pop r12
    pop rbx
    
    mov rsp, rbp
    pop rbp
    ret
