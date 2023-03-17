/**
 * @file dns.c
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <thread.h>
#include <device.h>
#include <network.h>
#include <ipv4.h>
#include <dns.h>
#include <udp.h>

/**
 * @fn dnsQuery
 *
 * DNS問い合わせを作成してパケット長を返す.
 *
 * 例: www.example.com -> 3www7example3com0 + QType + QClass
 *
 * @param dname ドメイン名
 * @param データ用のバッファへのポインタ
 * @param 照会タイプ
 * @return パケット長
 */
int32_t dnsQuery(char *dname, char *data, uint16_t type)
{
    uint32_t    qlen;           /* Length of Question        */
    uint32_t    dlen;           /* Length of domain name    */
    uint8_t    *llptr;          /* Label length pointer        */
    int32_t     i;              /* Loop index            */
    uint16_t    tmp;            /* Temporary for conversion    */

    /* 1. ドメイン名の長さを取得する */
    dlen = strlen(dname);

    /* 2. 次のラベルに必要な長さバイトを割り当て、0から始める */
    llptr = (uint8_t *)(data++);
    *llptr = 0;

    /* 3. 問い合わせ長を初期化する */
    qlen = 1;

    /* 4. ドメイン異名の各文字を問い合わせに追加する */
    for (i = 0; i < dlen; i++) {
        if (qlen >= DNS_DATASIZE) {
            return SYSERR;
        }
        if (dname[i] != '.') {
            /* 通常の文字は既存のラベルに追加する */
            *data++ = dname[i];
            *llptr = *llptr + 1;    // ラベル長 += 1
        } else {
            /* ドットが来たら次のラベルを開始する */
            llptr = (uint8_t *)(data++);
            *llptr = 0;             // ラベル長 = 0
        }
        qlen++;
    }
    /* 5. 問い合わせを長さ0で終端する */
    *data++ = 0;
    qlen++;

    /* 6. QTypeにType A Addressをセットする */
    tmp = hs2net(type);
    memcpy(data, (char *)&tmp, 2);
    data += 2;
    qlen += 2;

    /* 7. QClassにInternetをセットする */
    tmp = hs2net(DNS_QC_IN);
    memcpy(data, (char *)&tmp, 2);
    qlen += 2;

    /* 8. 合計のパケット長を返す */
    return sizeof(struct dnsPkt) - DNS_DATASIZE + qlen;
}

/**
 * @fn dnsGetA
 *
 * GetA応答セクションからもっともふさわしいIPアドレスを返す
 * @param dname ドメイン名
 * @param rpkt DNS応答メッセージパケットへのポインタ
 * @param addr IPアドレス
 * @return 成功したら OK; そうでなければ SYSERR
 */
syscall dnsGetA(char *dname, struct dnsPkt *rpkt, uint32_t *addr)
{
    uint16_t    qcount;     /* Number of Questions        */
    uint16_t    tmp16;      /* Used for endian conversion    */
    uint16_t    acount;     /* Number of Answers        */
    uint32_t    ipaddr;     /* IP address from the response    */
    char       *dptr;       /* Data pointer            */
    char        llen;       /* Label length            */
    int32_t     i;          /* Loop index            */
    char cname[1024] = {0}; /* CNAME */

    /* 1. 問い合わせ回数を抽出 */
    memcpy((char *)&tmp16, (char *) &rpkt->qucount, 2);
    qcount = net2hs(tmp16);
    dptr = rpkt->data;

    /* 2. qcount個の問い合わせをスキップ */
    for (i = 0; i < qcount; i++) {
        /* 2.1 ラベル長を取得 */
        llen = *((char *)dptr);
        /* 2.2 ドメイン名の終わりまで繰り返す */
        while (llen != 0) {
            /* llen[0:1] != 0b00 ならドメイン名はオフセット値で表現 */
            if (llen > 63) {
                dptr += 2;
                break;
            }
            /* 次のラベルに移動 */
            dptr += (llen + 1);
            llen = *((char *)dptr);
        }

        /* 2.3 次の問い合わせに移動 */
        if (llen == 0) {
            dptr += 1;
        }

        /* QtypeとQclassをスキップ */
        dptr += (2 + 2);
    }

    /* 3. 回答数を取得 */
    memcpy((char *)&tmp16, (char *)&rpkt->ancount, 2);
    acount = net2hs(tmp16);

    /* 4. IPアドレスを0にセット */
    ipaddr = 0;

    /* 5. 各回答がローカルネットにマッチするかチェックする */
    for (i = 0; i < acount; i++) {
        char        rname[1024];
        uint16_t    tmptype;
        uint32_t    tmpip;
        uint16_t    tmplen;
        uint32_t    namlen;

        /* 5.1 ドメイン名とそのパケットに占めるバイト数を取得する
         * （名前がオフセットで指定されている場合は短い） */
        namlen = dnsGetRName((char *)rpkt, dptr, rname);
        dptr += namlen;
        /* 5.2 回答がマッチしType Aであるか確認する */
        memcpy((char *)&tmptype, dptr, 2);
        if (((strncmp(dname, rname, strlen(dname)) == 0)
           || (cname != NULL && strncmp(cname, rname, strlen(cname)) == 0)) &&
            (net2hs(tmptype) == DNS_QT_A) ) {
            /* 5.2.1 IPアドレスをピックアップ */
            memcpy((char *)&tmpip, dptr+10, 4);
            if (ipaddr == 0) {
                ipaddr = tmpip;
            }
        } else if (net2hs(tmptype) == DNS_QT_CNAME) {
            namlen = dnsGetRName((char *)rpkt, dptr+10, cname);
        }
        /* 5.3 type, class, ttlを読み飛ばす */
        dptr += 8;
        /* 5.4 length (2), dataを読み飛ばす */
        memcpy((char *)&tmplen, dptr, 2);
        dptr += net2hs(tmplen) + 2;
    }

    if (ipaddr != 0) {
        *addr = net2hl(ipaddr);
        return OK;
    } else {
        return SYSERR;
    }
}

