/**
 * @file     xsh_nslookup.c
 *
 */

#include <conf.h>

#if NETHER

#include <device.h>
#include <ether.h> /* For ethertab */
#include <network.h>
#include <dns.h>
#include <shell.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef WITH_DHCPC
#  include <dhcpc.h>
#endif


static void usage(const char *command);

/**
 * @ingroup shell
 *
 * DNSを使ってドメイン名をIPアドレスに解決するシェルコマンド (nslookup).
 * @param nargs  引数の数
 * @param args   引数配列
 * @return 成功したら OK, それ以外は SYSERR
 */
shellcmd xsh_nslookup(int nargs, char *args[])
{
    struct netaddr ipaddr;

    /* Help */
    if ((2 == nargs) && (0 == strcmp(args[1], "--help")))
    {
        usage(args[0]);
        return SHELL_OK;
    }

    /* Parse device if specifed */
    if (nargs != 2)
    {
        fprintf(stderr, "ERROR: No domain name is specified.\n");
        return SHELL_ERROR;
    }

    /* ETH0がupしていなかったらエラー */
    if (netiftab[0].state != NET_ALLOC) {
        fprintf(stderr, "Network is down. DO netup command first!.\n");
        return SHELL_ERROR;
    }


    if (dnsResolve(&netiftab[0], args[1], &ipaddr) == SYSERR) {
        fprintf(stderr, "ERROR: Failed to resolve domain name %s.\n", args[1]);
        return SHELL_ERROR;
    }

    fprintf(stdout, "RESOLVED: domain %s -> ip address %d.%d.%d.%d.\n", args[1],
        ipaddr.addr[0], ipaddr.addr[1], ipaddr.addr[2], ipaddr.addr[3]);

    return SHELL_OK;
}

static void usage(const char *command)
{
    printf("Usage:\n\t%s DomainName\nDescription:\n\tResolve a domain name to IP address.\n", command);
}

#endif /* NETHER */
