/**
 * @file dma_buf.c
 */
#include <dma_buf.h>
#include <compiler.h>
#include <mutex.h>
#include <platform.h>

/** dma_buf用のメモリ容量 (1MB) */
#define SECTION_SIZE    0x00100000

/** 未使用バイトの先頭アドレス */
static int freespace_idx = 0;
/** 未使用バイト数 */
static int remaining_space = SECTION_SIZE;
/** 上の2つの変数を守るmutex */
static mutex_t dma_buf_mutex;

uint8_t dma_buf_space[SECTION_SIZE] __aligned(SECTION_SIZE);

/**
 * dma_bufを割り当てる.
 *
 * @param size 割り当てるサイズ
 * @return 割り当てたdma_bufへのポインタ
 */
void *dma_buf_alloc(uint size)
{
    void *retval;
    retval = (void *)(dma_buf_space + freespace_idx);

    mutex_acquire(dma_buf_mutex);
    freespace_idx += size;
    remaining_space -= size;
    mutex_release(dma_buf_mutex);

    return retval;
}

/**
 * dma_bufを解放する.
 *
 * @param ptr dma_bufへのポインタ
 * @param size 解放するサイズ
 * @return 常にOK
*/
syscall dma_buf_free(void *ptr, uint size)
{
    mutex_acquire(dma_buf_mutex);
    freespace_idx -= size;
    remaining_space += size;
    mutex_release(dma_buf_mutex);

    return OK;
}

/**
 * dma_bufを初期化する.
 *
 * @return 常にOK
*/
syscall dma_buf_init()
{
    freespace_idx = 0;
    remaining_space = SECTION_SIZE;
    dma_buf_mutex = mutex_create();

    return OK;
}
