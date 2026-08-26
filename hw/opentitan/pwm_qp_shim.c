/*
 * QEMU OpenTitan PWM device — qemu-passes shim (auto-generated)
 *
 * DO NOT EDIT.  Re-emit by re-running qemu-passes with --qemu-emit-c
 * after changing the kKnownDevices catalog in lib/QEMUEmitCPass.cpp.
 *
 * Frontend wrapper around the auto-generated PWM model from
 *   hw/opentitan/qemu_passes/pwm.{c,h}
 *
 * MMIO read/write callbacks delegate to the generated entrypoints
 * pwm_read() / pwm_write().  The QEMU backend
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

/* Generated pwm model entrypoints + state struct. */
#include "qemu_passes/pwm.h"

/* This shim's own type declaration. */
#include "hw/opentitan/pwm_qp_shim.h"

#define OT_PWM_QP_IRQ_NUM 0u

struct OtPwmQpState {
    SysBusDevice parent_obj;
    MemoryRegion mmio;
    IbexIRQ irqs[OT_PWM_QP_IRQ_NUM];
    IbexIRQ alert;
    qemu_irq pin_out[6]; /* data-pin output lines */
    uint64_t pin_out_last; /* last driven output bus (per-bit change dedup) */

    /* Property fields (mirror upstream ot_pwm_eg.c so SoC-table-injected
     * values are accepted by QOM).  We accept-but-ignore: the
     * generated model has its own initial state and no chardev wiring. */
    char * ot_id;

    /* Embedded auto-generated device state. */
    pwm_state core;
};

OBJECT_DEFINE_SIMPLE_TYPE(OtPwmQpState, ot_pwm_qp, OT_PWM_QP, SYS_BUS_DEVICE)

/* === Pin output export ========================================== */
static void ot_pwm_qp_update_pins(OtPwmQpState *s)
{
    uint64_t data = (uint64_t)s->core.cio_pwm_o;
    uint64_t oe   = (uint64_t)s->core.cio_pwm_en_o;
    for (unsigned i = 0; i < 6; i++) {
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

static uint64_t ot_pwm_qp_read(void *opaque, hwaddr addr, unsigned size)
{
    OtPwmQpState *s = OT_PWM_QP(opaque);
    return pwm_read(&s->core, addr, size);
}

static void ot_pwm_qp_write(void *opaque, hwaddr addr, uint64_t value,
                            unsigned size)
{
    OtPwmQpState *s = OT_PWM_QP(opaque);
    pwm_write(&s->core, addr, value, size);
    ot_pwm_qp_update_pins(s);
}

static const MemoryRegionOps ot_pwm_qp_ops = {
    .read = ot_pwm_qp_read,
    .write = ot_pwm_qp_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
};

static const Property ot_pwm_qp_properties[] = {
    DEFINE_PROP_STRING(OT_COMMON_DEV_ID, OtPwmQpState, ot_id),
};

static void ot_pwm_qp_realize(DeviceState *dev, Error **errp)
{
    /* Backend hookup intentionally absent — frontend-only milestone. */
    (void)dev;
    (void)errp;
}

static void ot_pwm_qp_init(Object *obj)
{
    OtPwmQpState *s = OT_PWM_QP(obj);

    /* This device has 0 IRQs — no IRQ init loop. */
    ibex_qdev_init_irq(obj, &s->alert, OT_DEVICE_ALERT);

    memory_region_init_io(&s->mmio, obj, &ot_pwm_qp_ops, s,
                          TYPE_OT_PWM_QP, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    /* Generic data-pin output lines: firmware drives them by writing
     * the device's data-output register; update_pins pushes them out. */
    qdev_init_gpio_out(DEVICE(obj), s->pin_out, 6);

    /* Pulse reset to commit RESVALs into every prim_subreg.
     * Pattern in generated code:  q = (~rst_ni) ? RESVAL : keep.
     * C-init leaves rst_ni at 0, so a settle round with rst
     * active forces q := RESVAL across every register.  Then
     * release reset by setting rst_ni high.  Without this pulse,
     * registers like REGWEN (RESVAL=1) stay at their C init=0. */
    pwm_reset(&s->core);
}

static void ot_pwm_qp_finalize(Object *obj)
{
    (void)obj;
}

static void ot_pwm_qp_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    (void)data;

    dc->realize = ot_pwm_qp_realize;
    device_class_set_props(dc, ot_pwm_qp_properties);
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}
