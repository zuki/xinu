.globl pause
pause:
    wfi               
    ret

/* EVentルーチンを送信する。Cコードで使用できるようにする (unparkcore.c) */
.globl sev
sev:
    sev
    ret
