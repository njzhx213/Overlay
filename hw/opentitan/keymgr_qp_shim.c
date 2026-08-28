/*
 * QEMU OpenTitan KEYMGR device — qemu-passes shim (auto-generated)
 *
 * DO NOT EDIT.  Re-emit by re-running qemu-passes with --qemu-emit-c
 * after changing the kKnownDevices catalog in lib/QEMUEmitCPass.cpp.
 *
 * Frontend wrapper around the auto-generated KEYMGR model from
 *   hw/opentitan/qemu_passes/keymgr.{c,h}
 *
 * MMIO read/write callbacks delegate to the generated entrypoints
 * keymgr_read() / keymgr_write().  The QEMU backend
 * interface (chardev, IRQ wires, ptimer, ...) is intentionally NOT wired
 * here — frontend-only milestone.  Backend hookup is a later milestone.
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qapi/error.h"
#include "chardev/char-fe.h"
#include "hw/sysbus.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-properties-system.h"
#include "hw/irq.h"
#include "hw/opentitan/ot_alert.h"
#include "hw/opentitan/ot_common.h"
#include "hw/riscv/ibex_irq.h"

/* Generated keymgr model entrypoints + state struct. */
#include "qemu_passes/keymgr.h"

/* This shim's own type declaration. */
#include "hw/opentitan/keymgr_qp_shim.h"

#define OT_KEYMGR_QP_IRQ_NUM 0u

struct OtKeymgrQpState {
    SysBusDevice parent_obj;
    MemoryRegion mmio;
    IbexIRQ irqs[OT_KEYMGR_QP_IRQ_NUM];
    IbexIRQ alert;

    /* Property fields (mirror upstream ot_keymgr_eg.c so SoC-table-injected
     * values are accepted by QOM).  We accept-but-ignore: the
     * generated model has its own initial state and no chardev wiring. */
    char * ot_id;

    /* Embedded auto-generated device state. */
    keymgr_state core;
};

OBJECT_DEFINE_SIMPLE_TYPE(OtKeymgrQpState, ot_keymgr_qp, OT_KEYMGR_QP, SYS_BUS_DEVICE)

/* Generic core accessor: SoC-integration bridges (device-to-device
 * signal links wired in the machine file) reach the generated state
 * through this + the <dev>.h field API. */
void *ot_keymgr_qp_core(DeviceState *dev)
{
    return &OT_KEYMGR_QP(dev)->core;
}

static uint64_t ot_keymgr_qp_read(void *opaque, hwaddr addr, unsigned size)
{
    OtKeymgrQpState *s = OT_KEYMGR_QP(opaque);
    return keymgr_read(&s->core, addr, size);
}

static void ot_keymgr_qp_write(void *opaque, hwaddr addr, uint64_t value,
                               unsigned size)
{
    OtKeymgrQpState *s = OT_KEYMGR_QP(opaque);
    keymgr_write(&s->core, addr, value, size);
}

static const MemoryRegionOps ot_keymgr_qp_ops = {
    .read = ot_keymgr_qp_read,
    .write = ot_keymgr_qp_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
};

static const Property ot_keymgr_qp_properties[] = {
    DEFINE_PROP_STRING(OT_COMMON_DEV_ID, OtKeymgrQpState, ot_id),
};

static void ot_keymgr_qp_realize(DeviceState *dev, Error **errp)
{
    /* Backend hookup intentionally absent — frontend-only milestone. */
    (void)dev;
    (void)errp;
}

static void ot_keymgr_qp_init(Object *obj)
{
    OtKeymgrQpState *s = OT_KEYMGR_QP(obj);

    /* This device has 0 IRQs — no IRQ init loop. */
    ibex_qdev_init_irq(obj, &s->alert, OT_DEVICE_ALERT);

    memory_region_init_io(&s->mmio, obj, &ot_keymgr_qp_ops, s,
                          TYPE_OT_KEYMGR_QP, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    /* Binding-declared input ties: constants on top-level inputs the
     * SoC model does not integrate (e.g. an always-acking EDN
     * entropy bus, a lifecycle-escalate line held at Off) so
     * handshake-waiting FSMs are not deadlocked.  Applied BEFORE
     * the reset pulse as well: sparse security FSMs sample these
     * on the reset-release tick and latch a terminal error state
     * if a tied line still reads its (illegal) zero init. */
    keymgr_set_edn_i_edn_ack(&s->core, 0x1u);
    keymgr_set_edn_i_edn_bus(&s->core, 0xAAAAAAAAu);
    keymgr_set_edn_i_edn_fips(&s->core, 0x1u);
    keymgr_set_kmac_en_masking_i(&s->core, 0x0u);
    keymgr_set_lc_keymgr_en_i(&s->core, 0x5u);
    keymgr_set_otp_key_i_creator_root_key_share0_valid(&s->core, 0x1u);
    keymgr_set_otp_key_i_creator_root_key_share1_valid(&s->core, 0x1u);
    keymgr_set_otp_key_i_creator_seed_valid(&s->core, 0x1u);
    keymgr_set_otp_key_i_owner_seed_valid(&s->core, 0x1u);
    keymgr_set_rom_digest_i_valid(&s->core, 0x1u);
    /* Pulse reset to commit RESVALs into every prim_subreg.
     * Pattern in generated code:  q = (~rst_ni) ? RESVAL : keep.
     * C-init leaves rst_ni at 0, so a settle round with rst
     * active forces q := RESVAL across every register.  Then
     * release reset by setting rst_ni high.  Without this pulse,
     * registers like REGWEN (RESVAL=1) stay at their C init=0. */
    keymgr_reset(&s->core);
    keymgr_set_edn_i_edn_ack(&s->core, 0x1u);
    keymgr_set_edn_i_edn_bus(&s->core, 0xAAAAAAAAu);
    keymgr_set_edn_i_edn_fips(&s->core, 0x1u);
    keymgr_set_kmac_en_masking_i(&s->core, 0x0u);
    keymgr_set_lc_keymgr_en_i(&s->core, 0x5u);
    keymgr_set_otp_key_i_creator_root_key_share0_valid(&s->core, 0x1u);
    keymgr_set_otp_key_i_creator_root_key_share1_valid(&s->core, 0x1u);
    keymgr_set_otp_key_i_creator_seed_valid(&s->core, 0x1u);
    keymgr_set_otp_key_i_owner_seed_valid(&s->core, 0x1u);
    keymgr_set_rom_digest_i_valid(&s->core, 0x1u);
    keymgr_settle(&s->core);
}

static void ot_keymgr_qp_finalize(Object *obj)
{
    (void)obj;
}

static void ot_keymgr_qp_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    (void)data;

    dc->realize = ot_keymgr_qp_realize;
    device_class_set_props(dc, ot_keymgr_qp_properties);
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}
