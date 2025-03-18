; Equivalent to: for (int i = 0; i < 5; i++) { print(i); }

__entry:
    mov 0 r8
    loop:
    ; body
    push r8
    print
    drop
    ; body end 
    inc r8
    push r8
    push 5
    lt
    jmpif loop
    ret










