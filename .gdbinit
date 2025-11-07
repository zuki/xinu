set architecture aarch64
file compile/xinu.elf
target remote localhost:1234
set print pretty on
set logging enable off
set height 0

#break resched.c:75
break ctxsw
#break nulluser
