/*
 * QEMU OpenTitan ROM_CTRL heart-swap boot front-end (qemu-passes).
 *
 * Serves the CPU-fetch ROM window (cleartext rom_device) and drives the
 * pwrmgr done/good handshake from the GENERATED rom_ctrl model's digest
 * verdict.  See ot_rom_ctrl_qp_boot.c for the full story.
 */
#ifndef HW_OPENTITAN_ROM_CTRL_QP_BOOT_H
#define HW_OPENTITAN_ROM_CTRL_QP_BOOT_H

#include "qom/object.h"
#include "hw/sysbus.h"

#define TYPE_OT_ROM_CTRL_QP_BOOT "ot-rom-ctrl-qp-boot"
OBJECT_DECLARE_SIMPLE_TYPE(OtRomCtrlQpBootState, OT_ROM_CTRL_QP_BOOT)

/* Machine reset-exit entry: load the ROM image, run the generated
 * rom_ctrl x kmac check, seal the ROM window, notify the pwrmgr. */
void ot_rom_ctrl_qp_boot_run(DeviceState *dev);

#endif /* HW_OPENTITAN_ROM_CTRL_QP_BOOT_H */
