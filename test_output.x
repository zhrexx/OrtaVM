; __entry(0)
__entry:
    togglelocalscope
    ; BUILTIN CALL @print
    push "Hello from refactored code!"
    print
    ; END BUILTIN CALL @print
    togglelocalscope
    halt
; END __entry(0)

__error_not_enough_args_0:
    push "[%s ERROR] invalid amount of arguments"
    sprintf
    print
    halt 1
