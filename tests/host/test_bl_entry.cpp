/**
 * @file test_bl_entry.cpp
 * @brief Host unit tests for the reboot-to-bootloader trigger predicates
 *        (Core/Inc/bl_entry.h).
 *
 * Pins the two halves of the gate:
 *   - TRIGGER: only id 0x002, dlc 4, payload B0 07 AD 13, on the ACU bus
 *     (FDCAN2) is the uDV's trigger. The AMS (..AD 11) and ECU (..AD 12)
 *     payloads share the id on the same wire and must be ignored, and so must
 *     the frame on FDCAN3, whose filter accepts every id.
 *   - STATE: honoured in AS OFF only; READY/DRIVING/EMERGENCY/FINISHED refuse.
 *
 * Build + run:  make test_bl_entry
 */
#include <cstdio>
#include <cstdint>
#include "bl_entry.h"

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond)                                                           \
    do {                                                                      \
        ++g_checks;                                                           \
        if (!(cond)) {                                                        \
            ++g_failures;                                                     \
            std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);       \
        }                                                                     \
    } while (0)

static void test_trigger_matches_only_udv_payload_on_acu_bus()
{
    const uint8_t udv[8] = {0xB0, 0x07, 0xAD, 0x13, 0, 0, 0, 0};
    const uint8_t ams[8] = {0xB0, 0x07, 0xAD, 0x11, 0, 0, 0, 0};
    const uint8_t ecu[8] = {0xB0, 0x07, 0xAD, 0x12, 0, 0, 0, 0};

    CHECK(bl_entry_is_trigger(2, 0x002, 4, udv));
    /* The other nodes' payloads on the same id and wire. */
    CHECK(!bl_entry_is_trigger(2, 0x002, 4, ams));
    CHECK(!bl_entry_is_trigger(2, 0x002, 4, ecu));
    /* Wrong bus: FDCAN1 (RES) and FDCAN3 (accept-all filter). */
    CHECK(!bl_entry_is_trigger(1, 0x002, 4, udv));
    CHECK(!bl_entry_is_trigger(3, 0x002, 4, udv));
    /* Wrong id, wrong dlc, no data. */
    CHECK(!bl_entry_is_trigger(2, 0x003, 4, udv));
    CHECK(!bl_entry_is_trigger(2, 0x000, 4, udv));
    CHECK(!bl_entry_is_trigger(2, 0x002, 3, udv));
    CHECK(!bl_entry_is_trigger(2, 0x002, 8, udv));
    CHECK(!bl_entry_is_trigger(2, 0x002, 4, nullptr));
    /* Every single-byte corruption of the payload is rejected. */
    for (int i = 0; i < 4; ++i) {
        uint8_t bad[8];
        for (int j = 0; j < 8; ++j) bad[j] = udv[j];
        bad[i] ^= 0x01;
        CHECK(!bl_entry_is_trigger(2, 0x002, 4, bad));
    }
}

static void test_payload_encodes_node_id()
{
    CHECK(BL_ENTRY_NODE_ID == 0x3u);
    CHECK(BL_ENTRY_PAYLOAD_3 == 0x13u);
    CHECK(BL_BOOT_REQ_MAGIC == 0xB00710ADu);
}

static void test_allowed_in_as_off_only()
{
    /* Raw ASState (as_state.h): OFF=0 READY=1 DRIVING=2 EMERGENCY=3 FINISHED=4 */
    CHECK(bl_entry_allowed(0));
    for (unsigned s = 1; s < 256; ++s) {
        CHECK(!bl_entry_allowed((uint8_t)s));
    }
}

int main()
{
    test_trigger_matches_only_udv_payload_on_acu_bus();
    test_payload_encodes_node_id();
    test_allowed_in_as_off_only();
    std::printf("test_bl_entry: %d checks, %d failures\n", g_checks, g_failures);
    return g_failures ? 1 : 0;
}
