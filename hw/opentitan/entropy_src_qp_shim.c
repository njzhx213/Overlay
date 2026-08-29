/*
 * QEMU OpenTitan ENTROPY_SRC device — qemu-passes shim (auto-generated)
 *
 * DO NOT EDIT.  Re-emit by re-running qemu-passes with --qemu-emit-c
 * after changing the kKnownDevices catalog in lib/QEMUEmitCPass.cpp.
 *
 * Frontend wrapper around the auto-generated ENTROPY_SRC model from
 *   hw/opentitan/qemu_passes/entropy_src.{c,h}
 *
 * MMIO read/write callbacks delegate to the generated entrypoints
 * entropy_src_read() / entropy_src_write().  The QEMU backend
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

/* Generated entropy_src model entrypoints + state struct. */
#include "qemu_passes/entropy_src.h"

/* This shim's own type declaration. */
#include "hw/opentitan/entropy_src_qp_shim.h"

#define OT_ENTROPY_SRC_QP_IRQ_NUM 4u

struct OtEntropySrcQpState {
    SysBusDevice parent_obj;
    MemoryRegion mmio;
    IbexIRQ irqs[OT_ENTROPY_SRC_QP_IRQ_NUM];
    IbexIRQ alert;

    /* Property fields (mirror upstream ot_entropy_src_eg.c so SoC-table-injected
     * values are accepted by QOM).  We accept-but-ignore: the
     * generated model has its own initial state and no chardev wiring. */
    char * ot_id;

    /* Embedded auto-generated device state. */
    entropy_src_state core;
};

OBJECT_DEFINE_SIMPLE_TYPE(OtEntropySrcQpState, ot_entropy_src_qp, OT_ENTROPY_SRC_QP, SYS_BUS_DEVICE)

/* === Interrupt delivery ========================================= */
static void ot_entropy_src_qp_update_irqs(OtEntropySrcQpState *s)
{
    /* The IRQ lines reflect the SETTLED device state: an external
     * stimulus (pin edge, chardev push, SPI byte) may have moved the
     * model only one clock (synchronizer / edge-detect / INTR_STATE
     * latch still propagating), and a bus read samples the register
     * on the request clock.  Let the model reach quiescence first —
     * cheap when already settled (one tick that changes nothing). */
    entropy_src_settle(&s->core);
    uint32_t intr_state  = (uint32_t)entropy_src_read(&s->core, 0x0u, 4);
    uint32_t intr_enable = (uint32_t)entropy_src_read(&s->core, 0x4u, 4);
    uint32_t masked = intr_state & intr_enable;
    for (unsigned i = 0; i < OT_ENTROPY_SRC_QP_IRQ_NUM; i++) {
        ibex_irq_set(&s->irqs[i], (int)((masked >> i) & 1u));
    }
}

/* Generic core accessor: SoC-integration bridges (device-to-device
 * signal links wired in the machine file) reach the generated state
 * through this + the <dev>.h field API. */
void *ot_entropy_src_qp_core(DeviceState *dev)
{
    return &OT_ENTROPY_SRC_QP(dev)->core;
}

void ot_entropy_src_qp_set_settle_hook(DeviceState *dev, int (*fn)(void *), void *ctx)
{
    OT_ENTROPY_SRC_QP(dev)->core._qp_settle_hook = fn;
    OT_ENTROPY_SRC_QP(dev)->core._qp_settle_hook_ctx = ctx;
}

static uint64_t ot_entropy_src_qp_read(void *opaque, hwaddr addr, unsigned size)
{
    OtEntropySrcQpState *s = OT_ENTROPY_SRC_QP(opaque);
    return entropy_src_read(&s->core, addr, size);
}

static void ot_entropy_src_qp_write(void *opaque, hwaddr addr, uint64_t value,
                                    unsigned size)
{
    OtEntropySrcQpState *s = OT_ENTROPY_SRC_QP(opaque);
    entropy_src_write(&s->core, addr, value, size);
    /* Alert line: a write to ALERT_TEST (IR-derived offset) pulses
     * the device alert — mirrors upstream ot_entropy_src R_ALERT_TEST. */
    if (addr == 0xCu) {
        ibex_irq_set(&s->alert, (int)(value & 1u));
    }
    ot_entropy_src_qp_update_irqs(s);
}

static const MemoryRegionOps ot_entropy_src_qp_ops = {
    .read = ot_entropy_src_qp_read,
    .write = ot_entropy_src_qp_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
};

static const Property ot_entropy_src_qp_properties[] = {
    DEFINE_PROP_STRING(OT_COMMON_DEV_ID, OtEntropySrcQpState, ot_id),
};

static void ot_entropy_src_qp_realize(DeviceState *dev, Error **errp)
{
    /* Backend hookup intentionally absent — frontend-only milestone. */
    (void)dev;
    (void)errp;
}

static void ot_entropy_src_qp_init(Object *obj)
{
    OtEntropySrcQpState *s = OT_ENTROPY_SRC_QP(obj);

    for (unsigned i = 0; i < OT_ENTROPY_SRC_QP_IRQ_NUM; i++) {
        ibex_sysbus_init_irq(obj, &s->irqs[i]);
    }
    ibex_qdev_init_irq(obj, &s->alert, OT_DEVICE_ALERT);

    memory_region_init_io(&s->mmio, obj, &ot_entropy_src_qp_ops, s,
                          TYPE_OT_ENTROPY_SRC_QP, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    /* Pulse reset to commit RESVALs into every prim_subreg.
     * Pattern in generated code:  q = (~rst_ni) ? RESVAL : keep.
     * C-init leaves rst_ni at 0, so a settle round with rst
     * active forces q := RESVAL across every register.  Then
     * release reset by setting rst_ni high.  Without this pulse,
     * registers like REGWEN (RESVAL=1) stay at their C init=0. */
    entropy_src_reset(&s->core);
}

static void ot_entropy_src_qp_finalize(Object *obj)
{
    (void)obj;
}

static void ot_entropy_src_qp_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    (void)data;

    dc->realize = ot_entropy_src_qp_realize;
    device_class_set_props(dc, ot_entropy_src_qp_properties);
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}
