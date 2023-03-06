UDP
===

XINUは **UDP** (**User Datagram Protocol**)をサポートしています。
このサポートは :source:`device/udp/` ディレクトリで見ることができます。
:source:`network/` では *ない* ことに注意してください。これはUDP
モジュールが ``open()``, ``control()``, ``read()``, ``write()``,
``close()`` などのXINUの汎用デバイス関数を使用して制御可能な
**UDPデバイス** を提供するために設計されているからです。

.. contents::
   :local:

デバッグ
---------

UDPモジュールにはデバッグのための :doc:`Trace statements </development/Trace>`
が含まれています。これを有効にするには :source:`include/udp.h` の
以下の行のコメントをはずしてください。また、必要であれば、メッセージを
出力するデバイス（TTY1など）を変更してください::

    // #define TRACE_UDP  TTY1

例 （TFTPクライアント）
--------------------------

:source:`network/tftp/tftpGet.c` を見てください。コードには少し
難しいところもありますが、実際のプロトコル (TFTP GET) をサポート
しています。

例 (client + server)
-------------------------

概要
~~~~~

XINU（バージョン2.0以降）のUDPネットワーク機能の利用法を示す
簡単な例です。

利用方法
~~~~~~~~~~

-  以下の2つのファイルを ``xsh_udpclient.c`` と ``xsh_udpserver.c``
   としてシェルディレクトリに追加する。
-  ``system/shell.c`` と ``include/shell.h`` を変更して
   udpclientコマンドとudpserverコマンドを含める。
-  ``shell/Makerules`` を変更して ``xsh_udpclient.c`` と
   ``xsh_udpserver.c`` を追加して, compileディレクトリでXINUをmakeする。
-  XIMUコンソールを2つ起動する。
-  両コンソールのshellで **netup** を実行する。
-  1つ目のコンソールのshellで **udpserver** を実行する。
-  2つ目のコンソールのshellで **udpclient** をudpserverのIPアドレスと
   引用符で囲んだ送信メッセージを引数として実行する。たとえば、
   次のように:

        ``udpclient 192.168.6.102 "Hello XINU World!"``

``xsh_udpclient.c``
~~~~~~~~~~~~~~~~~~~

.. code:: c

    #include <stddef.h>
    #include <stdio.h>
    #include <device.h>
    #include <ether.h>
    #include <udp.h>
    #include <string.h>

    #define MSG_MAX_LEN 64
    #define ECHO_PORT 9989


    /**
    * Shellコマンド (udpclient) はUDPを使ってネットワーク経由で
    * ASCIIメッセージをサーバーに送信するクライアントである。
    * arg0にはechoclient, args1には宛先のIPアドレス、
    * args2には引用符で囲んだメッセージを想定する。
    * @param nargs 引数配列の引数の数
    * @param args 引数の配列
    * @return エラーの場合は非0の値
    */
    shellcmd xsh_udpclient(int nargs, char *args[])
    {
        ushort dev = 0;
        char buf[MSG_MAX_LEN];

        char *dest = args[1];

        struct netaddr dst;
        struct netaddr *localhost;
        struct netif *interface;


        /* 新しいUDPドエバイスを割り当てる */
        if ((ushort)SYSERR == (dev = udpAlloc()))
        {
            fprintf(stderr, "Client: Failed to allocate a UDP device.");
            return SYSERR;
        }

        /* ローカルIP情報を検索する */
        interface = netLookup((ethertab[0].dev)->num);
        if (NULL == interface)
        {
            fprintf(stderr, "Client: No network interface found\r\n");
            return SYSERR;
        }
        localhost = &(interface->ip);

        /* 宛先をipv4アドレスに変更する */
        if (SYSERR == dot2ipv4(dest, &dst))
        {
            fprintf(stderr, "Client: Failed to convert ip address.");
            return SYSERR;
        }

        /* 宛先とechoポートを指定してUDPデバイスを開く */
        if (SYSERR == open(dev, localhost, &dst, NULL, ECHO_PORT))
        {
            fprintf(stderr, "Client: Could not open the UDP device\r\n");
            return SYSERR;
        }

        /* 宛先にメッセージを送信する */
        memcpy(buf, args[2], MSG_MAX_LEN);

        if(SYSERR == write(dev, buf, MSG_MAX_LEN))
        {
            close(dev);
            return SYSERR;
        }

        /* 終わったらデバイスを閉じる */
        close(dev);

        return 0;
    }

``xsh_udpserver.c``
~~~~~~~~~~~~~~~~~~~

.. code:: c

    #include <stddef.h>
    #include <stdio.h>
    #include <device.h>
    #include <udp.h>
    #include <stdlib.h>
    #include <ether.h>
    #include <string.h>

    #define ECHO_PORT 9989

    /**
    * Shellコマンド (udpserver) は着信メッセージを待ち、
    * メッセージをプリントアウトするUDPサーバーを実行する。
    * どんな引数も想定しない。
    * @param nargs 引数配列の引数の数
    * @param args 引数の配列
    * @return エラーの場合は非0の値
    */
    shellcmd xsh_echoserver(int nargs, char *args[])
    {
        ushort dev = 0;
        int len = 0;

        char buffer[UDP_MAX_DATALEN];

        struct netaddr *localhost;

        struct netif *interface;
        struct udpPseudoHdr *pseudo;
        struct udpPkt *udp;


        /* 新しいUDPドエバイスを割り当てる */
        if ((ushort)SYSERR == (dev = udpAlloc()))
        {
            fprintf(stderr, "Server: Failed to allocate a UDP device.\r\n");
            return SYSERR;
        }

        /* ローカルIP情報を検索する */
        interface = netLookup((ethertab[0].dev)->num);

        if (NULL == interface)
        {
            fprintf(stderr, "Server: No network interface found\r\n");
            return SYSERR;
        }


        /* localhostとlistenするechoポートを指定してUDPデバイスを開く */
        localhost = &(interface->ip);

        if (SYSERR == open(dev, localhost, NULL, ECHO_PORT, NULL))
        {
            fprintf(stderr, "Server: Could not open the UDP device\r\n");
            return SYSERR;
        }

        /* UDPデバイスをpassiveモードにセットする */
        if (SYSERR == control(dev, UDP_CTRL_SETFLAG, UDP_FLAG_PASSIVE, NULL))
        {
            kprintf("Server: Could not set UDP device to passive mode\r\n");
            close(dev);
            return SYSERR;
        }


        /* Readループして新しいリクエストを待つ */
        printf("Server: Waiting for message\r\n");

        while (SYSERR != (len = read(dev, buffer, UDP_MAX_DATALEN)))
        {
            pseudo = (struct udpPseudoHdr *)buffer;
            udp = (struct udpPkt *)(pseudo + 1);
            printf("Server: Received Message - %s\r\n", udp->data);
        }

            close(dev);

        return 0;
    }

資料
---------

* :wikipedia:`User Datagram Protocol - Wikipedia <User Datagram Protocol>`
* :rfc:`768`
