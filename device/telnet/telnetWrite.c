/**
 * @file     telnetWrite.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <ctype.h>
#include <device.h>
#include <telnet.h>
#include <thread.h>
#include <stdio.h>

/**
 * @ingroup telnet
 *
 * telnetクライアントにバッファを書き出す.
 * @param devptr TELNETデバイステーブルエントリ
 * @param buf 出力する文字のバッファ
 * @param len バッファのサイズ
 * @return 出力した文字数
 */
devcall telnetWrite(device *devptr, void *buf, uint len)
{
    struct telnet *tntptr;
    device *phw;
    uchar ch = 0;
    uint count = 0;
    uchar *buffer = buf;

    /* 設定と構造体へのポインタのエラーチェック */
    tntptr = &telnettab[devptr->minor];
    phw = tntptr->phw;
    if (NULL == phw)
    {
        return SYSERR;
    }

    wait(tntptr->osem);

    /* すべての文字を正しいフォーマットでバッファに書き指す */
    while (count < len)
    {
        ch = buffer[count++];

        /* 2文字以上入らない場合、バッファを下位デバイスに書き込む */
        if (tntptr->ostart >= TELNET_OBLEN - 1)
        {
            if (SYSERR == telnetFlush(devptr))
                return SYSERR;
        }

        switch (ch)
        {
            /* CRLFをバッファに追加してバッファを書き出す */
        case '\n':
            tntptr->out[tntptr->ostart++] = '\r';
            tntptr->out[tntptr->ostart++] = '\n';

            if (SYSERR == telnetFlush(devptr))
            {
                return SYSERR;
            }
            break;
            /* IAC文字をエスケープする */
        case TELNET_IAC:
            tntptr->out[tntptr->ostart++] = ch;
            tntptr->out[tntptr->ostart++] = ch;
            break;
            /* 普通に書き出す */
        default:
            tntptr->out[tntptr->ostart++] = ch;
            break;
        }
    }

    signal(tntptr->osem);

    return count;
}
