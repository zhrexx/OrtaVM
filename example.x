#define $LOOPS 10000000

__entry:
    mov 0 r8
    loop:
    ; body
    print r8 ; Removed stack operations
    ; body end 
    inc r8
    push r8
    push $LOOPS
    lt
    jmpif loop
    ret




