/**
 * @file colon2mac.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <device.h>
#include <ether.h>

#include <ctype.h>

/**
 * @ingroup lan7800
 *
 * コロン区切りの文字列表記のMACをバイト配列に変換する
 * @param src コロン区切りのMAC文字列へのポインタ
 * @param dst バイト配列へのポインタ
 * @return 変換したオクテットの数
 */
int colon2mac(char *src, unsigned char *dst)
{
    uchar count = 0, digit = 0, c = 0;

    if (NULL == src || NULL == dst)
    {
        return SYSERR;
    }

    while ((count < ETH_ADDR_LEN) && ('\0' != *src))
    {
        // bit [7:4]の変換
        c = *src++;
        if (isdigit(c))
        {
            digit = c - '0';
        }
        else if (isxdigit(c))
        {
            digit = 10 + c - (isupper(c) ? 'A' : 'a');
        }
        else
        {
            digit = 0;
        }
        dst[count] = digit * 16;

        // bit [3:0]の変換
        c = *src++;
        if (isdigit(c))
        {
            digit = c - '0';
        }
        else if (isxdigit(c))
        {
            digit = 10 + c - (isupper(c) ? 'A' : 'a');
        }
        else
        {
            digit = 0;
        }
        dst[count] += digit;

        count++;
        if (':' != *src++)
        {
            break;
        }
    }

    return count;
}
