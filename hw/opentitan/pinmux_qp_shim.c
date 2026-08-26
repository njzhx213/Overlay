/*
 * QEMU OpenTitan PINMUX device — qemu-passes shim (auto-generated)
 *
 * DO NOT EDIT.  Re-emit by re-running qemu-passes with --qemu-emit-c
 * after changing the kKnownDevices catalog in lib/QEMUEmitCPass.cpp.
 *
 * Frontend wrapper around the auto-generated PINMUX model from
 *   hw/opentitan/qemu_passes/pinmux.{c,h}
 *
 * MMIO read/write callbacks delegate to the generated entrypoints
 * pinmux_read() / pinmux_write().  The QEMU backend
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

/* Generated pinmux model entrypoints + state struct. */
#include "qemu_passes/pinmux.h"

/* This shim's own type declaration. */
#include "hw/opentitan/pinmux_qp_shim.h"

#define OT_PINMUX_QP_IRQ_NUM 0u

struct OtPinmuxQpState {
    SysBusDevice parent_obj;
    MemoryRegion mmio;
    IbexIRQ irqs[OT_PINMUX_QP_IRQ_NUM];
    IbexIRQ alert;
    qemu_irq pin_out[47]; /* data-pin output lines */
    uint64_t pin_out_last; /* last driven output bus (per-bit change dedup) */
    uint64_t pin_in_shadow; /* current data-pin input bus value */
    qemu_irq aux_periph_out[57]; /* aux pin group 'periph' output lines */
    __uint128_t aux_periph_out_last;
    __uint128_t aux_periph_in_shadow; /* aux pin group 'periph' input bus */

    /* Property fields (mirror upstream ot_pinmux_eg.c so SoC-table-injected
     * values are accepted by QOM).  We accept-but-ignore: the
     * generated model has its own initial state and no chardev wiring. */
    char * ot_id;

    /* Embedded auto-generated device state. */
    pinmux_state core;
};

OBJECT_DEFINE_SIMPLE_TYPE(OtPinmuxQpState, ot_pinmux_qp, OT_PINMUX_QP, SYS_BUS_DEVICE)

