/**
 * @file netGetbuf.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <bufpool.h>
#include <network.h>
#include <stdlib.h>

/**
 * @ingroup network
 *
 * パケットを格納するバッファを提供する.
 * @return パケットバッファへのポインタ, エラーが発生したら SYSERR
 */
struct packet *netGetbuf(void)
{
    struct packet *pkt = NULL;          /* pointer to packet            */

    /* Obtain a buffer for the packet */
    pkt = bufget(netpool);
    if (SYSERR == (int)pkt)
    {
        return (struct packet *)SYSERR;
    }

    bzero(pkt, sizeof(struct packet) + NET_MAX_PKTLEN);

    /* Initialize packet buffer */
    pkt->nif = NULL;
    pkt->len = 0;
    /* currがバッファの最後の位置を指すように初期化する */
    pkt->curr = pkt->data + NET_MAX_PKTLEN;

    return pkt;
}
