/**
 * @file telnetRead.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stdio.h>
#include <stddef.h>
#include <ctype.h>
#include <semaphore.h>
#include <string.h>
#include <device.h>
#include <telnet.h>
#include <thread.h>

static void telnetEchoNegotiate(struct telnet *, int);
static void telnetEcho(device *, int);
static void telnetSendOption(device *, uchar, uchar);

/**
 * @ingroup telnet
 *
 * telnet接続から文字を読み込む
 * @param devptr telnetデバイスへのポインタ
 * @param buf 文字を読み込むためのバッファ
 * @param len バッファサイズ
 * @return 読み込んだ文字数; ファイルの終わりに達したら ::EOF;
 *          それ以外は ::SYSERR
 */
devcall telnetRead(device *devptr, void *buf, uint len)
{
    struct telnet *tntptr;
    device *phw;
    int ch = 0;
    int count = 0;
    int index = 0;
    bool willSga = FALSE;

    uchar *buffer = buf;
    uchar cmdbuf[3] = { 0, 0, 0 };

    char amthere[] = "\r\n*****connections still open*****\r\n";

    tntptr = &telnettab[devptr->minor];
    phw = tntptr->phw;
    if (NULL == phw)
    {
        TELNET_TRACE("Null phw");
        return SYSERR;
    }

    /* If the EOFフラグがセットされていたらフラグをクリアしてEOFを返す */
    if (tntptr->ieof)
    {
        TELNET_TRACE("EOF");
        tntptr->ieof = FALSE;
        return EOF;
    }

    /* 他の誰かが入力バッファをお更新中? */
    wait(tntptr->isem);

    TELNET_TRACE("Have input semaphore");
    /* 入力バッファにデータが有るかチェックする */
    if (0 == tntptr->icount)
    {
        while ((tntptr->icount < TELNET_IBLEN) && !(tntptr->idelim))
        {
            /* index値を icount + istart にセット */
            index = tntptr->icount + tntptr->istart;

            /* 1文字読み込む */
            ch = (*phw->getc) (phw);
            if (SYSERR == ch)
            {
                TELNET_TRACE("Read error");
                return SYSERR;
            }

            /* 特殊文字を処理して、それ以外は文字のバッファへ追加する */
            switch (ch)
            {
            case '\r':
                TELNET_TRACE("Recv Carriage return");
                tntptr->in[index % TELNET_IBLEN] = ch;
                tntptr->icount++;
                index = tntptr->icount + tntptr->istart;
                telnetEcho(devptr, ch);
                telnetFlush(devptr);
                /* 次の文字を取得して、idelimをセットすべきか判断する */
                ch = (*phw->getc) (phw);
                if (SYSERR == ch)
                {
                    TELNET_TRACE("Read error");
                    return SYSERR;
                }
                switch (ch)
                {
                    /* 次にNULかLFが続かなくてはならない */
                case '\0':
                case '\n':
                    /* バッファに文字を追加して、idelimにTRUEをセットする */
                    tntptr->in[index % TELNET_IBLEN] = ch;
                    tntptr->icount++;
                    tntptr->idelim = TRUE;
                    telnetEcho(devptr, ch);
                    break;
                default:
                    /* デフォルトはNULとする。明らかにCRだけが望ましいからである */
                    tntptr->in[index % TELNET_IBLEN] = '\0';
                    tntptr->icount++;
                    index = tntptr->icount + tntptr->istart;
                    tntptr->in[index % TELNET_IBLEN] = ch;
                    tntptr->icount++;
                    break;
                }
                //if (TELNET_ECHO_SELF_ECHOES == tntptr->echoState)
                if (tntptr->flags & TELNET_FLAG_ECHO)
                {
                    telnetEcho(devptr, ch);
                }
                break;
            case TELNET_CHR_DEL:
            case '\b':
                TELNET_TRACE("Recv Backspace");
                /* 可能であれば位置を1文字だけ左に移動する */
                if (tntptr->icount >= 1)
                {
                    tntptr->icount--;

                    if (tntptr->flags & TELNET_FLAG_ECHO)
                    {
                        telnetEcho(devptr, ch);
                    }
                }
                break;

            case '\n':
                TELNET_TRACE("Recv Newline");
                /* バッファに文字を追加して、idelimにTRUEをセットする */
                tntptr->in[index % TELNET_IBLEN] = ch;
                tntptr->icount++;
                tntptr->idelim = TRUE;
                //if (TELNET_ECHO_SELF_ECHOES == tntptr->echoState)
                if (tntptr->flags & TELNET_FLAG_ECHO)
                {
                    telnetEcho(devptr, ch);
                }
                break;
            case 0x04:
                TELNET_TRACE("Recv EOF");
                /* EOFを処理する */
                tntptr->ieof = TRUE;
                tntptr->idelim = TRUE;
                break;
            case TELNET_IAC:
                /* 次の文字を取得してどのコマンドであるかチェックする */
                ch = (*phw->getc) (phw);
                if (SYSERR == ch)
                {
                    TELNET_TRACE("Recv Command read error");
                    return SYSERR;
                }
                switch (ch)
                {
                case TELNET_WILL:
                    /* 次の文字を取得してどのオプションであるかチェックする */
                    ch = (*phw->getc) (phw);
                    if (SYSERR == ch)
                    {
                        TELNET_TRACE("Recv WILL Read error");
                        return SYSERR;
                    }
                    cmdbuf[2] = ch;

                    /* サーバエンド時にSUPRESS_GAを有効にするリクエスト*/
                    if (TELNET_SUPPRESS_GA == ch)
                    {
                        TELNET_TRACE("Recv WILL Suppress Go-Ahead");
                        cmdbuf[1] = TELNET_WILL;
                        if (FALSE == willSga)
                        {
                            TELNET_TRACE("Send WILL Suppress Go-Ahead");
                            willSga = TRUE;
                            telnetSendOption(phw, cmdbuf[1], cmdbuf[2]);
                        }
                    }
                    else if (TELNET_ECHO == ch)
                    {
                        TELNET_TRACE("Recv WILL Echo");
                        telnetEchoNegotiate(tntptr, TELNET_WILL);
                    }
                    else
                    {
                        TELNET_TRACE("Recv WILL %d (unsupported)", ch);
                        cmdbuf[1] = TELNET_DONT;
                        TELNET_TRACE("Send DONT %d (unsupported)", ch);
                        telnetSendOption(phw, cmdbuf[1], cmdbuf[2]);
                    }

                    break;
                case TELNET_WONT:
                    /* クライアントがwon't echoを要求したらサーバはそうする */
                    ch = (*phw->getc) (phw);
                    if (SYSERR == ch)
                    {
                        TELNET_TRACE("Recv WONT Read error");
                        return SYSERR;
                    }

                    cmdbuf[0] = TELNET_IAC;
                    if (TELNET_ECHO == ch)
                    {
                        TELNET_TRACE("Recv WONT Echo");
                        telnetEchoNegotiate(tntptr, TELNET_WONT);
                    }
                    else
                    {
                        TELNET_TRACE("Recv WONT %d (unsupported)", ch);
                    }
                    /* それ以外はwon'tリクエストを無視する */
                    break;
                case TELNET_DO:
                    /* 次の文字を取得してどのオプションであるかチェックする */
                    ch = (*phw->getc) (phw);
                    if (SYSERR == ch)
                    {
                        TELNET_TRACE("Recv DO   Read error");
                        return SYSERR;
                    }

                    /* どのオプションで適用できるかチェックする */
                    cmdbuf[0] = TELNET_IAC;
                    if (TELNET_SUPPRESS_GA == ch)
                    {
                        TELNET_TRACE("Recv DO   Suppress Go-Ahead");
                        TELNET_TRACE("Send WILL Suppress Go-Ahead");
                        telnetSendOption(phw, TELNET_WILL, ch);
                        tntptr->flags |= TELNET_FLAG_SUPPRESS_GA;
                    }
                    else if (TELNET_ECHO == ch)
                    {
                        TELNET_TRACE("Recv DO   Echo");
                        telnetEchoNegotiate(tntptr, TELNET_DO);
                    }
                    else
                    {
                        TELNET_TRACE("Recv DO   %d (unsupported)", ch);
                        TELNET_TRACE("Send WONT %d (unsupported)", ch);
                        telnetSendOption(phw, TELNET_WONT, ch);
                    }
                    break;
                case TELNET_DONT:
                    /* 次の文字を取得してどのオプションであるかチェックする */
                    ch = (*phw->getc) (phw);
                    if (SYSERR == ch)
                    {
                        TELNET_TRACE("Recv DONT Read error");
                        return SYSERR;
                    }
                    if (TELNET_ECHO == ch)
                    {
                        TELNET_TRACE("Recv DONT Echo");
                        telnetEchoNegotiate(tntptr, TELNET_DONT);
                    }
                    break;

                case TELNET_SE:
                case TELNET_NOP:
                    /* null操作なので、何もしない */
                case TELNET_DM:
                    /* 支給モードを実装していないので意味ない */
                case TELNET_BRK:
                    TELNET_TRACE("Recv Command %d (unsupported)", ch);
                    break;
                case TELNET_IP:
                    TELNET_TRACE("Recv Interrupt");
                    /* 名前が"SHELL"で始まっていない限り、現在実行している
                     * スレッドを終了させる */
                    uint cpuid;
                    cpuid = getcpuid();
                    thrtab_acquire(thrcurrent[cpuid]);
                    if (strncmp(thrtab[thrcurrent[cpuid]].name, "SHELL", 5) != 0)
                    {
                        kill((tid_typ) thrcurrent);
                    }
                    thrtab_release(thrcurrent[cpuid]);
                    break;
                case TELNET_AO:
                    TELNET_TRACE("Recv Abort Output");
                    /* 出力バッファをフラッシュする */
                    telnetFlush(devptr);
                    break;
                case TELNET_AYT:
                    TELNET_TRACE("Recv Are You There");
                    /* "am there"メッセージを送信する */
                    (*phw->write) (phw, (void *)amthere, 36);
                    break;
                case TELNET_EC:
                    TELNET_TRACE("Recv Erase character");
                    if (tntptr->icount >= 1)
                    {
                        tntptr->icount--;
                    }
                    break;
                case TELNET_EL:
                    TELNET_TRACE("Recv Erase Line");
                    while (tntptr->icount >= 1 &&
                           tntptr->in[index % TELNET_IBLEN] != '\n')
                    {
                        tntptr->icount--;
                        index = tntptr->icount + tntptr->istart;
                        tntptr->in[index % TELNET_IBLEN] = 0;
                    }
                    break;
                case TELNET_GA:
                    TELNET_TRACE("Recv Go-Ahead");
                    /* クライアントにsuppress GAを求めているのでこれは送信されない
                     * はずである。念の為、無視する。 */
                    break;
                case TELNET_SB:
                    break;
                }
                break;
            default:
                /* 印刷可能文字であれば入力バッファに追加する */
                if (isprint(ch))
                {
                    tntptr->in[index % TELNET_IBLEN] = ch;
                    tntptr->icount++;
                }
                //if (TELNET_ECHO_SELF_ECHOES == tntptr->echoState)
                if (tntptr->flags & TELNET_FLAG_ECHO)
                {
                    telnetEcho(devptr, ch);
                }
                break;
            }
        }
    }

    /* 入力セマフォに入力バッファの更新終了を通知 */
    signal(tntptr->isem);

    /* 入力バッファからユーザバッファにコピーする */
    while ((0 < tntptr->icount) && (count < len))
    {
        *buffer++ = tntptr->in[tntptr->istart];
        tntptr->icount--;
        tntptr->istart = (tntptr->istart + 1) % TELNET_IBLEN;
        count++;
    }

    if (0 == tntptr->icount)
    {
        tntptr->idelim = FALSE;
    }

    return count;
}

