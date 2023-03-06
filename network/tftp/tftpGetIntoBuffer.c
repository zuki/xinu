/**
 * @file tftpGetIntoBuffer.c
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#include <tftp.h>
#include <memory.h>
#include <string.h>

static int tftpCopyIntoBufferCb(const uchar *data, uint len, void *ctx);

/** @ingroup tftp
 * @def TFTP_FILE_DATA_BLOCK_SIZE
 * @brief リンクリストに格納するTFTPファイルデータブロックサイズ
 */
#define TFTP_FILE_DATA_BLOCK_SIZE 4096

/** @ingroup tftp
 * TFTPファイルデータブロック録構造体. リンクリストに一時的に格納する
 * ファイルデータ。
 */
struct tftpFileDataBlock
{
    ulong bytes_filled;                 /** 設定済みバイト数 */
    struct tftpFileDataBlock *next;     /**< 次のデータブロック */
    uchar data[TFTP_FILE_DATA_BLOCK_SIZE - sizeof(ulong) - sizeof(void*)];  /**< データ */
};

/**
 * @ingroup tftp
 *
 * TFTPを使ってリモートサーバからファイルをダウンロードし、
 * ファイルコンテンツを格納するインメモリバッファを割り当てて返す.
 *
 * @param[in] filename
 *      ダウンロードするファイルの名前.
 * @param[in] local_ip
 *      コネクションに使用するローカルのプロトコルアドレス.
 * @param[in] server_ip
 *      コネクションに使用するリモートのプロトコルアドレス（TFTPサーバのアドレス）.
 * @param[out] len_ret
 *      成功の場合、バイト単位のファイル長、返されるバッファの長さでもあり、この場所に
 *      書かれる.
 *
 * @return
 *      成功の場合、ダンロードしたファイルのコンテンツを含むバッファへのポインタ（
 *      @c int にキャスト）を返す。このバッファはmemget()で割り当てられるので、
 *      memfree()を使って解放することができる。メモリ不足、タイムアウト、ファイルが
 *      見つからなかったなどのエラーの場合は、::SYSTEM を返す。
 */
syscall tftpGetIntoBuffer(const char *filename, const struct netaddr *local_ip,
                          const struct netaddr *server_ip, uint *len_ret)
{
    /* 残念ながら、TFTP（拡張機能なし）ではダウンロードするファイルの最終的なサイズを
     * 取得する方法はない。そのため、ブロック単位で領域を確保し、それらをリンクリストに
     * リンクし、最後にデータを1つのバッファにコピーする。注: リンクリストに格納される
     * メモリブロックのサイズ (TFTP_FILE_DATA_BLOCK_SIZE) はTFTPブロックサイズ (拡張
     * 機能がない場合は常に512バイト) と一致する必要はない。 */
    struct tftpFileDataBlock *head, *ptr, *next, *tail;
    int retval;
    uchar *finalbuf;
    uint totallen;

    /* 呼び出し側が長さを格納する場所を求めなかった場合、返されたメモリをmemfree()で
     * 解放することができない */
    if (NULL == len_ret)
    {
        TFTP_TRACE("Length pointer not supplied.");
        return SYSERR;
    }

    /* ブロックリストの戦闘を割り当てる */
    head = memget(TFTP_FILE_DATA_BLOCK_SIZE);
    if (SYSERR == (int)head)
    {
        TFTP_TRACE("Out of memory.");
        return SYSERR;
    }
    head->bytes_filled = 0;
    head->next = NULL;
    tail = head;

    /* ファイルをダウンロードする。受信したデータからブロックリストを構築するのは
     * コールバック関数tftpCopyIntoBufferCb()の役目である */
    retval = tftpGet(filename, local_ip, server_ip, tftpCopyIntoBufferCb, &tail);

    /* 返された状態をチェックする */

    TFTP_TRACE("tftpGet() returned %d", retval);

    if (OK == retval)
    {
        /* ファイルのダウンロードに成功した。ファイルのサイズを計算し、結果として
         * 返すバッファを割り当てる */
        totallen = 0;
        ptr = head;
        do
        {
            totallen += ptr->bytes_filled;
            ptr = ptr->next;
        } while (NULL != ptr);

        TFTP_TRACE("Allocating buffer for file data (%u bytes).", totallen);
        finalbuf = memget(totallen);
        if (SYSERR == (int)finalbuf)
        {
            TFTP_TRACE("Out of memory.");
        }
    }
    else
    {
        /* ファイルのダウンロードに失敗した。SYSERRを返すが、その前にブロックリストを
         * 解放する必要がある */
        TFTP_TRACE("File download failed.");
        finalbuf = (uchar*)SYSERR;
    }

    /* ブロックリストを解放する。ダウンロードに成功した場合は、同時に、ファイルデータを
     * バッファにコピーする */
    TFTP_TRACE("Freeing block list and copying data into final buffer.");
    totallen = 0;
    next = head;
    do
    {
        ptr = next;
        next = ptr->next;
        if (SYSERR != (int)finalbuf)
        {
            memcpy(&finalbuf[totallen], ptr->data, ptr->bytes_filled);
        }
        totallen += ptr->bytes_filled;
        memfree(ptr, TFTP_FILE_DATA_BLOCK_SIZE);
    } while (NULL != next);

    /* 成功の場合、ファイル長を呼び出し側が指定した場所に保存する */
    if (SYSERR != (int)finalbuf)
    {
        *len_ret = totallen;
        TFTP_TRACE("TFTP download into buffer successful "
                   "(address=0x%08x, length=%u)", finalbuf, totallen);
    }

    /* ファイルデータを含むバッファ、あるいはSYSERRを返す */
    return (int)finalbuf;
}

/*
 * TFTPデータブロックを渡されるコールバック関数でtftpGet() に与えられる。
 * この実装ではTFTPデータをメモリに保存する（ブロックリストについては
 * このファイルの前のほうで説明されている）。
 *
 * 成功すればOK、そうでなければSYSERRを返すことが期待されている。
 */
static int tftpCopyIntoBufferCb(const uchar *data, uint len, void *ctx)
{
    struct tftpFileDataBlock **tailptr = ctx;
    struct tftpFileDataBlock *tail = *tailptr;

    while (0 != len)
    {
        struct tftpFileDataBlock *newtail;
        uint copylen;

        if (tail->bytes_filled == sizeof(tail->data))
        {
            /* ブロックは満杯: 新しいブロックを追加する */
            newtail = memget(TFTP_FILE_DATA_BLOCK_SIZE);
            if (SYSERR == (int)newtail)
            {
                TFTP_TRACE("Out of memory.");
                return SYSERR;
            }
            newtail->bytes_filled = 0;
            newtail->next = NULL;
            tail->next = newtail;
            *tailptr = tail = newtail;
        }

        /* できるだけ多くのデータを格納する */
        if (len > sizeof(tail->data) - tail->bytes_filled)
        {
            copylen = sizeof(tail->data) - tail->bytes_filled;
        }
        else
        {
            copylen = len;
        }
        memcpy(&tail->data[tail->bytes_filled], data, copylen);
        tail->bytes_filled += copylen;
        len -= copylen;
        data += copylen;
    }
    return OK;
}
