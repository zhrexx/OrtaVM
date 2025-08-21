#include "src/orta.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#include <io.h>
#define F_OK 0
#define access _access
#else
#include <unistd.h>
#endif


void ExampleExtern(OrtaVM *vm)
{
    xstack_push(&vm->xpu.stack, (Word){.as_int = 100, .type = WINT});
}

void IsFile(OrtaVM *vm)
{


}



