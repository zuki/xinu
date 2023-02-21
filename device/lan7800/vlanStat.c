/**
 * @file vlanStat.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <ether.h>
#include <stdio.h>

/**
 * @ingroup ether_lan7800
 *
 * VLAN統計を出力. 未実装
 */
int vlanStat(void)
{
    fprintf(stderr, "ERROR: VLANs not supported by this driver.\n");
    return SYSERR;
}
