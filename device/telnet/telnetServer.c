/**
 * @file telnetServer.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stdio.h>
#include <stddef.h>
#include <device.h>
#include <tcp.h>
#include <shell.h>
#include <thread.h>
#include <telnet.h>
#include <core.h>

thread telnetServerKiller(ushort, ushort);

/**
 * @ingroup telnet
 *
 * telnetサーバを開始する.
 * @param ethdev  telnetサーバをlistenするインタフェース
 * @param port  サーバを開始するポート
 * @param telnetdev  接続に使用するtelnetデバイス
 * @param shellname  接続に使用するshellデバイス
 * @return  成功の場合は ::OK; 失敗の場合は ::SYSERR
 */
thread telnetServer(int ethdev, int port, ushort telnetdev,
                    char *shellname)
{
    tid_typ tid, killtid;
    ushort tcpdev;
    struct netif *interface;
    struct netaddr *host;
    char thrname[24];
    uchar buf[6];

    TELNET_TRACE("ethdev %d, port %d, telnet %d", ethdev, port,
                 telnetdev);
    /* ethernetデバイスのnetaddrを探す */
    interface = netLookup(ethdev);
    if (NULL == interface)
    {
        close(telnetdev);
        return SYSERR;
    }
    host = &(interface->ip);

    while (TRUE)
    {
        tcpdev = tcpAlloc();
        if (SYSERR == (short)tcpdev)
        {
            close(telnetdev);
            fprintf(stderr,
                    "telnet server failed to allocate TCP device\n");
            return SYSERR;
        }
        sprintf(thrname, "telnetSvrKill_%d", (devtab[telnetdev].minor));
        killtid = create((void *)telnetServerKiller, INITSTK, INITPRIO,
                         thrname, 2, telnetdev, tcpdev);
        ready(killtid, RESCHED_YES, CORE_ZERO);

        if (open(tcpdev, host, NULL, port, NULL, TCP_PASSIVE) < 0)
        {
            kill(killtid);
            close(tcpdev);
            close(telnetdev);
            fprintf(stderr,
                    "telnet server failed to open TCP socket %d\n",
                    tcpdev);
            return SYSERR;
        }

        if (SYSERR == open(telnetdev, tcpdev))
        {
            kill(killtid);
            close(tcpdev);
            close(telnetdev);
            fprintf(stderr,
                    "telnet server failed to open TELNET device %d\n",
                    telnetdev);
            return SYSERR;
        }
#ifdef TELNET0
        TELNET_TRACE("telnetServer() opened telnet %d on TCP%d\n",
                     telnetdev - TELNET0, tcpdev - TCP0);
#endif

        /* 次のオプションをクライアントに要求する */
        buf[0] = TELNET_IAC;
        buf[1] = TELNET_WILL;
        buf[2] = TELNET_ECHO;
        buf[3] = TELNET_IAC;
        buf[4] = TELNET_DO;
        buf[5] = TELNET_SUPPRESS_GA;
        write(tcpdev, (void *)buf, 6);

        TELNET_TRACE
            ("telnetServer() sending WILL ECHO and Suppress GA\n");

        /* TELNETデバイス上でshellをオープンする */
        tid = create((void *)shell, INITSTK, INITPRIO, shellname, 3,
                     telnetdev, telnetdev, telnetdev);
        if (SYSERR == tid)
        {
            close(tcpdev);
            close(telnetdev);
            kill(killtid);
            return SYSERR;
        }
        /* 保留中のメッセージをすべてクリアする */
        recvclr();
        TELNET_TRACE("telnetServer() spawning shell thread %d\n", tid);
        ready(tid, RESCHED_YES, CORE_ZERO);

        /* 子プロセスが死ぬまでループする */
        while (recvclr() != tid)
        {
            sleep(200);
            control(telnetdev, TELNET_CTRL_FLUSH, 0, 0);
        }

        if (SYSERR == close(tcpdev))
        {
            close(telnetdev);
            kill(killtid);
            return SYSERR;
        }
        if (SYSERR == close(telnetdev))
        {
            kill(killtid);
            return SYSERR;
        }
    }

    return SYSERR;
}

/**
 * @ingroup telnet
 *
 * 起動されたtelnetサーバをkillする.
 * @param telnetdev クローズするtelnetデバイス
 * @param tcpdev クローズするtcpデバイス
 * @return 状態を返す
 */
thread telnetServerKiller(ushort telnetdev, ushort tcpdev)
{
    int minor, sem;

    minor = devtab[telnetdev].minor;
    sem = telnettab[minor].killswitch;

    /* デバイスクローズセマフォを待機する */
    wait(sem);

    TELNET_TRACE("Killing server");

    /* tcpデバイスをクローズする */
    close(tcpdev);

    /* telnetデバイスをクローズする */
    close(telnetdev);

    return OK;
}
