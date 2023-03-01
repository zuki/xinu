.globl  getcpuid
.func   getcpuid
getcpuid:
    mrc p15, 0, r0, c0, c0, 5   // MPIDRレジスタにアクセスしてROに保管
    and r0, r0, #7              // [0:2]ビットだけを返す
    bx  lr
    .endfunc
