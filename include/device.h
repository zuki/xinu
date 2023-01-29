/**
 * @file device.h
 *
 * Contains all definitions relating to the Xinu device subsystem.
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

/* Device table declarations */
#ifndef _DEVICE_H_
#define _DEVICE_H_

#include <conf.h>
#include <stddef.h>

/**
 * @ingroup devcalls
 *
 * デバイスIDが0とNDEVSの間にあるかチェックする
 *
 * @param f テストするID番号
 */
#define isbaddev(f)  ( !(0 <= (f) && (f) < NDEVS) )

/* 標準デバイス関数 */
devcall open(int, ...);
devcall close(int);
devcall read(int, void *, uint);
devcall write(int, const void *, uint);
devcall getc(int);
devcall putc(int, char);
devcall seek(int, uint);
devcall control(int, int, long, long);
syscall getdev(const char *);

#endif                          /* _DEVICE_H_ */
