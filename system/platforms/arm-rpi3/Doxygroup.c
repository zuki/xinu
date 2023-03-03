/**
 * @defgroup raspi3b Raspberry Pi 3 Model B+
 * @ingroup system
 *
 * @defgroup bcm2837
 * @ingroup raspi3b
 * @brief Raspberry Pi 3 Model B+のSOCはBroadcom BCM2837B0である.
 *
 * Embedded Xinuのポートの多くはBCM2837B0用に再設計されている
 * （ブートローダ、マルチコア機能、L1キャッシュなど）が、車輪全体を
 * 再発明する必要はないのでこのOSには前回のポート（BCM2835 Raspberry
 * Pi 1）の名残が残っている。
 *
 * @defgroup usbhcd
 * @ingroup raspi3b
 * @brief USBホストコントローラドライバはUSB転送プロトコルを規定する.
 */
