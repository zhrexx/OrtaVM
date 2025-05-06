;#include "std.x"

__entry:
    here
    push ": ERROR: Could not find bomb"
    push "%s%s"
    sprintf
    print
