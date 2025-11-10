#ifndef INC_MBOX_H
#define INC_MBOX_H

#include <stdint.h>

#define MBOX_CLOCK_EMMC     0x1
#define MBOX_CLOCK_UART     0x2
#define MBOX_CLOCK_ARM      0x3
#define MBOX_CLOCK_CORE     0x4
#define MBOX_CLOCK_EMMC2    0xC

#define MBOX_DEVICE_SDCARD  0x0
#define MBOX_DEVICE_UART0   0x1
#define MBOX_DEVICE_UART1   0x2

#define MBOX_EXP_GPIO_BASE  128
#define MBOX_EXP_GPIO_NUM   8

int mbox_get_arm_memory(void);
int mbox_get_clock_rate(int clock_id);
int mbox_set_gpio_state(uint32_t npgio, uint32_t state);
int mbox_set_power_state(uint32_t devid, bool on, bool wait);
int mbox_get_macaddr(uint8_t *address);
int mbox_get_serial(uint32_t *low, uint32_t *high);
int mbox_get_power_mask(void);
void mbox_power_init(void);

#endif
