#include <stdio.h>
#include <stdint.h>

// ifc prefix means interface

void ifc_orta_print_int(int num) {
    printf("%d\n", num);
}

void ifc_orta_input() {
    int buffer[256];
    scanf("%d", buffer);
    asm("movq %0, %%rdi"
        :
        : "r" (buffer)
        : "%rdi"
    );
}