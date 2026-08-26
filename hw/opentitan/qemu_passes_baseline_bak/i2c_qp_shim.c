/*
 * QEMU OpenTitan I2C device — qemu-passes shim (auto-generated)
 *
 * DO NOT EDIT.  Re-emit by re-running qemu-passes with --qemu-emit-c
 * after changing the kKnownDevices catalog in lib/QEMUEmitCPass.cpp.
 *
 * Frontend wrapper around the auto-generated I2C model from
 *   hw/opentitan/qemu_passes/i2c.{c,h}
 *
 * MMIO read/write callbacks delegate to the generated entrypoints
 * i2c_read() / i2c_write().  The QEMU backend
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

/* Generated i2c model entrypoints + state struct. */
#include "qemu_passes/i2c.h"

/* This shim's own type declaration. */
#include "hw/opentitan/i2c_qp_shim.h"

#define OT_I2C_QP_IRQ_NUM 10u

struct OtI2CQpState {
    SysBusDevice parent_obj;
    MemoryRegion mmio;
    IbexIRQ irqs[OT_I2C_QP_IRQ_NUM];
    IbexIRQ alert;

    /* Property fields (mirror upstream ot_i2c_eg.c so SoC-table-injected
     * values are accepted by QOM).  We accept-but-ignore: the
     * generated model has its own initial state and no chardev wiring. */
    char * ot_id;
    char * clock_name;
    DeviceState * clock_src;

    /* Embedded auto-generated device state. */
    i2c_state core;
};

OBJECT_DEFINE_SIMPLE_TYPE(OtI2CQpState, ot_i2c_qp, OT_I2C_QP, SYS_BUS_DEVICE)

/* === Interrupt delivery ========================================= */
static void ot_i2c_qp_update_irqs(OtI2CQpState *s)
{
    uint32_t intr_state  = (uint32_t)i2c_read(&s->core, 0x0u, 4);
    uint32_t intr_enable = (uint32_t)i2c_read(&s->core, 0x4u, 4);
    uint32_t masked = intr_state & intr_enable;
    for (unsigned i = 0; i < OT_I2C_QP_IRQ_NUM; i++) {
        ibex_irq_set(&s->irqs[i], (int)((masked >> i) & 1u));
    }
}

static uint64_t ot_i2c_qp_read(void *opaque, hwaddr addr, unsigned size)
{
    OtI2CQpState *s = OT_I2C_QP(opaque);
    return i2c_read(&s->core, addr, size);
}

static void ot_i2c_qp_write(void *opaque, hwaddr addr, uint64_t value,
                            unsigned size)
{
    OtI2CQpState *s = OT_I2C_QP(opaque);
    i2c_write(&s->core, addr, value, size);
    /* Alert line: a write to ALERT_TEST (IR-derived offset) pulses
     * the device alert — mirrors upstream ot_i2c R_ALERT_TEST. */
    if (addr == 0xCu) {
        ibex_irq_set(&s->alert, (int)(value & 1u));
    }
    ot_i2c_qp_update_irqs(s);
}

static const MemoryRegionOps ot_i2c_qp_ops = {
    .read = ot_i2c_qp_read,
    .write = ot_i2c_qp_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
};

static const Property ot_i2c_qp_properties[] = {
    DEFINE_PROP_STRING(OT_COMMON_DEV_ID, OtI2CQpState, ot_id),
    DEFINE_PROP_STRING("clock-name", OtI2CQpState, clock_name),
    DEFINE_PROP_LINK("clock-src", OtI2CQpState, clock_src, TYPE_DEVICE,
                     DeviceState *),
};

static void ot_i2c_qp_realize(DeviceState *dev, Error **errp)
{
    /* Backend hookup intentionally absent — frontend-only milestone. */
    (void)dev;
    (void)errp;
}

static void ot_i2c_qp_init(Object *obj)
{
    OtI2CQpState *s = OT_I2C_QP(obj);

    for (unsigned i = 0; i < OT_I2C_QP_IRQ_NUM; i++) {
        ibex_sysbus_init_irq(obj, &s->irqs[i]);
    }
    ibex_qdev_init_irq(obj, &s->alert, OT_DEVICE_ALERT);

    memory_region_init_io(&s->mmio, obj, &ot_i2c_qp_ops, s,
                          TYPE_OT_I2C_QP, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    /* Pulse reset to commit RESVALs into every prim_subreg.
     * Pattern in generated code:  q = (~rst_ni) ? RESVAL : keep.
     * C-init leaves rst_ni at 0, so a settle round with rst
     * active forces q := RESVAL across every register.  Then
     * release reset by setting rst_ni high.  Without this pulse,
     * registers like REGWEN (RESVAL=1) stay at their C init=0. */
    i2c_reset(&s->core);
}

static void ot_i2c_qp_finalize(Object *obj)
{
    (void)obj;
}

static void ot_i2c_qp_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    (void)data;

    dc->realize = ot_i2c_qp_realize;
    device_class_set_props(dc, ot_i2c_qp_properties);
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}
