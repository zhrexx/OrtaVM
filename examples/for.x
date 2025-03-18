; Equivalent to: for (int i = 0; i < 5; i++) { print(i); }
#define $LOOPS 5

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
    push $LOOPS
    lt
    jmpif loop
    ret










