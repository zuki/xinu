#include <arm.h>

.globl setmode
setmode:
    .func setmode


    cpsid if, #ARM_MODE_SYS
    bx    lr

    .endfunc
