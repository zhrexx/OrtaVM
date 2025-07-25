
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

; Reserve n bytes
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

; Call external function from library
; Args per stack: <lib_path> <function name>
; Or via call syntax: call callExtern <lib_path> <function name>
callExtern:
    mov 3 rax
    pop rbx
    pop rcx
    xcall
    ret

; Check if running on windows
isWindows:
    ovm platform
    push "windows"
    eq
    ret

; Check if running on windows
isUnix:
    ovm platform
    push "unix"
    eq
    ret

; for crossplatform usage
; expects 2 libs 1 linux 2 windows
chooseLib:
    togglelocalscope
    setvar __libso
    setvar __libdll

    call isUnix
    jmpif chooseLib_if

    chooseLib_else:
        getvar __libdll
        jmp chooseLib_end_if
    chooseLib_if:
        getvar __libso

    chooseLib_end_if:
    togglelocalscope
    ret

