/**
 * @file tftpGet.c
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#include <clock.h>
#include <device.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <tftp.h>
#include <thread.h>
#include <udp.h>
#include <core.h>

/** @ingroup tftp
 * @def TFTP_DROP_PACKET_PERCENT
 * @brief ストレステスト---有効な受信データパケットのこのパーセントを
 * ランダムに無視する。 */
#define TFTP_DROP_PACKET_PERCENT 0

/**
 * @ingroup tftp
 *
 * TFTPを使ってリモートサーバからファイルをダウンロードし、その内容を
 * ブロック単位でコールバック関数に渡す。このコールバック関数は、
 * ファイルデータをバッファに格納したり、永続的なストレージに書き込んだりと、
 * 好きなように操作することができる。
 *
 * @param[in] filename
 *      ダウンロードするファイルの名前
 * @param[in] local_ip
 *      接続に使用するローカルプロトコルアドレス
 * @param[in] server_ip
 *      接続に使用するリモートプロトコルアドレス（TFTPサーバのアドレス）
 * @param[in] recvDataFunc
 *      ファイルデータがブロック単位で渡されるコールバック関数。コールバック関数が
 *      呼ばれる際には、@p data （第1引数）には次のデータブロックへのポインタが、
 *      @p len （第2引数）にはブロック長がセットされる。すべてのデータブロックは
 *      同じサイズであるが、最後のデータブロックは0バイトから前のデータブロック
 *      サイズ（もしあれば）までの間のサイズになる。<br/>現在の実装では、ブロック
 *      サイズ（最後のブロックを除く）は512バイトに固定されている。ただし、TFTP
 *      ブロックサイズネゴシエーションに対応するようにtftpGet()を拡張することが
 *      できるようにこのコールバックの実装はより大きなブロックサイズを処理できる
 *      ようにするべきである。<br/>このコールバックは成功した場合、:OKを返すことが
 *      期待される。OKを返さない場合は、TFTP転送は中止され、tftpGet()はこの値を返す。
 * @param[in] recvDataCtx
 *      @p recvDataFunc にそのまま渡される追加パラメータ
 *
 * @return
 *      成功した場合は ::OK; TFTP転送がタイムアウトまたは失敗、またはエラーが発生
 *      した場合は ::SYSERR; または、@p recvDataFunc により返された ::OK 以外の値
 */
