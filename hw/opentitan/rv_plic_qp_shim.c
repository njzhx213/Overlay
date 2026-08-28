/*
 * QEMU OpenTitan RV_PLIC device — qemu-passes shim (auto-generated)
 *
 * DO NOT EDIT.  Re-emit by re-running qemu-passes with --qemu-emit-c
 * after changing the kKnownDevices catalog in lib/QEMUEmitCPass.cpp.
 *
 * Frontend wrapper around the auto-generated RV_PLIC model from
 *   hw/opentitan/qemu_passes/rv_plic.{c,h}
 *
 * MMIO read/write callbacks delegate to the generated entrypoints
 * rv_plic_read() / rv_plic_write().  The QEMU backend
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

/* Generated rv_plic model entrypoints + state struct. */
#include "qemu_passes/rv_plic.h"

/* This shim's own type declaration. */
#include "hw/opentitan/rv_plic_qp_shim.h"

#define OT_RV_PLIC_QP_IRQ_NUM 0u

struct OtRvPlicQpState {
    SysBusDevice parent_obj;
    MemoryRegion mmio;
    IbexIRQ irqs[OT_RV_PLIC_QP_IRQ_NUM];
    IbexIRQ alert;
    IbexIRQ out_lines[2]; /* declared 1-bit model outputs (out_lines role) */
    uint64_t pin_in_shadow[3]; /* current data-pin input bus value (wide) */

    /* Property fields (mirror upstream ot_rv_plic_eg.c so SoC-table-injected
     * values are accepted by QOM).  We accept-but-ignore: the
     * generated model has its own initial state and no chardev wiring. */
    char * ot_id;

    /* Embedded auto-generated device state. */
    rv_plic_state core;
};

OBJECT_DEFINE_SIMPLE_TYPE(OtRvPlicQpState, ot_rv_plic_qp, OT_RV_PLIC_QP, SYS_BUS_DEVICE)

/* === Declared output lines (out_lines role) ===================== */
static void ot_rv_plic_qp_update_out_lines(OtRvPlicQpState *s)
{
    /* Settle first: a source that just arrived via set_pin_in has
     * only ticked once — synchronizer chains + the gateway need a
     * few more clocks before the line levels are meaningful (same
     * convention as update_irqs). */
    rv_plic_settle(&s->core);
    ibex_irq_set(&s->out_lines[0], (int)(s->core.irq_o & 1u));
    ibex_irq_set(&s->out_lines[1], (int)(s->core.msip_o & 1u));
}

/* === Pin input path ============================================= */
static void ot_rv_plic_qp_set_pin_in(void *opaque, int n, int level)
{
    OtRvPlicQpState *s = OT_RV_PLIC_QP(opaque);
    if (level)
        s->pin_in_shadow[n >> 6] |= (1ull << (n & 63));
    else
        s->pin_in_shadow[n >> 6] &= ~(1ull << (n & 63));
    if (s->core._qp_busy) {
        /* Re-entered from our own settle (organ observer reached a
         * device that called back).  Latch the raw input field —
         * asynchronous-input semantics — and let the outer settle
         * loop see it on its next tick.  The outer caller refreshes
         * IRQs when its access completes. */
        memcpy(s->core.intr_src_i, s->pin_in_shadow, sizeof(s->pin_in_shadow));
        return;
    }
    rv_plic_set_intr_src_i(&s->core, s->pin_in_shadow);
    ot_rv_plic_qp_update_out_lines(s);
}

/* Generic core accessor: SoC-integration bridges (device-to-device
 * signal links wired in the machine file) reach the generated state
 * through this + the <dev>.h field API. */
void *ot_rv_plic_qp_core(DeviceState *dev)
{
    return &OT_RV_PLIC_QP(dev)->core;
}

static uint64_t ot_rv_plic_qp_read(void *opaque, hwaddr addr, unsigned size)
{
    OtRvPlicQpState *s = OT_RV_PLIC_QP(opaque);
    {
        uint64_t _v = rv_plic_read(&s->core, addr, size);
        ot_rv_plic_qp_update_out_lines(s);
        return _v;
    }
}

static void ot_rv_plic_qp_write(void *opaque, hwaddr addr, uint64_t value,
                                unsigned size)
{
    OtRvPlicQpState *s = OT_RV_PLIC_QP(opaque);
    rv_plic_write(&s->core, addr, value, size);
    ot_rv_plic_qp_update_out_lines(s);
}

static const MemoryRegionOps ot_rv_plic_qp_ops = {
    .read = ot_rv_plic_qp_read,
    .write = ot_rv_plic_qp_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
};

static const Property ot_rv_plic_qp_properties[] = {
    DEFINE_PROP_STRING(OT_COMMON_DEV_ID, OtRvPlicQpState, ot_id),
};

static void ot_rv_plic_qp_realize(DeviceState *dev, Error **errp)
{
    /* Backend hookup intentionally absent — frontend-only milestone. */
    (void)dev;
    (void)errp;
}

static void ot_rv_plic_qp_init(Object *obj)
{
    OtRvPlicQpState *s = OT_RV_PLIC_QP(obj);

    /* This device has 0 IRQs — no IRQ init loop. */
    /* Declared 1-bit output lines (out_lines role): exported as this
     * device's UNNAMED qdev gpio-outs in declaration order — the SoC
     * table's OT_EG_SOC_GPIO(n, ...) macro targets exactly those. */
    ibex_qdev_init_irqs(obj, &s->out_lines[0], NULL, 2);
    ibex_qdev_init_irq(obj, &s->alert, OT_DEVICE_ALERT);

    memory_region_init_io(&s->mmio, obj, &ot_rv_plic_qp_ops, s,
                          TYPE_OT_RV_PLIC_QP, 0x8000000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    /* Generic data-pin input lines: external drivers/testbench toggle
     * them; the handler injects into the model + refreshes IRQs/pins. */
    qdev_init_gpio_in(DEVICE(obj), ot_rv_plic_qp_set_pin_in, 186);

    /* Pulse reset to commit RESVALs into every prim_subreg.
     * Pattern in generated code:  q = (~rst_ni) ? RESVAL : keep.
     * C-init leaves rst_ni at 0, so a settle round with rst
     * active forces q := RESVAL across every register.  Then
     * release reset by setting rst_ni high.  Without this pulse,
     * registers like REGWEN (RESVAL=1) stay at their C init=0. */
    rv_plic_reset(&s->core);
}

static void ot_rv_plic_qp_finalize(Object *obj)
{
    (void)obj;
}

static void ot_rv_plic_qp_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    (void)data;

    dc->realize = ot_rv_plic_qp_realize;
    device_class_set_props(dc, ot_rv_plic_qp_properties);
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}
