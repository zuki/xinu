TCP
===

XINUは **TCP** (**Transmission Control Protocol**)をサポートしています。
このサポートは :source:`device/tcp/` ディレクトリで見ることができます。
:source:`network/` では *ない* ことに注意してください。これはTCP
モジュールが ``open()``, ``control()``, ``read()``, ``write()``,
``close()`` などのXINUの汎用デバイス関数を使用して制御可能な
**TCPデバイス** を提供するために設計されているからです。

.. contents::
   :local:

デバッグ
---------

TCPモジュールにはデバッグのための :doc:`Trace statements </development/Trace>`
が含まれています。これを有効にするには :source:`include/tcp.h` の
以下の行のコメントをはずしてください。また、必要であれば、メッセージを
出力するデバイス（TTY1など）を変更してください::

    // #define TRACE_TCP  TTY1

例 - TCPエコーテスト
-----------------------

概要
~~~~~

これは簡単なTCPの使用例であり、エコーサーバにメッセージを送信して、その
応答をプリントアウトするエコーテストクライアントです。エコープロトコルは
:rfc:`RFC 862 <862>` で定義されています。手短に言うと、サーバにメッセージを
送ると、サーバはまったく同じメッセージをエコーバックします。

利用方法
~~~~~~~~~~っｍ

-  以下のソースファイルを ``shell/xsh_echotest.c`` として追加する。
-  ``shell/shell.c`` と ``include/shell.h`` を変更して **echotest** に
   含める。
-  ``shell/Makerules`` を変更して ``xsh_echotest.c`` を追加して、
   compileディレクトリでXINUをmakeする。
-  XINUを起動する。
-  shellから **netup** コマンドを実行する。
-  shellから **echoclient** コマンドをエコーサーバのIPアドレスと引用符で
   囲んだ送信メッセージを引数として実行する。たとえば、次のように:

       ``echoclient 192.168.6.102 "Hello XINU World!"``

``xsh_echotest.c``
------------------

.. code:: c

    #include <stddef.h>
    #include <stdio.h>
    #include <device.h>
    #include <ether.h>
    #include <tcp.h>
    #include <string.h>

    #define MSG_MAX_LEN 64
    #define ECHO_PORT 7

    /**
     * Shellコマンド (echotest) はRFC 862に基づいてメッセージをエコー
     * サーバに送信する。そして応答を待ち、エコーサーバからの返答を
     * プリントアウトする。
     * 想定する引数: echotest, エコーサーバのip, 引用符で囲んだメッセージ
     * @param nargs 引数配列の引数の数
     * @param args  引数の配列
     * @return エラーの場合は非0の値
     */
    shellcmd xsh_echotest(int nargs, char *args[])
    {
        ushort dev = 0;
        char buf[MSG_MAX_LEN + 1];

        char *dest = args[1];

        struct netaddr dst;
        struct netaddr *localhost;
        struct netif *interface;

        int len;

        /* 新しいTCPドエバイスを割り当てる */
        if ((ushort)SYSERR == (dev = tcpAlloc()))
        {
            fprintf(stderr, "Client: Failed to allocate a TCP device.");
            return SYSERR;
        }

        /* ローカルIP情報を検索する */
        interface = netLookup((ethertab[0].dev)->num);
        if (NULL == interface)
        {
            fprintf(stderr, "Client: No network interface found\r\n");
            return SYSERR;
        }
        localhost = &(interface->ip);

        /* 宛先をipv4アドレスに変更する */
        if (SYSERR == dot2ipv4(dest, &dst))
        {
            fprintf(stderr, "Client: Failed to convert ip address.");
            return SYSERR;
        }

        /* 宛先とechoポートを指定してTCPデバイスを開く */
        if (SYSERR == open(dev, localhost, &dst, NULL, ECHO_PORT, TCP_ACTIVE))
        {
            fprintf(stderr, "Client: Could not open the TCP device\r\n");
            return SYSERR;
        }

        /* 宛先にメッセージを送信する */
        memcpy(buf, args[2], MSG_MAX_LEN);

        if(SYSERR == write(dev, buf, MSG_MAX_LEN))
        {
            fprintf(stderr, "Client: Error writing packet to the network");
            close(dev);
            return SYSERR;
        }

        /* サーバーからの応答を読み込む */
        if(SYSERR != (len = read(dev, buf, MSG_MAX_LEN)))
        {
            /* 不正な応答の場合にはマニュアルでヌル終端する必要がある */
            buf[len] = '\0';
            printf("Client: Got response - %s\r\n", buf);
        }

        /* 終わったらデバイスを閉じる */
        close(dev);

        return 0;
    }

資料
---------

* :wikipedia:`Transmission Control Protocol - Wikipedia <Transmission Control Protocol>`
* :rfc:`793`
