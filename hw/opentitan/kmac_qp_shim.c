/*
 * QEMU OpenTitan KMAC device — qemu-passes shim (auto-generated)
 *
 * DO NOT EDIT.  Re-emit by re-running qemu-passes with --qemu-emit-c
 * after changing the kKnownDevices catalog in lib/QEMUEmitCPass.cpp.
 *
 * Frontend wrapper around the auto-generated KMAC model from
 *   hw/opentitan/qemu_passes/kmac.{c,h}
 *
 * MMIO read/write callbacks delegate to the generated entrypoints
 * kmac_read() / kmac_write().  The QEMU backend
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

/* Generated kmac model entrypoints + state struct. */
#include "qemu_passes/kmac.h"

/* This shim's own type declaration. */
#include "hw/opentitan/kmac_qp_shim.h"

#define OT_KMAC_QP_IRQ_NUM 3u

struct OtKmacQpState {
    SysBusDevice parent_obj;
    MemoryRegion mmio;
    IbexIRQ irqs[OT_KMAC_QP_IRQ_NUM];
    IbexIRQ alert;

    /* Property fields (mirror upstream ot_kmac_eg.c so SoC-table-injected
     * values are accepted by QOM).  We accept-but-ignore: the
     * generated model has its own initial state and no chardev wiring. */
    char * ot_id;

    /* Embedded auto-generated device state. */
    kmac_state core;
};

OBJECT_DEFINE_SIMPLE_TYPE(OtKmacQpState, ot_kmac_qp, OT_KMAC_QP, SYS_BUS_DEVICE)

static uint64_t ot_kmac_qp_read(void *opaque, hwaddr addr, unsigned size)
{
    OtKmacQpState *s = OT_KMAC_QP(opaque);
    return kmac_read(&s->core, addr, size);
}

static void ot_kmac_qp_write(void *opaque, hwaddr addr, uint64_t value,
                             unsigned size)
{
    OtKmacQpState *s = OT_KMAC_QP(opaque);
    kmac_write(&s->core, addr, value, size);
}

static const MemoryRegionOps ot_kmac_qp_ops = {
    .read = ot_kmac_qp_read,
    .write = ot_kmac_qp_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
};

static const Property ot_kmac_qp_properties[] = {
    DEFINE_PROP_STRING(OT_COMMON_DEV_ID, OtKmacQpState, ot_id),
};

static void ot_kmac_qp_realize(DeviceState *dev, Error **errp)
{
    /* Backend hookup intentionally absent — frontend-only milestone. */
    (void)dev;
    (void)errp;
}

static void ot_kmac_qp_init(Object *obj)
{
    OtKmacQpState *s = OT_KMAC_QP(obj);

    for (unsigned i = 0; i < OT_KMAC_QP_IRQ_NUM; i++) {
        ibex_sysbus_init_irq(obj, &s->irqs[i]);
    }
    ibex_qdev_init_irq(obj, &s->alert, OT_DEVICE_ALERT);

    memory_region_init_io(&s->mmio, obj, &ot_kmac_qp_ops, s,
                          TYPE_OT_KMAC_QP, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    /* Binding-declared input ties: constants on top-level inputs the
     * SoC model does not integrate (e.g. an always-acking EDN
     * entropy bus, a lifecycle-escalate line held at Off) so
     * handshake-waiting FSMs are not deadlocked.  Applied BEFORE
     * the reset pulse as well: sparse security FSMs sample these
     * on the reset-release tick and latch a terminal error state
     * if a tied line still reads its (illegal) zero init. */
    kmac_set_entropy_i_edn_ack(&s->core, 0x1u);
    kmac_set_entropy_i_edn_bus(&s->core, 0xAAAAAAAAu);
    kmac_set_entropy_i_edn_fips(&s->core, 0x1u);
    kmac_set_lc_escalate_en_i(&s->core, 0xAu);
    /* Pulse reset to commit RESVALs into every prim_subreg.
     * Pattern in generated code:  q = (~rst_ni) ? RESVAL : keep.
     * C-init leaves rst_ni at 0, so a settle round with rst
     * active forces q := RESVAL across every register.  Then
     * release reset by setting rst_ni high.  Without this pulse,
     * registers like REGWEN (RESVAL=1) stay at their C init=0. */
    kmac_reset(&s->core);
    kmac_set_entropy_i_edn_ack(&s->core, 0x1u);
    kmac_set_entropy_i_edn_bus(&s->core, 0xAAAAAAAAu);
    kmac_set_entropy_i_edn_fips(&s->core, 0x1u);
    kmac_set_lc_escalate_en_i(&s->core, 0xAu);
    kmac_settle(&s->core);
}

static void ot_kmac_qp_finalize(Object *obj)
{
    (void)obj;
}

static void ot_kmac_qp_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    (void)data;

    dc->realize = ot_kmac_qp_realize;
    device_class_set_props(dc, ot_kmac_qp_properties);
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}
