#include "std.x"

__entry:
    togglelocalscope
    var a
    push 10
    setvar a
    getvar a
    print
    togglelocalscope 
