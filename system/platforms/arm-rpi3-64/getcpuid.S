    .globl  getcpuid
    .type getcpuid, "function"
getcpuid:
    mrs x0, mpidr_el1
    and x0, x0, #3
    ret
