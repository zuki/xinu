/*
 * @file     xsh_date.c
 *
 */
/* Embedded Xinu, Copyright (C) 2008.  All rights reserved. */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <date.h>
#include <shell.h>
#include <network.h>
#include <ntp.h>

/**
 * @ingroup shell
 *
 * Shell command (date).
 * @param nargs  number of arguments in args array
 * @param args   array of arguments
 * @return OK for success, SYSERR for syntax error
 */
shellcmd xsh_date(int nargs, char *args[])
{
    uint32_t epoc;
    int set = 0;
    uint32_t now;

    /* Output help, if '--help' argument was supplied */
    if (nargs == 2) {
        if (strcmp(args[1], "-s") == 0) {
            set = 1;
        } else {
            printf("Usage: %s\n\n", args[0]);
            printf("Description:\n");
            printf("\tDisplays current time\n");
            printf("Options:\n");
            printf("\t-s\tset datetime from ntp server\n");
            printf("\t--help\tdisplay this help and exit\n");
            return OK;
        }
    }

    /* Check for correct number of arguments */
    if (nargs > 2)
    {
        fprintf(stderr, "%s: too many arguments\n", args[0]);
        fprintf(stderr, "Try '%s --help' for more information\n",
                args[0]);
        return SYSERR;
    }
    if (nargs < 1)
    {
        fprintf(stderr, "%s: too few arguments\n", args[0]);
        fprintf(stderr, "Try '%s --help' for more information\n",
                args[0]);
        return SYSERR;
    }

    if (set) {
        /* ETH0がupしていなかったらエラー */
        if (netiftab[0].state != NET_ALLOC) {
            fprintf(stderr, "Network is down. DO netup command first!.\n");
            return SHELL_ERROR;
        }

        if (ntpGetEpoc(&netiftab[0], &epoc) != OK) {
            fprintf(stderr, "Could not get epoc from NTP server.\n");
            return SHELL_ERROR;
        }
        printDate(set_datetime(epoc));
    } else {
        now = get_datetime();
        if (now == 0) {
            printf("Datetime not set, run with '-s' option\n");
            return SHELL_ERROR;
        }
        printDate(now);
    }

    return OK;
}
