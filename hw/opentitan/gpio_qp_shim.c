/*
 * QEMU OpenTitan GPIO device — qemu-passes shim (auto-generated)
 *
 * DO NOT EDIT.  Re-emit by re-running qemu-passes with --qemu-emit-c
 * after changing the kKnownDevices catalog in lib/QEMUEmitCPass.cpp.
 *
 * Frontend wrapper around the auto-generated GPIO model from
 *   hw/opentitan/qemu_passes/gpio.{c,h}
 *
 * MMIO read/write callbacks delegate to the generated entrypoints
 * gpio_read() / gpio_write().  The QEMU backend
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

/* Generated gpio model entrypoints + state struct. */
#include "qemu_passes/gpio.h"

/* This shim's own type declaration. */
#include "hw/opentitan/gpio_qp_shim.h"

#define OT_GPIO_QP_IRQ_NUM 32u

struct OtGpioQpState {
    SysBusDevice parent_obj;
    MemoryRegion mmio;
    IbexIRQ irqs[OT_GPIO_QP_IRQ_NUM];
    IbexIRQ alert;

    /* Property fields (mirror upstream ot_gpio_eg.c so SoC-table-injected
     * values are accepted by QOM).  We accept-but-ignore: the
     * generated model has its own initial state and no chardev wiring. */
    char * ot_id;
    uint32_t reset_in;
    uint32_t reset_out;
    uint32_t reset_oe;
    bool wipe;
    CharFrontend chr;

    /* Embedded auto-generated device state. */
    gpio_state core;
};

OBJECT_DEFINE_SIMPLE_TYPE(OtGpioQpState, ot_gpio_qp, OT_GPIO_QP, SYS_BUS_DEVICE)

static uint64_t ot_gpio_qp_read(void *opaque, hwaddr addr, unsigned size)
{
    OtGpioQpState *s = OT_GPIO_QP(opaque);
    return gpio_read(&s->core, addr, size);
}

static void ot_gpio_qp_write(void *opaque, hwaddr addr, uint64_t value,
                             unsigned size)
{
    OtGpioQpState *s = OT_GPIO_QP(opaque);
    gpio_write(&s->core, addr, value, size);
}

static const MemoryRegionOps ot_gpio_qp_ops = {
    .read = ot_gpio_qp_read,
    .write = ot_gpio_qp_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
};

static const Property ot_gpio_qp_properties[] = {
    DEFINE_PROP_STRING(OT_COMMON_DEV_ID, OtGpioQpState, ot_id),
    DEFINE_PROP_UINT32("in", OtGpioQpState, reset_in, 0u),
    DEFINE_PROP_UINT32("out", OtGpioQpState, reset_out, 0u),
    DEFINE_PROP_UINT32("oe", OtGpioQpState, reset_oe, 0u),
    DEFINE_PROP_BOOL("wipe", OtGpioQpState, wipe, false),
    DEFINE_PROP_CHR("chardev", OtGpioQpState, chr),
};

static void ot_gpio_qp_realize(DeviceState *dev, Error **errp)
{
    /* Backend hookup intentionally absent — frontend-only milestone. */
    (void)dev;
    (void)errp;
}

static void ot_gpio_qp_init(Object *obj)
{
    OtGpioQpState *s = OT_GPIO_QP(obj);

    for (unsigned i = 0; i < OT_GPIO_QP_IRQ_NUM; i++) {
        ibex_sysbus_init_irq(obj, &s->irqs[i]);
    }
    ibex_qdev_init_irq(obj, &s->alert, OT_DEVICE_ALERT);

    memory_region_init_io(&s->mmio, obj, &ot_gpio_qp_ops, s,
                          TYPE_OT_GPIO_QP, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    /* Out-of-reset.  Generated model gates writes on `!rst_ni`,
     * so park rst_ni high or every register stays at its init value. */
    s->core.rst_ni = 1;
}

static void ot_gpio_qp_finalize(Object *obj)
{
    (void)obj;
}

static void ot_gpio_qp_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    (void)data;

    dc->realize = ot_gpio_qp_realize;
    device_class_set_props(dc, ot_gpio_qp_properties);
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}