syscall tftpGet(const char *filename, const struct netaddr *local_ip,
                const struct netaddr *server_ip, tftpRecvDataFunc recvDataFunc,
                void *recvDataCtx)
{
    int udpdev;
    int udpdev2;
    int send_udpdev;
    int recv_udpdev;
    int retval;
    tid_typ recv_tid;
    uint num_rreqs_sent;
    uint block_recv_tries;
    uint next_block_number;
    ushort localpt;
    uint block_max_end_time = 0;  /* この値は使用されないがgccが発見できないので  */
    uint block_attempt_time;
    struct tftpPkt pkt;

    /* 引数のエラーチェック */
    if (NULL == filename || NULL == local_ip ||
        NULL == server_ip || NULL == recvDataFunc)
    {
        TFTP_TRACE("Invalid parameter.");
        return SYSERR;
    }

#ifdef ENABLE_TFTP_TRACE
    {
        char str_local_ip[20];
        char str_server_ip[20];
        netaddrsprintf(str_local_ip, local_ip);
        netaddrsprintf(str_server_ip, server_ip);
        TFTP_TRACE("Downloading %s from %s (using local_ip=%s)",
                   filename, str_server_ip, str_local_ip);
    }
#endif

    /* TFTPサーバーと通信するためのUDPデバイス（ソケット）を割り当てて
     * オープンする。ローカルとリモートのプロトコルアドレスは、この関数の
     * 必須パラメータとして指定される。RFC1350のクライアントのTFTP転送
     * 識別子（TID）に対応するローカルポートはランダムに割り当てる必要が
     * ある。ローカルポートとして0が渡された場合、UDPこd−がこれを処理する。
     * リモートポートは最初はTFTPのウェルノウンポート（69）であるが、最初の
     * データパケットを受信した後、サーバーが実際に応答したポートに変更する
     * 必要がある。
     *
     * しかし...サーバーが異なるポートで応答するという最後の点（これは避けられ
     * ない。TFTPがそのように設計されているので）は事態を著しく複雑にしている。
     * UDPコードはサーバの応答を最初のUDPデバイスにルーティング*しない*からである。
     * このデバイスはポート69にバインドされ、サーバが実際に応答したポートではない。
     * UDPヘッダをマニュアルで処理することなくこの問題を回避するために*2番目の*
     * UDPデバイスを作成している。これは最初はクライアントが最初のRRQを送信した
     * ポートをリッスンするが、当初はどのリモートポートやアドレスにバインド
     * *されない*。その後、この2番目のUDPデバイスにUDP_FLAG_BINDFIRSTをセットする
     * ことでサーバからの応答を受信した際にリモートポートとアドレスが自動的に
     * セットされるようにしている。サーバから送信された追加のパケットはこの2番目の
     * UDPデバイスで受信され、クライアントから送信されたパケットは1番目のUDP
     * デバイスではなく、この2番目のUDPデバイスで送信される。なぜなら、2番目の
     * UDPデバイスにリモートポートが正しく設定されているからである。
     */

    udpdev = udpAlloc();

    if (SYSERR == udpdev)
    {
        TFTP_TRACE("Failed to allocate first UDP device.");
        return SYSERR;
    }

    if (SYSERR == open(udpdev, local_ip, server_ip, 0, UDP_PORT_TFTP))
    {
        TFTP_TRACE("Failed to open first UDP device.");
        udptab[udpdev - UDP0].state = UDP_FREE;
        return SYSERR;
    }

    localpt = udptab[udpdev - UDP0].localpt;

    udpdev2 = udpAlloc();
    if (SYSERR == udpdev2)
    {
        TFTP_TRACE("Failed to allocate second UDP device.");
        retval = SYSERR;
        goto out_close_udpdev;
    }

    if (SYSERR == open(udpdev2, local_ip, NULL, localpt, 0))
    {
        TFTP_TRACE("Failed to open second UDP device.");
        retval = SYSERR;
        udptab[udpdev2 - UDP0].state = UDP_FREE;
        goto out_close_udpdev;
    }

    send_udpdev = udpdev;
    recv_udpdev = udpdev2;

    /* See lengthy comment above for explanation of this flag.  */
    control(recv_udpdev, UDP_CTRL_SETFLAG, UDP_FLAG_BINDFIRST, 0);

    TFTP_TRACE("Using UDP%d (for initial send) "
               "and UDP%d (for binding reply), client port %u",
               send_udpdev - UDP0, recv_udpdev - UDP0, localpt);

    /* 受信スレッドを作成する。これは、現在実行中のスレッドがUDPデバイス上で
     * read()を呼び出すと無限にブロックされる可能性があるため、それを避ける
     * ための回避策である */
    recv_tid = create(tftpRecvPackets, TFTP_RECV_THR_STK,
                      TFTP_RECV_THR_PRIO, "tftpRecvPackets", 3,
                      recv_udpdev, &pkt, gettid());
    if (isbadtid(recv_tid))
    {
        TFTP_TRACE("Failed to create TFTP receive thread.");
        retval = SYSERR;
        goto out_close_udpdev2;
    }
    ready(recv_tid, RESCHED_NO, CORE_ZERO);

    /* ファイルを要求することでダウンロードを開始する */
    retval = tftpSendRRQ(send_udpdev, filename);
    if (SYSERR == retval)
    {
        retval = SYSERR;
        goto out_kill_recv_thread;
    }
    num_rreqs_sent = 1;
    next_block_number = 1;

    /* ファイルが完全にダウンロードされるか、エラー状態が発生するまでループする。
     * 基本的な考え方は、クライアントはDATAパケットを1つずつ受け取るが、それぞれが
     * ファイルデータの次のブロックに対応し、サーバーが次のパケットを送信する前に
     * クライアントはそれぞれのパケットにACKすることである。ただし、以下の実際の
     * コードは、タイムアウト、再試行、無効なパケットなどを処理しなければならない
     * ので、もう少し複雑である。
     */
    block_recv_tries = 0;
    for (;;)
    {
        ushort opcode;
        ushort recv_block_number;
        struct netaddr *remote_address;
        bool wrong_source;
        ushort block_nbytes;

        /* Handle bookkeeping for timing out.  */

        block_attempt_time = clktime;
        if (block_recv_tries == 0)
        {
            uint timeout_secs;

            if (next_block_number == 1)
            {
                timeout_secs = TFTP_INIT_BLOCK_TIMEOUT;
            }
            else
            {
                timeout_secs = TFTP_BLOCK_TIMEOUT;
            }
            block_max_end_time = block_attempt_time + timeout_secs;
        }

        if (block_attempt_time <= block_max_end_time)
        {
            /* 適当なタイムアウトを使ってブロックの受信を試みる。
             * 実際の受信はtftpRecvPacket(s)を実行している別の
             * スレッドで行われる */
            TFTP_TRACE("Waiting for block %u", next_block_number);
            block_recv_tries++;
            send(recv_tid, 0);
            retval = recvtime(1000 * (block_max_end_time -
                                      block_attempt_time) + 500);
        }
        else
        {
            /* タイムアウト */
            retval = TIMEOUT;
        }

        /* タイムアウトを処理する */
        if (TIMEOUT == retval)
        {
            TFTP_TRACE("Receive timed out.");

            /* クライアントがサーバからの最初の応答を依然として待っている場合、
             * 最初のタイムアウトでは失敗せず、クライアントがRRQを数回再送する
             * 機会を持つまで待機する */
            if (next_block_number == 1 &&
                num_rreqs_sent < TFTP_INIT_BLOCK_MAX_RETRIES)
            {
                TFTP_TRACE("Trying RRQ again (try %u of %u)",
                           num_rreqs_sent + 1, TFTP_INIT_BLOCK_MAX_RETRIES);
                retval = tftpSendRRQ(send_udpdev, filename);
                if (SYSERR == retval)
                {
                    break;
                }
                block_recv_tries = 0;
                num_rreqs_sent++;
                continue;
            }

            /* 実際にタイムアウトとする。クリーンアップして失敗ステータスを返す */
            retval = SYSERR;
            break;
        }

        /* 何らかの理由でパケットを正常に受信できなかった場合、失敗
         * ステータスを返す */
        if (SYSERR == retval)
        {
            TFTP_TRACE("UDP device or message passing error; aborting.");
            break;
        }

        /* そうでなければ'retval'は受信したTFTPパケットの長さである */

        /* 受信したパケットから情報を抽出し、検証を開始する。探すものは正しい
         * IPアドレスからの正しいTFTPデータパケットであり、そのブロック番号は
         * 次のブロック番号か前のブロック番号のいずれかである。しかし、一番最初の
         * ブロックにはとっく別な処理が必要である。特に、この場合、前のブロックは
         * 存在せず、リモートネットワークアドレスをチェックして、実際にソケットが期待
         * 通りにサーバーのネットワークアドレスにバインドされていることを確認
         * する必要がある。
         */
        remote_address = &udptab[recv_udpdev - UDP0].remoteip;
        opcode = net2hs(pkt.opcode);
        recv_block_number = net2hs(pkt.DATA.block_number);
        wrong_source = !netaddrequal(server_ip, remote_address);

        if (wrong_source || retval < 4 || TFTP_OPCODE_DATA != opcode ||
            (recv_block_number != (ushort)next_block_number &&
             (next_block_number == 1 ||
              recv_block_number != (ushort)next_block_number - 1)))
        {
            /* Check for TFTP ERROR packet  */
            if (!wrong_source && (retval >= 2 && TFTP_OPCODE_ERROR == opcode))
            {
                TFTP_TRACE("Received TFTP ERROR opcode packet; aborting.");
                retval = SYSERR;
                break;
            }
            TFTP_TRACE("Received invalid or unexpected packet.");

            /* サーバからの最初の有効な応答を待っているが、バインドされた接続が
             * サーからのものもでは*ない*場合は、BINDFIRSTフラグをリセットする */
            if (wrong_source && next_block_number == 1)
            {
                irqmask im;
                TFTP_TRACE("Received packet is from wrong source; "
                           "re-setting bind flag.");
                im = disable();
                control(recv_udpdev, UDP_CTRL_BIND, 0, (long)NULL);
                control(recv_udpdev, UDP_CTRL_SETFLAG, UDP_FLAG_BINDFIRST, 0);
                restore(im);
            }

            /* 不正なパケットは無視して、受信を再度試みる */
            continue;
        }

        /* 受信したパケットは次のブロックか前のブロックのいずれかの
         * 正しいTFTPデータパケット */


    #if TFTP_DROP_PACKET_PERCENT != 0
        /* Stress testing.  */
        if (rand() % 100 < TFTP_DROP_PACKET_PERCENT)
        {
            TFTP_TRACE("WARNING: Ignoring valid TFTP packet for test purposes.");
            continue;
        }
    #endif

        /* これがサーバからの最初の応答の場合、応答した実際のポートをセットする */
        if (next_block_number == 1)
        {
            send_udpdev = recv_udpdev;
            TFTP_TRACE("Server responded on port %u; bound socket",
                       udptab[recv_udpdev - UDP0].remotept);
        }

        /* 次のデータブロックの受信を処理する  */
        block_nbytes = TFTP_BLOCK_SIZE;
        if (recv_block_number == (ushort)next_block_number)
        {
            block_nbytes = retval - 4;
            TFTP_TRACE("Received block %u (%u bytes)",
                       recv_block_number, block_nbytes);

            /* 受信したデータをコールバック関数に与える */
            retval = (*recvDataFunc)(pkt.DATA.data, block_nbytes,
                                     recvDataCtx);
            /* コールバック関数がOKを返さなかった場合は復帰する */
            if (OK != retval)
            {
                break;
            }
            next_block_number++;
            block_recv_tries = 0;
        }

        /* ブロックの受信をACKする */
        retval = tftpSendACK(send_udpdev, recv_block_number);

        /* TFTP Get転送は短いデータブロックを受信したときに完了する。
         * クライアントの観点からは、最後のデータブロックがACKされるか
         * 否かは重要でないことに注意されたい。しかし、サーバは最後の
         * ブロックを再送し続けないようにこれを知りたい。このため
         * 最後のACKパケットを送信したが、送信に失敗した場合は無視する */
        if (block_nbytes < TFTP_BLOCK_SIZE)
        {
            retval = OK;
            break;
        }

        /* ACKの送信に失敗した場合はBreak */
        if (SYSERR == retval)
        {
            break;
        }
    }
    /* Clean up and return.  */
out_kill_recv_thread:
    kill(recv_tid);
out_close_udpdev2:
    close(udpdev2);
out_close_udpdev:
    close(udpdev);
    return retval;
}
