#/bin/sh

qemu-system-aarch64 -M raspi3b -nographic -kernel compile/xinu.elf -S -gdb tcp::1234
