/**
 * @file mailbox.h
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#ifndef _MAILBOX_H_
#define _MAILBOX_H_

#include <semaphore.h>
#include <stddef.h>
#include <conf.h>

/** @ingroup mailbox
 * メールボックスは未使用 */
#define MAILBOX_FREE     0
/** @ingroup mailbox
 * メールボックスは割当て済み */
#define MAILBOX_ALLOC    1

/**
 * @ingroup mailbox
 * 
 * メールテーブルエントリの定義. 
 */
struct mbox
{
    semaphore sender;           /**< メールボックスの空きスペースの数       */
    semaphore receiver;         /**< 受信可能なメッセージの数               */
    uint max;                   /**< 保持できるメッセー師の最大数           */
    uint count;                 /**< 現在メールボックスにあるメッセージの数 */
    uint start;                 /**< 最初のメッセージのbっファ内でのindex   */
    uchar state;                /**< メールボックスるの状態                 */
    int *msgs;                  /**< このメールボックス用のメッセージキュー */
};

/** @ingroup mailbox */
typedef uint mailbox;

extern semaphore mboxtabsem;

extern struct mbox mboxtab[];

/* Mailbox function prototypes */
syscall mailboxAlloc(uint);
syscall mailboxCount(mailbox);
syscall mailboxFree(mailbox);
syscall mailboxInit(void);
syscall mailboxReceive(mailbox);
syscall mailboxSend(mailbox, int);

#endif                          /* _MAILBOX_H_ */
