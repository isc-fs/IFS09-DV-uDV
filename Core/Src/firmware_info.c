/**
 * @file    firmware_info.c
 * @brief   The bootloader's firmware-info record (GET_FW_INFO).
 *
 * isc-fs/stm32-can-bootloader reads a 64-byte record at a fixed address,
 * BL_APP_BASE + 0x400 = 0x08020400 (Core/Inc/bl_fwinfo.h), to answer
 * GET_FW_INFO: who this image is, which version, which node. The linker
 * script pins the `.firmware_info` section there. It is app-owned and
 * optional for the jump itself: the bootloader validates the app by the
 * metadata FLASHWORD at 0x080FFFE0, which it stamps itself after a verified
 * flash (ARCHITECTURE.md "Application metadata record").
 *
 * The layout is copied from bl_fwinfo.h, as the ECU and the AMS do (the
 * bootloader repo is not a submodule here). Keep the magic and the record
 * version in lockstep with it.
 */
#include <stdint.h>

/* Build id injected by the Makefile (-DGIT_HASH=0x<8 hex>); 0 if unset. */
#ifndef GIT_HASH
#define GIT_HASH 0u
#endif

typedef struct {
    uint32_t magic;             /* BL_FWINFO_MAGIC */
    uint32_t record_version;    /* major << 16 | minor */
    uint32_t fw_version_major;
    uint32_t fw_version_minor;
    uint32_t fw_version_patch;
    uint32_t mcu_id;            /* DBGMCU IDCODE DEV_ID */
    uint8_t  git_hash[8];
    uint64_t build_timestamp;
    char     product_name[16];  /* ASCII, NUL-padded */
    uint32_t reserved[2];       /* [0]: bootloader node id (as the AMS/ECU) */
} bl_fwinfo_t;

_Static_assert(sizeof(bl_fwinfo_t) == 64u, "bl_fwinfo_t must be exactly 64 bytes");

const bl_fwinfo_t __firmware_info __attribute__((section(".firmware_info"), used)) = {
    .magic            = 0xF14F1B00u,   /* BL_FWINFO_MAGIC */
    .record_version   = 0x00010000u,   /* BL_FWINFO_RECORD_VERSION 1.0 */
    /* The uDV has no release numbering yet: 0.0.0, identified by git_hash. */
    .fw_version_major = 0u,
    .fw_version_minor = 0u,
    .fw_version_patch = 0u,
    .mcu_id           = 0x00000483u,   /* STM32H72x/73x DEV_ID (RM0468 DBGMCU_IDC) */
    /* The 8-hex-digit short hash, most significant byte first, so the bytes
     * read as the hash does; the last four stay 0. */
    .git_hash         = { (uint8_t)((uint32_t)GIT_HASH >> 24), (uint8_t)((uint32_t)GIT_HASH >> 16),
                          (uint8_t)((uint32_t)GIT_HASH >> 8),  (uint8_t)((uint32_t)GIT_HASH),
                          0u, 0u, 0u, 0u },
    .build_timestamp  = 0u,
    .product_name     = "IFS09-DV-uDV",
    .reserved         = { 0x3u /* the uDV's node id */, 0u },
};
