/**
 * @file random.c
 *
 * 乱数関連の関数を提供する.
 */
#include <random.h>

/**
 * 乱数を初期化する.
 */
void random_init(void)
{
    *RNG_STATUS = 0x40000;
    *RNG_INT_MASK |= 1;            // mask interupts
    *RNG_CTRL |= 1;                // enable
    // wait for it to gain entropy
    while(!((*RNG_STATUS)>>24)) asm volatile("nop");
}

/**
 * 乱数を発生させる.
 *
 * @return 乱数
 */
unsigned int random(void)
{
//    return *RNG_DATA % (max - min) + min;
    return (unsigned int) *RNG_DATA;
}
