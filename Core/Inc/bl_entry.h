/**
 * @file    bl_entry.h
 * @brief   Reboot from the running app into the CAN bootloader (app -> BL).
 *
 * The uDV runs behind isc-fs/stm32-can-bootloader, which every MainLite
 * carries in flash sector 0. The bootloader reads RTC->BKP0R on every reset:
 * BL_BOOT_REQ_MAGIC there makes it clear the word (one-shot) and stay in its
 * CAN listen loop, waiting for a flash, instead of jumping to the app
 * (bootloader ARCHITECTURE.md, "Bootloader-request handshake (app -> BL)").
 *
 * The trigger is one classic frame on the ACU bus (FDCAN2), the bus the uDV is
 * flashed over (node 0x3): id 0x002, dlc 4, payload B0 07 AD 13. The id is the
 * one the AMS (payload ..AD 11, node 0x2) and the ECU (..AD 12, node 0x1)
 * listen on; the last byte selects the board, so one frame reboots one node
 * on the shared bus (can-flasher src/app_control.rs).
 *
 * Gate: honoured only in AS OFF. Resetting the uDV drops it into the
 * bootloader, whose pins sit at their power-on level: EBS fires and the SDC
 * opens. That is the fail-safe state, but a 4-byte frame anyone on the bus can
 * send must not fire the EBS of a car in READY or DRIVING, nor cut the
 * EMERGENCY / FINISHED indication short. EMERGENCY and FINISHED latch until
 * ASMS off, which returns the car to OFF, where reflashing is allowed.
 *
 * The predicates are pure static inline functions so the host test suite
 * (tests/host/test_bl_entry.cpp) exercises them without HAL or RTOS.
 */
#ifndef BL_ENTRY_H
#define BL_ENTRY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* RTC->BKP0R value that keeps the bootloader in CAN-listen mode (BL
 * Core/Inc/bl_memmap.h BL_BOOT_REQ_MAGIC). */
#define BL_BOOT_REQ_MAGIC        0xB00710ADu

/* The uDV's node id on the bootloader bus (provisioned per board). */
#define BL_ENTRY_NODE_ID         0x3u

/* Trigger frame: ACU bus (FDCAN2), id 0x002, dlc 4, B0 07 AD 13. */
#define BL_ENTRY_CAN_BUS         2u
#define BL_ENTRY_CAN_ID          0x002u
#define BL_ENTRY_DLC             4u
#define BL_ENTRY_PAYLOAD_0       0xB0u
#define BL_ENTRY_PAYLOAD_1       0x07u
#define BL_ENTRY_PAYLOAD_2       0xADu
#define BL_ENTRY_PAYLOAD_3       (0x10u | BL_ENTRY_NODE_ID)   /* 0x13 */

/* Raw AS state values (as_state.h): OFF=0. */
#define BL_ENTRY_AS_OFF          0u

/* True iff a received frame is this node's reboot-to-bootloader trigger. */
static inline bool bl_entry_is_trigger(uint8_t bus, uint32_t id, uint8_t dlc,
                                       const uint8_t *data)
{
    return bus == BL_ENTRY_CAN_BUS && id == BL_ENTRY_CAN_ID &&
           dlc == BL_ENTRY_DLC && data != NULL &&
           data[0] == BL_ENTRY_PAYLOAD_0 && data[1] == BL_ENTRY_PAYLOAD_1 &&
           data[2] == BL_ENTRY_PAYLOAD_2 && data[3] == BL_ENTRY_PAYLOAD_3;
}

/* True iff the AS state allows a reboot into the bootloader (OFF only). */
static inline bool bl_entry_allowed(uint8_t as_state)
{
    return as_state == BL_ENTRY_AS_OFF;
}

/* Let in-flight TX clear, write BL_BOOT_REQ_MAGIC to RTC->BKP0R and reset.
 * Never returns. Task context only (it sleeps). Defined in bl_entry.c. */
void bl_entry_reboot_to_bootloader(void) __attribute__((noreturn));

#ifdef __cplusplus
}
#endif

#endif /* BL_ENTRY_H */