/**
 * @fn dnsGetCNAME
 *
 * GetCNAME応答セクションからCNAMEを返す
 * @param dname ドメイン名
 * @param rpkt DNS応答メッセージパケットへのポインタ
 * @param cname CNAME
 * @return 成功したら OK; そうでなければ SYSERR
 */
syscall dnsGetCNAME(char *dname, struct dnsPkt *rpkt, char *cname)
{
    uint16_t    qcount;     /* Number of Questions        */
    uint16_t    tmp16;      /* Used for endian conversion    */
    uint16_t    acount;     /* Number of Answers        */
    char       *dptr;       /* Data pointer            */
    char        llen;       /* Label length            */
    int32_t     i;          /* Loop index            */
    int         resolved = 0;

    /* 1. 問い合わせ回数を抽出 */
    memcpy((char *)&tmp16, (char *) &rpkt->qucount, 2);
    qcount = net2hs(tmp16);
    dptr = rpkt->data;

    /* 2. qcount個の問い合わせをスキップ */
    for (i = 0; i < qcount; i++) {
        /* 2.1 ラベル長を取得 */
        llen = *((char *)dptr);
        /* 2.2 ドメイン名の終わりまで繰り返す */
        while (llen != 0) {
            /* llen[0:1] != 0b00 ならドメイン名はオフセット値で表現 */
            if (llen > 63) {
                dptr += 2;
                break;
            }
            /* 次のラベルに移動 */
            dptr += (llen + 1);
            llen = *((char *)dptr);
        }

        /* 2.3 次の問い合わせに移動 */
        if (llen == 0) {
            dptr += 1;
        }

        /* QtypeとQclassをスキップ */
        dptr += (2 + 2);
    }

    /* 3. 回答数を取得 */
    memcpy((char *)&tmp16, (char *)&rpkt->ancount, 2);
    acount = net2hs(tmp16);

    /* 5. 各回答がローカルネットにマッチするかチェックする */
    for (i = 0; i < acount; i++) {
        char        rname[1024];
        uint16_t    tmptype;
        uint16_t    tmplen;
        uint32_t    namlen;
        uint16_t    datalen;

        /* 5.1 ドメイン名とそのパケットに占めるバイト数を取得する
         * （名前がオフセットで指定されている場合は短い） */
        namlen = dnsGetRName((char *)rpkt, dptr, rname);
        dptr += namlen;
        /* 5.2 回答がマッチしType CNAMEであるか確認する */
        memcpy((char *)&tmptype, dptr, 2);
        memcpy((char *)&tmplen, dptr+8, 2);
        datalen = net2hs(tmplen);
        if ((strncmp(dname, rname, strlen(dname)) == 0) &&
            (net2hs(tmptype) == DNS_QT_CNAME) ) {
            /* 5.2.1 CNAMEをピックアップ */
            datalen = dnsGetRName((char *)rpkt, dptr+10, cname);
            resolved = 1;
            break;
        }
        /* 5.3 type, class, ttlを読み飛ばす */
        dptr += 8;
        /* 5.4 length (2), cnameを読み飛ばす */
        dptr += 2 + datalen;
    }

    if (resolved) {
        return OK;
    } else {
        return SYSERR;
    }
}

/**
 * @fn dnsGetRName
 *
 * RRにあるドメイン名をヌル終端の文字列に変換する.
 * @param sop パケットの開始アドレス
 * @param son 名前の開始アドレス
 * @param dst 文字列を格納するバッファへのポインタ
 * @return 処理したドメイン名の長さ（オフセットの場合は2）
 */
uint32_t dnsGetRName(char *sop, char *son, char *dst)
{
    char        llen;           /* Label length            */
    uint16_t    tmpoff;         /* Temporary to hold offset    */
    uint16_t    offset;         /* Offset in host byte order    */
    char       *sson;           /* Saved start of name        */
    int32_t     i;              /* Loop index            */

    /* 1. 最初の位置を記録する */
    sson = son;

    /* 2. 最初のラベル長を取得する */
    llen = *son++;

    /* 3. 斜めの終わりでない間、ラベルを取得する */
    while (llen != 0) {
        if(llen <= 63) {
            /* 3.1 ラベルだけ文字列をdstにコピーする（オフセット値でない） */
            for (i = 0; i < llen; i++) {
                *dst++ = *son++;
            }
            *dst++ = '.';
            llen = *son++;
        } else {
            /* 3.2 オフセットを処理する */
            son--;
            memcpy( (char *)&tmpoff, son, 2);
            offset = net2hs(tmpoff) & 0x3fff;
            son += 2;
            dnsGetRName(sop, sop+offset, dst);
            return (son-sson);
        }
    }

    /* 4. 文字列をヌル終端する */
    dst--;
    *dst = NULLCH;

    return (uint32_t)(son-sson);
}