static void telnetEchoNegotiate(struct telnet *tntptr, int command)
{
    device *phw;
    phw = tntptr->phw;
    switch (tntptr->echoState)
    {
    case TELNET_ECHO_SENT_DO:
        if (TELNET_WILL == command)
        {
            TELNET_TRACE("Recv WILL Echo");
            tntptr->echoState = TELNET_ECHO_OTHER_ECHOES;
            TELNET_TRACE("Echo state OTHER_ECHOES");
            tntptr->flags &= ~TELNET_FLAG_ECHO;
        }
        else if (TELNET_WONT == command)
        {
            TELNET_TRACE("Recv WONT Echo");
            telnetSendOption(phw, TELNET_WILL, TELNET_ECHO);
            TELNET_TRACE("Send WILL Echo");
            tntptr->echoState = TELNET_ECHO_SENT_WILL;
            TELNET_TRACE("Echo state SENT_WILL");
        }
        else if (TELNET_DO == command)
        {
            TELNET_TRACE("Recv DO   Echo");
            telnetSendOption(phw, TELNET_WONT, TELNET_ECHO);
            TELNET_TRACE("Send WONT Echo");
        }
        break;
    case TELNET_ECHO_SENT_WILL:
        if (TELNET_DO == command)
        {
            TELNET_TRACE("Recv DO   Echo");
            tntptr->echoState = TELNET_ECHO_SELF_ECHOES;
            TELNET_TRACE("Echo state SELF_ECHOES");
            tntptr->flags |= TELNET_FLAG_ECHO;
        }
        else if (TELNET_DONT == command)
        {
            TELNET_TRACE("Recv DONT Echo");
            tntptr->echoState = TELNET_ECHO_NO_ECHO;
            TELNET_TRACE("Echo state NO_ECHO");
            tntptr->flags &= ~TELNET_FLAG_ECHO;
        }
        else if (TELNET_WILL == command)
        {
            TELNET_TRACE("Recv WILL Echo");
            telnetSendOption(phw, TELNET_DONT, TELNET_ECHO);
            TELNET_TRACE("Send DONT Echo");
        }
        break;
    case TELNET_ECHO_SELF_ECHOES:
        if (TELNET_DONT == command)
        {
            TELNET_TRACE("Recv DONT Echo");
            telnetSendOption(phw, TELNET_WONT, TELNET_ECHO);
            TELNET_TRACE("Send WONT Echo");
            tntptr->echoState = TELNET_ECHO_NO_ECHO;
            TELNET_TRACE("Echo state NO_ECHO");
            tntptr->flags &= ~TELNET_FLAG_ECHO;
        }
        else if (TELNET_WILL == command)
        {
            TELNET_TRACE("Recv WILL Echo");
            telnetSendOption(phw, TELNET_DONT, TELNET_ECHO);
            TELNET_TRACE("Send DONT Echo");
        }
        break;
    case TELNET_ECHO_OTHER_ECHOES:
        if (TELNET_WONT == command)
        {
            TELNET_TRACE("Recv WONT Echo");
            telnetSendOption(phw, TELNET_DONT, TELNET_ECHO);
            TELNET_TRACE("Send DONT Echo");
            telnetSendOption(phw, TELNET_WILL, TELNET_ECHO);
            TELNET_TRACE("Send WILL Echo");
            tntptr->echoState = TELNET_ECHO_SENT_WILL;
            TELNET_TRACE("Echo state SENT_WILL");
        }
        else if (TELNET_DO == command)
        {
            TELNET_TRACE("Recv DO   Echo");
            telnetSendOption(phw, TELNET_WONT, TELNET_ECHO);
        }
        break;
    case TELNET_ECHO_NO_ECHO:
        if (TELNET_DO == command)
        {
            TELNET_TRACE("Recv DO   Echo");
            telnetSendOption(phw, TELNET_WILL, TELNET_ECHO);
            TELNET_TRACE("Send WILL Echo");
            tntptr->echoState = TELNET_ECHO_SELF_ECHOES;
            TELNET_TRACE("Echo state SELF_ECHOES");
            tntptr->flags |= TELNET_FLAG_ECHO;
        }
        else if (TELNET_WILL == command)
        {
            TELNET_TRACE("Recv WILL Echo");
            telnetSendOption(phw, TELNET_DONT, TELNET_ECHO);
            TELNET_TRACE("Send DONT Echo");
        }
        break;
    default:
        TELNET_TRACE("Echo state unknown %d!", tntptr->echoState);
        //Error, unknown echoState.
        break;
    }
}

static void telnetEcho(device *devptr, int ch)
{
    if (('\b' == ch) || (TELNET_EC == ch) || (TELNET_CHR_DEL == ch))
    {
        telnetPutc(devptr, '\b');
        telnetPutc(devptr, ' ');
        telnetPutc(devptr, '\b');
        telnetFlush(devptr);
        return;
    }

    if ('\r' == ch)
    {
        telnetPutc(devptr, '\r');
        telnetPutc(devptr, '\n');
        telnetFlush(devptr);
        return;
    }

    if ('\n' == ch)
    {
        return;
    }

    if (!isprint(ch))
    {
        return;
    }

    telnetPutc(devptr, (char)ch);
    telnetFlush(devptr);
}

static void telnetSendOption(device *phw, uchar command, uchar option)
{
    uchar cmdbuf[3];
    cmdbuf[0] = TELNET_IAC;
    cmdbuf[1] = command;
    cmdbuf[2] = option;
    (*phw->write) (phw, (void *)cmdbuf, 3);
}
