/**
 * @file safemem.h
 * Definitions for memory protection, including region-based memory
 * allocator, user thread memory allocator, and paging functions.
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#ifndef _SAFEMEM_H_
#define _SAFEMEM_H_

#include <stddef.h>
#include <stdint.h>
#include <conf.h>

#define PAGE_SIZE 4096

/** メモリアドレスを上位ページ境界に丸める */
#define roundpage(x) ((4095 + (uint64_t)(x)) & ~0x0FFF)
/** メモリアドレスを下位ページ境界に切り捨てる */
#define truncpage(x) ((uint64_t)(x) & ~0x0FFF)

/* Region allocator */

/**
 * Structure for a memory regions (collection of page sized memory chunks).
 */
struct memregion
{
    struct memregion *prev;     /**< pointer to previous region in list */
    struct memregion *next;     /**< pointer to next region in list */
    void *start;                /**< Starting address (page aligned) */
    uint32_t length;                /**< Size of region */
    tid_typ thread_id;          /**< Holding thread identifier */
};

extern struct memregion *regfreelist;  /**< List of free regions */
extern struct memregion *regalloclist; /**< List of allocated regions */
extern struct memregion *regtab;       /**< Array of regions */

/* Prototypes for memory region allocator */
void memRegionInit(void *, uint32_t);
void memRegionClear(struct memregion *);
void memRegionInsert(struct memregion *, struct memregion **);
void memRegionRemove(struct memregion *, struct memregion **);
struct memregion *memRegionAlloc(uint32_t);
struct memregion *memRegionSplit(struct memregion *, uint32_t length);
struct memregion *memRegionValid(void *);
void memRegionTransfer(void *, tid_typ);
void memRegionReclaim(tid_typ);

/* Memory protection structures and data */

#define ENT_GLOBAL  0x01 /**< Page table entry is accessible to all */
#define ENT_PRESENT 0x02 /**< Page table entry is present in system */
#define ENT_WRITE   0x04 /**< Page table entry is writable          */
#define ENT_ALL     (ENT_WRITE | ENT_PRESENT | ENT_GLOBAL)
#define ENT_USER    (ENT_WRITE | ENT_PRESENT)

/**
 * Page table entry
 */
struct pgtblent
{
    int entry;                      /**< 0: TLB entry (frame and attr) */
    char resv[3];                   /**< 4: reserved space             */
    char asid;                      /**< 7: address space identifier   */
};

extern struct pgtblent *pgtbl;      /**< system page table */
extern uint32_t pgtbl_nents;            /**< number of pages in page table */

/* Prototypes for memory protection functions */
void safeInit(void);
int safeMap(void *, int16_t);
int safeMapRange(void *, uint32_t, int16_t);
int safeUnmap(void *);
int safeUnmapRange(void *, uint32_t);
void safeKmapInit(void);

#endif                          /* _SAFEMEM_H_ */
