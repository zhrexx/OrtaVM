#include "src/orta.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ExampleExtern(OrtaVM *vm)
{
    xstack_push(&vm->xpu.stack, (Word){.as_int = 100, .type = WINT});
}

