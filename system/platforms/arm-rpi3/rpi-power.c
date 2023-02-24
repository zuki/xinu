/**
 * @file bcm2837_power.c
 *
 * This driver provides the ability to power on and power off hardware, such as
 * the USB Controller, on the BCM2837B0 SoC used on the Raspberry Pi 3 B+.  This makes
 * use of the BCM2837B0's mailbox mechanism.
 */
#include <stdint.h>
#include "bcm2837.h"
#include "rpi-mailbox.h"

/**
 * @ingroup bcm2837
 *
 * 電源オン/オフ状態のビットマスクを取得する.
 * @return BCM2837のメールボック電源管理チャンネルが保持する値
 */
static uint bcm2837_get_power_mask(void)
{
    return (rpi_mailbox_read(MB_CHANNEL_POWER) >> 4);
}

/**
 * @ingroup bcm2837
 *
 * 電源オン/オフ状態のビットマスクをセットする.
 * @param mask	チャンネルに書き込むビットマスク
 */
static int
bcm2837_set_power_mask(uint32_t mask)
{
    return rpi_mailbox_write(MB_CHANNEL_POWER, mask << 4);
}

/**
 * @ingroup bcm2837
 *
 * BCM2837ハードウェアの電源オン/オフの現在の状態を与えるビットマスク.
 * これはキャッシュ値である.
 */
static uint32_t bcm2837_power_mask;

/**
 * @ingroup bcm2837
 *
 * BCM2837ハードウェアの電源をオン/オフする.
 *
 * @param feature   電源をオン/オフするデバイスまたはハードウェア.
 * @param on	電源オンの場合は ::TRUE; オフの場合は ::FALSE.
 *
 * @return 成功したら ::OK; それ以外は ::SYSERR.
 */
int bcm2837_setpower(enum board_power_feature feature, bool on)
{
    uint32_t bit;
    uint32_t newmask;
    bool is_on;

    bit = 1 << feature;
    is_on = (bcm2837_power_mask & bit) != 0;
    if (on != is_on)
    {
        newmask = bcm2837_power_mask ^ bit;
        bcm2837_set_power_mask(newmask);
        bcm2837_power_mask = bcm2837_get_power_mask();
        if (bcm2837_power_mask != newmask)
        {
            return SYSERR;
        }
    }
    return OK;
}

/**
 * @ingroup bcm2837
 *
 * BCM2837の電源をデフォルト状態（すべてのデバイスの電源オフ）にリセットする.
 */
void bcm2837_power_init(void)
{
    bcm2837_set_power_mask(0);
    bcm2837_power_mask = 0;
}
