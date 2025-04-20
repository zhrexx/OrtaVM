__entry:
    alloc charp 1 rax 
    push "Hello, World"
    @w rax 0 charp
    @r rax 0 charp
    print
    free rax
    
    alloc int 1 rax 
    @w rax 0 int 42
    @r rax 0 int
    print
    free rax
