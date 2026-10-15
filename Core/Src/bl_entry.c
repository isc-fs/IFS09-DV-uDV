/**
 * @file    bl_entry.c
 * @brief   App -> CAN bootloader reboot (see bl_entry.h for the contract).
 */
#include "bl_entry.h"

#include "cmsis_os.h"
#include "main.h"

void bl_entry_reboot_to_bootloader(void)
{
    /* 1) Let in-flight TX (the last 0x507 / 0x512, the pit-diag stream) clear
     * the FDCAN controllers. 10 ms is plenty at 500 kbit/s: an 8-byte classic
     * frame is under 250 us on the wire. Same as the ECU and the AMS. */
    osDelay(10);

    /* 2) Hand the bootloader its boot-request magic. The backup registers are
     * write-protected until PWR_CR1.DBP is set; the RTC's APB clock is on from
     * reset (RCC_APB4ENR.RTCAPBEN resets to 1, RM0468). */
    HAL_PWR_EnableBkUpAccess();
    RTC->BKP0R = BL_BOOT_REQ_MAGIC;
    __DSB();

    /* 3) System reset. The bootloader reads BKP0R, clears it (one-shot) and
     * stays in CAN-listen mode for the flasher. */
    NVIC_SystemReset();

    for (;;) {
    }
}
