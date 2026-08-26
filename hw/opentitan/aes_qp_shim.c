/*
 * QEMU OpenTitan AES device — qemu-passes shim (auto-generated)
 *
 * DO NOT EDIT.  Re-emit by re-running qemu-passes with --qemu-emit-c
 * after changing the kKnownDevices catalog in lib/QEMUEmitCPass.cpp.
 *
 * Frontend wrapper around the auto-generated AES model from
 *   hw/opentitan/qemu_passes/aes.{c,h}
 *
 * MMIO read/write callbacks delegate to the generated entrypoints
 * aes_read() / aes_write().  The QEMU backend
 * interface (chardev, IRQ wires, ptimer, ...) is intentionally NOT wired
 * here — frontend-only milestone.  Backend hookup is a later milestone.
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qapi/error.h"
#include "chardev/char-fe.h"
#include "hw/opentitan/ot_key_sink.h"
#include "hw/sysbus.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-properties-system.h"
#include "hw/irq.h"
#include "hw/opentitan/ot_alert.h"
#include "hw/opentitan/ot_common.h"
#include "hw/riscv/ibex_irq.h"

/* Generated aes model entrypoints + state struct. */
#include "qemu_passes/aes.h"

/* This shim's own type declaration. */
#include "hw/opentitan/aes_qp_shim.h"

#define OT_AES_QP_IRQ_NUM 0u

struct OtAesQpState {
    SysBusDevice parent_obj;
    MemoryRegion mmio;
    IbexIRQ irqs[OT_AES_QP_IRQ_NUM];
    IbexIRQ alert;

    /* Property fields (mirror upstream ot_aes_eg.c so SoC-table-injected
     * values are accepted by QOM).  We accept-but-ignore: the
     * generated model has its own initial state and no chardev wiring. */
    char * ot_id;

    /* Embedded auto-generated device state. */
    aes_state core;
};

OBJECT_DEFINE_SIMPLE_TYPE_WITH_INTERFACES(OtAesQpState, ot_aes_qp,
                                          OT_AES_QP, SYS_BUS_DEVICE,
                                          { TYPE_OT_KEY_SINK_IF },
                                          { NULL })

static uint64_t ot_aes_qp_read(void *opaque, hwaddr addr, unsigned size)
{
    OtAesQpState *s = OT_AES_QP(opaque);
    return aes_read(&s->core, addr, size);
}

static void ot_aes_qp_write(void *opaque, hwaddr addr, uint64_t value,
                            unsigned size)
{
    OtAesQpState *s = OT_AES_QP(opaque);
    aes_write(&s->core, addr, value, size);
}

static const MemoryRegionOps ot_aes_qp_ops = {
    .read = ot_aes_qp_read,
    .write = ot_aes_qp_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
};

static const Property ot_aes_qp_properties[] = {
    DEFINE_PROP_STRING(OT_COMMON_DEV_ID, OtAesQpState, ot_id),
};

static void ot_aes_qp_realize(DeviceState *dev, Error **errp)
{
    /* Backend hookup intentionally absent — frontend-only milestone. */
    (void)dev;
    (void)errp;
}

static void ot_aes_qp_init(Object *obj)
{
    OtAesQpState *s = OT_AES_QP(obj);

    /* This device has 0 IRQs — no IRQ init loop. */
    ibex_qdev_init_irq(obj, &s->alert, OT_DEVICE_ALERT);

    memory_region_init_io(&s->mmio, obj, &ot_aes_qp_ops, s,
                          TYPE_OT_AES_QP, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    /* Binding-declared input ties: constants on top-level inputs the
     * SoC model does not integrate (e.g. an always-acking EDN
     * entropy bus, a lifecycle-escalate line held at Off) so
     * handshake-waiting FSMs are not deadlocked.  Applied BEFORE
     * the reset pulse as well: sparse security FSMs sample these
     * on the reset-release tick and latch a terminal error state
     * if a tied line still reads its (illegal) zero init. */
    aes_set_edn_i_edn_ack(&s->core, 0x1u);
    aes_set_edn_i_edn_bus(&s->core, 0xAAAAAAAAu);
    aes_set_edn_i_edn_fips(&s->core, 0x1u);
    aes_set_lc_escalate_en_i(&s->core, 0xAu);
    /* Pulse reset to commit RESVALs into every prim_subreg.
     * Pattern in generated code:  q = (~rst_ni) ? RESVAL : keep.
     * C-init leaves rst_ni at 0, so a settle round with rst
     * active forces q := RESVAL across every register.  Then
     * release reset by setting rst_ni high.  Without this pulse,
     * registers like REGWEN (RESVAL=1) stay at their C init=0. */
    aes_reset(&s->core);
    aes_set_edn_i_edn_ack(&s->core, 0x1u);
    aes_set_edn_i_edn_bus(&s->core, 0xAAAAAAAAu);
    aes_set_edn_i_edn_fips(&s->core, 0x1u);
    aes_set_lc_escalate_en_i(&s->core, 0xAu);
    aes_settle(&s->core);
}

/* keymgr sideload sink: accept the pushed key silently — sideload
 * key material is not modeled (the sink link must exist or the
 * keymgr asserts at boot). */
static void ot_aes_qp_push_key(OtKeySinkIf *ifd, const uint8_t *share0,
              const uint8_t *share1, size_t key_len, bool valid)
{
    (void)ifd; (void)share0; (void)share1; (void)key_len; (void)valid;
}

static void ot_aes_qp_finalize(Object *obj)
{
    (void)obj;
}

static void ot_aes_qp_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    (void)data;

    dc->realize = ot_aes_qp_realize;
    device_class_set_props(dc, ot_aes_qp_properties);
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);

    OtKeySinkIfClass *kc = OT_KEY_SINK_IF_CLASS(klass);
    kc->push_key = ot_aes_qp_push_key;
}
