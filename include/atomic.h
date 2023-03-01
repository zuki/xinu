/**
 * @file atomic.h
 *
 * アトミックなARMアセンブリ関数の関数プロトタイプを含んでいる.
 *
 */
/* Embedded Xinu, Copyright (C) 2019. All rights reserved. */

#ifndef _ATOMIC_H_
#define _ATOMIC_H_

/**
 * @fn int _atomic_increment(int *var)
 *
 * 指定した整数をアトミックに増分する.
 * (Cの++varに相当する).
 *
 * @param var 変数へのポインタ
 * @return 増分後に変数が格納している値
 */
int _atomic_increment(int *var);

/**
 * @fn int _atomic_decrement(int *var)
 *
 * 指定した整数をアトミックに減分する.
 * (Cの--varに相当する).
 *
 * @param var 変数へのポインタ
 * @return 減分後に変数が格納している値
 *
 */
int _atomic_decrement(int *var);

/**
 * @fn int _atomic_increment_mod(int *var, int mod)
 *
 * アトミックに増分した後、指定した値でmodする.
 * var = (var + 1) % mod に相当する.
 *
 * @param var 変数へのポインタ
 * @param mod varがこれ以上になったらラップアラウンドするための値.
 * @return 増分したvarの値
 */
int _atomic_increment_mod(int *var, int mod);


#endif                          /* _ATOMIC_H_ */
