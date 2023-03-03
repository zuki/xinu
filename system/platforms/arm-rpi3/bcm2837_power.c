/**
 * @file bcm2837_power.c
 *
 * This driver provides the ability to power on and power off hardware, such as
 * the USB Controller, on the BCM2837B0 SoC used on the Raspberry Pi 3 B+.  This makes
 * use of the BCM2837B0's mailbox mechanism.
 */
#include "bcm2837.h"
#include "bcm2837_mbox.h"
#include "mmu.h"

/**
 * @ingroup bcm2837
 *
 * メールボックスの指定のチャンネルに書き出す.
 * @param channel 書き出すメールボックスチャンネル
 * @param value   メールボックスに書き出した値
 */
void
bcm2837_mailbox_write(uint channel, uint value)
{
    while (mailbox_regs[MAILBOX_STATUS] & MAILBOX_FULL)
    {
    }
    mailbox_regs[MAILBOX_WRITE] = (value & ~MAILBOX_CHANNEL_MASK) | channel;
}

/**
 * @ingroup bcm2837
 *
 * メールボックスの指定のチャンネルから読み込む.
 * @param channel 読み込むメールボックスチャンネル
 * @return        メールボックスから読み込んだ値
 */
uint
bcm2837_mailbox_read(uint channel)
{
    uint value;

    while (mailbox_regs[MAILBOX_STATUS] & MAILBOX_EMPTY)
    {
    }
    do
    {
        value = mailbox_regs[MAILBOX_READ];
    } while ((value & MAILBOX_CHANNEL_MASK) != channel);
    return (value & ~MAILBOX_CHANNEL_MASK);
}

/**
 * @ingroup bcm2837
 *
 * 電源オン/オフ状態のビットマスクを取得する.
 * @return BCM2837のメールボックス電源管理チャンネルに格納されている値
 */
static uint bcm2837_get_power_mask(void)
{
    return (bcm2837_mailbox_read(MAILBOX_CHANNEL_POWER_MGMT) >> 4);
}

/**
 * @ingroup bcm2837
 *
 * 電源オン/オフ状態のビットマスクをセットする.
 * @param mask	チャンネルに書き出すビットマスク
 */
static void
bcm2837_set_power_mask(uint mask)
{
    bcm2837_mailbox_write(MAILBOX_CHANNEL_POWER_MGMT, mask << 4);
}

/**
 * @ingroup bcm2837
 *
 * BCM2837ハードウェアの現在のオン/オフ状態を与えるビットマスク.
 * これはキャッスされる値である.
 */
static uint bcm2837_power_mask;

/**
 * @ingroup bcm2837
 *
 * BCM2837ハードウェアの電源をオン/オフする.
 *
 * @param feature	Device or hardware to power on or off.
 * @param on	::TRUE to power on; ::FALSE to power off.
 *
 * @return 成功の場合は ::OK; それ以外は ::SYSERR.
 */
int bcm2837_setpower(enum board_power_feature feature, bool on)
{
    uint bit;
    uint newmask;
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
 * BCM2837の電源をデフォルト状態（すべてのデバイスの
 * 電源をオフ）にリセットする.
 */
void bcm2837_power_init(void)
{
    bcm2837_set_power_mask(0);
    bcm2837_power_mask = 0;
}
