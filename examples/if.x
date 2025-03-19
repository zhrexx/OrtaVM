#define i 10
#define to_compare 10
__entry:

    push i
    push to_compare
    eq 
    jmpif if

    else:
        push "i != 10"
        print
        jmp end_if

    if:
        push "i == 10"
        print
end_if:
    ret
