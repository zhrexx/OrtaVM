__entry:
    alloc charp 1 rax 
    push "Hello, World"
    @w rax 0 charp
    @r rax 0 charp
    print
    free rax
    
    alloc int 1 rax
    push 42
    @w rax 0 int 
    @r rax 0 int
    print
    free rax

    alloc char 4 rax  
    push 22 
    @w rax 0 int 
    @r rax 0 int 
    print
    free rax