/* === Pin output export ========================================== */
static void ot_pinmux_qp_update_pins(OtPinmuxQpState *s)
{
    uint64_t data = (uint64_t)s->core.mio_out_o;
    uint64_t oe   = (uint64_t)s->core.mio_oe_o;
    for (unsigned i = 0; i < 47; i++) {
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

/* === Aux pin group 'periph' output export ============ */
static void ot_pinmux_qp_update_aux_periph(OtPinmuxQpState *s)
{
    __uint128_t data = (__uint128_t)s->core.mio_to_periph_o;
    for (unsigned i = 0; i < 57; i++) {
        __uint128_t bit = ((__uint128_t)1) << i;
        __uint128_t level = (data & bit) ? bit : 0;
        if ((s->aux_periph_out_last & bit) != level) {
            qemu_set_irq(s->aux_periph_out[i], level ? 1 : 0);
            s->aux_periph_out_last = (s->aux_periph_out_last & ~bit) | level;
        }
    }
}

/* === Aux pin group 'periph' input path =============== */
static void ot_pinmux_qp_set_aux_periph_in(void *opaque, int n, int level)
{
    OtPinmuxQpState *s = OT_PINMUX_QP(opaque);
    if (level)
        s->aux_periph_in_shadow |= ((__uint128_t)1) << n;
    else
        s->aux_periph_in_shadow &= ~(((__uint128_t)1) << n);
    if (s->core._qp_busy) {
        /* Re-entered from our own settle: latch raw, outer loop
         * sees it next tick (asynchronous-input semantics). */
        s->core.periph_to_mio_i = s->aux_periph_in_shadow;
        return;
    }
    pinmux_set_periph_to_mio_i(&s->core, s->aux_periph_in_shadow);
    ot_pinmux_qp_update_aux_periph(s);
    ot_pinmux_qp_update_pins(s);
}

/* === Pin input path ============================================= */
static void ot_pinmux_qp_set_pin_in(void *opaque, int n, int level)
{
    OtPinmuxQpState *s = OT_PINMUX_QP(opaque);
    if (level)
        s->pin_in_shadow |= (1ull << n);
    else
        s->pin_in_shadow &= ~(1ull << n);
    if (s->core._qp_busy) {
        /* Re-entered from our own settle (organ observer reached a
         * device that called back).  Latch the raw input field —
         * asynchronous-input semantics — and let the outer settle
         * loop see it on its next tick.  The outer caller refreshes
         * IRQs when its access completes. */
        s->core.mio_in_i = s->pin_in_shadow;
        return;
    }
    pinmux_set_mio_in_i(&s->core, s->pin_in_shadow);
    ot_pinmux_qp_update_aux_periph(s);
}

static uint64_t ot_pinmux_qp_read(void *opaque, hwaddr addr, unsigned size)
{
    OtPinmuxQpState *s = OT_PINMUX_QP(opaque);
    return pinmux_read(&s->core, addr, size);
}

static void ot_pinmux_qp_write(void *opaque, hwaddr addr, uint64_t value,
                               unsigned size)
{
    OtPinmuxQpState *s = OT_PINMUX_QP(opaque);
    pinmux_write(&s->core, addr, value, size);
    ot_pinmux_qp_update_pins(s);
    ot_pinmux_qp_update_aux_periph(s);
}

static const MemoryRegionOps ot_pinmux_qp_ops = {
    .read = ot_pinmux_qp_read,
    .write = ot_pinmux_qp_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
};

static const Property ot_pinmux_qp_properties[] = {
    DEFINE_PROP_STRING(OT_COMMON_DEV_ID, OtPinmuxQpState, ot_id),
};

static void ot_pinmux_qp_realize(DeviceState *dev, Error **errp)
{
    /* Backend hookup intentionally absent — frontend-only milestone. */
    (void)dev;
    (void)errp;
}

static void ot_pinmux_qp_init(Object *obj)
{
    OtPinmuxQpState *s = OT_PINMUX_QP(obj);

    /* This device has 0 IRQs — no IRQ init loop. */
    ibex_qdev_init_irq(obj, &s->alert, OT_DEVICE_ALERT);

    memory_region_init_io(&s->mmio, obj, &ot_pinmux_qp_ops, s,
                          TYPE_OT_PINMUX_QP, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    /* Generic data-pin output lines: firmware drives them by writing
     * the device's data-output register; update_pins pushes them out. */
    qdev_init_gpio_out(DEVICE(obj), s->pin_out, 47);
    /* Generic data-pin input lines: external drivers/testbench toggle
     * them; the handler injects into the model + refreshes IRQs/pins. */
    qdev_init_gpio_in(DEVICE(obj), ot_pinmux_qp_set_pin_in, 47);
    qdev_init_gpio_out_named(DEVICE(obj), s->aux_periph_out, "periph-out", 57);
    qdev_init_gpio_in_named(DEVICE(obj), ot_pinmux_qp_set_aux_periph_in, "periph-in", 75);

    pinmux_set_periph_to_mio_oe_i(&s->core, (~(__uint128_t)0) >> 53);
    /* Pulse reset to commit RESVALs into every prim_subreg.
     * Pattern in generated code:  q = (~rst_ni) ? RESVAL : keep.
     * C-init leaves rst_ni at 0, so a settle round with rst
     * active forces q := RESVAL across every register.  Then
     * release reset by setting rst_ni high.  Without this pulse,
     * registers like REGWEN (RESVAL=1) stay at their C init=0. */
    pinmux_reset(&s->core);
    pinmux_set_periph_to_mio_oe_i(&s->core, (~(__uint128_t)0) >> 53);
    pinmux_settle(&s->core);
}

static void ot_pinmux_qp_finalize(Object *obj)
{
    (void)obj;
}

static void ot_pinmux_qp_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    (void)data;

    dc->realize = ot_pinmux_qp_realize;
    device_class_set_props(dc, ot_pinmux_qp_properties);
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}
