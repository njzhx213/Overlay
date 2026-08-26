/*
 * QEMU OpenTitan HMAC device — qemu-passes shim (auto-generated)
 *
 * DO NOT EDIT.  Re-emit by re-running qemu-passes with --qemu-emit-c
 * after changing the kKnownDevices catalog in lib/QEMUEmitCPass.cpp.
 *
 * Frontend wrapper around the auto-generated HMAC model from
 *   hw/opentitan/qemu_passes/hmac.{c,h}
 *
 * MMIO read/write callbacks delegate to the generated entrypoints
 * hmac_read() / hmac_write().  The QEMU backend
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

/* Generated hmac model entrypoints + state struct. */
#include "qemu_passes/hmac.h"

/* This shim's own type declaration. */
#include "hw/opentitan/hmac_qp_shim.h"

#define OT_HMAC_QP_IRQ_NUM 3u

struct OtHmacQpState {
    SysBusDevice parent_obj;
    MemoryRegion mmio;
    IbexIRQ irqs[OT_HMAC_QP_IRQ_NUM];
    IbexIRQ alert;

    /* Property fields (mirror upstream ot_hmac_eg.c so SoC-table-injected
     * values are accepted by QOM).  We accept-but-ignore: the
     * generated model has its own initial state and no chardev wiring. */
    char * ot_id;

    /* Embedded auto-generated device state. */
    hmac_state core;
};

OBJECT_DEFINE_SIMPLE_TYPE(OtHmacQpState, ot_hmac_qp, OT_HMAC_QP, SYS_BUS_DEVICE)

static uint64_t ot_hmac_qp_read(void *opaque, hwaddr addr, unsigned size)
{
    OtHmacQpState *s = OT_HMAC_QP(opaque);
    return hmac_read(&s->core, addr, size);
}

static void ot_hmac_qp_write(void *opaque, hwaddr addr, uint64_t value,
                             unsigned size)
{
    OtHmacQpState *s = OT_HMAC_QP(opaque);
    hmac_write(&s->core, addr, value, size);
}

static const MemoryRegionOps ot_hmac_qp_ops = {
    .read = ot_hmac_qp_read,
    .write = ot_hmac_qp_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
};

static const Property ot_hmac_qp_properties[] = {
    DEFINE_PROP_STRING(OT_COMMON_DEV_ID, OtHmacQpState, ot_id),
};

static void ot_hmac_qp_realize(DeviceState *dev, Error **errp)
{
    /* Backend hookup intentionally absent — frontend-only milestone. */
    (void)dev;
    (void)errp;
}

static void ot_hmac_qp_init(Object *obj)
{
    OtHmacQpState *s = OT_HMAC_QP(obj);

    for (unsigned i = 0; i < OT_HMAC_QP_IRQ_NUM; i++) {
        ibex_sysbus_init_irq(obj, &s->irqs[i]);
    }
    ibex_qdev_init_irq(obj, &s->alert, OT_DEVICE_ALERT);

    memory_region_init_io(&s->mmio, obj, &ot_hmac_qp_ops, s,
                          TYPE_OT_HMAC_QP, 0x2000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    /* Pulse reset to commit RESVALs into every prim_subreg.
     * Pattern in generated code:  q = (~rst_ni) ? RESVAL : keep.
     * C-init leaves rst_ni at 0, so a settle round with rst
     * active forces q := RESVAL across every register.  Then
     * release reset by setting rst_ni high.  Without this pulse,
     * registers like REGWEN (RESVAL=1) stay at their C init=0. */
    hmac_reset(&s->core);
}

static void ot_hmac_qp_finalize(Object *obj)
{
    (void)obj;
}

static void ot_hmac_qp_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    (void)data;

    dc->realize = ot_hmac_qp_realize;
    device_class_set_props(dc, ot_hmac_qp_properties);
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}
