/*
 * QEMU OpenTitan PATTGEN device — qemu-passes shim (auto-generated)
 *
 * DO NOT EDIT.  Re-emit by re-running qemu-passes with --qemu-emit-c
 * after changing the kKnownDevices catalog in lib/QEMUEmitCPass.cpp.
 *
 * Frontend wrapper around the auto-generated PATTGEN model from
 *   hw/opentitan/qemu_passes/pattgen.{c,h}
 *
 * MMIO read/write callbacks delegate to the generated entrypoints
 * pattgen_read() / pattgen_write().  The QEMU backend
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

/* Generated pattgen model entrypoints + state struct. */
#include "qemu_passes/pattgen.h"

/* This shim's own type declaration. */
#include "hw/opentitan/pattgen_qp_shim.h"

#define OT_PATTGEN_QP_IRQ_NUM 2u

struct OtPattgenQpState {
    SysBusDevice parent_obj;
    MemoryRegion mmio;
    IbexIRQ irqs[OT_PATTGEN_QP_IRQ_NUM];
    IbexIRQ alert;
    qemu_irq pin_out[1]; /* data-pin output lines */
    uint64_t pin_out_last; /* last driven output bus (per-bit change dedup) */

    /* Property fields (mirror upstream ot_pattgen_eg.c so SoC-table-injected
     * values are accepted by QOM).  We accept-but-ignore: the
     * generated model has its own initial state and no chardev wiring. */
    char * ot_id;

    /* Embedded auto-generated device state. */
    pattgen_state core;
};

OBJECT_DEFINE_SIMPLE_TYPE(OtPattgenQpState, ot_pattgen_qp, OT_PATTGEN_QP, SYS_BUS_DEVICE)

/* === Interrupt delivery ========================================= */
static void ot_pattgen_qp_update_irqs(OtPattgenQpState *s)
{
    /* The IRQ lines reflect the SETTLED device state: an external
     * stimulus (pin edge, chardev push, SPI byte) may have moved the
     * model only one clock (synchronizer / edge-detect / INTR_STATE
     * latch still propagating), and a bus read samples the register
     * on the request clock.  Let the model reach quiescence first —
     * cheap when already settled (one tick that changes nothing). */
    pattgen_settle(&s->core);
    uint32_t intr_state  = (uint32_t)pattgen_read(&s->core, 0x0u, 4);
    uint32_t intr_enable = (uint32_t)pattgen_read(&s->core, 0x4u, 4);
    uint32_t masked = intr_state & intr_enable;
    for (unsigned i = 0; i < OT_PATTGEN_QP_IRQ_NUM; i++) {
        ibex_irq_set(&s->irqs[i], (int)((masked >> i) & 1u));
    }
}

/* === Pin output export ========================================== */
static void ot_pattgen_qp_update_pins(OtPattgenQpState *s)
{
    uint64_t data = (uint64_t)s->core.cio_pda0_tx_o;
    uint64_t oe   = (uint64_t)s->core.cio_pda0_tx_en_o;
    for (unsigned i = 0; i < 1; i++) {
        uint64_t bit = 1ull << i;
        uint64_t level = ((oe & bit) && (data & bit)) ? bit : 0;
        /* Only pulse lines whose level actually changed — avoids
         * O(width) redundant sink callbacks (and, under a pin loopback,
         * a storm of input re-injections + settles) on every write. */
        if ((s->pin_out_last & bit) != level) {
            qemu_set_irq(s->pin_out[i], level ? 1 : 0);
            s->pin_out_last = (s->pin_out_last & ~bit) | level;
        }
    }
}

/* Generic core accessor: SoC-integration bridges (device-to-device
 * signal links wired in the machine file) reach the generated state
 * through this + the <dev>.h field API. */
void *ot_pattgen_qp_core(DeviceState *dev)
{
    return &OT_PATTGEN_QP(dev)->core;
}

void ot_pattgen_qp_set_settle_hook(DeviceState *dev, int (*fn)(void *), void *ctx)
{
    OT_PATTGEN_QP(dev)->core._qp_settle_hook = fn;
    OT_PATTGEN_QP(dev)->core._qp_settle_hook_ctx = ctx;
}

static uint64_t ot_pattgen_qp_read(void *opaque, hwaddr addr, unsigned size)
{
    OtPattgenQpState *s = OT_PATTGEN_QP(opaque);
    return pattgen_read(&s->core, addr, size);
}

static void ot_pattgen_qp_write(void *opaque, hwaddr addr, uint64_t value,
                                unsigned size)
{
    OtPattgenQpState *s = OT_PATTGEN_QP(opaque);
    pattgen_write(&s->core, addr, value, size);
    /* Alert line: a write to ALERT_TEST (IR-derived offset) pulses
     * the device alert — mirrors upstream ot_pattgen R_ALERT_TEST. */
    if (addr == 0xCu) {
        ibex_irq_set(&s->alert, (int)(value & 1u));
    }
    ot_pattgen_qp_update_irqs(s);
    ot_pattgen_qp_update_pins(s);
}

static const MemoryRegionOps ot_pattgen_qp_ops = {
    .read = ot_pattgen_qp_read,
    .write = ot_pattgen_qp_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
};

static const Property ot_pattgen_qp_properties[] = {
    DEFINE_PROP_STRING(OT_COMMON_DEV_ID, OtPattgenQpState, ot_id),
};

static void ot_pattgen_qp_realize(DeviceState *dev, Error **errp)
{
    /* Backend hookup intentionally absent — frontend-only milestone. */
    (void)dev;
    (void)errp;
}

static void ot_pattgen_qp_init(Object *obj)
{
    OtPattgenQpState *s = OT_PATTGEN_QP(obj);

    for (unsigned i = 0; i < OT_PATTGEN_QP_IRQ_NUM; i++) {
        ibex_sysbus_init_irq(obj, &s->irqs[i]);
    }
    ibex_qdev_init_irq(obj, &s->alert, OT_DEVICE_ALERT);

    memory_region_init_io(&s->mmio, obj, &ot_pattgen_qp_ops, s,
                          TYPE_OT_PATTGEN_QP, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    /* Generic data-pin output lines: firmware drives them by writing
     * the device's data-output register; update_pins pushes them out. */
    qdev_init_gpio_out(DEVICE(obj), s->pin_out, 1);

    /* Pulse reset to commit RESVALs into every prim_subreg.
     * Pattern in generated code:  q = (~rst_ni) ? RESVAL : keep.
     * C-init leaves rst_ni at 0, so a settle round with rst
     * active forces q := RESVAL across every register.  Then
     * release reset by setting rst_ni high.  Without this pulse,
     * registers like REGWEN (RESVAL=1) stay at their C init=0. */
    pattgen_reset(&s->core);
}

static void ot_pattgen_qp_finalize(Object *obj)
{
    (void)obj;
}

static void ot_pattgen_qp_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    (void)data;

    dc->realize = ot_pattgen_qp_realize;
    device_class_set_props(dc, ot_pattgen_qp_properties);
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}
