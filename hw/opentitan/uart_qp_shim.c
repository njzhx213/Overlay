/*
 * QEMU OpenTitan UART device — qemu-passes shim (auto-generated)
 *
 * DO NOT EDIT.  Re-emit by re-running qemu-passes with --qemu-emit-c
 * after changing the kKnownDevices catalog in lib/QEMUEmitCPass.cpp.
 *
 * Frontend wrapper around the auto-generated UART model from
 *   hw/opentitan/qemu_passes/uart.{c,h}
 *
 * MMIO read/write callbacks delegate to the generated entrypoints
 * uart_read() / uart_write().  The QEMU backend
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

/* Generated uart model entrypoints + state struct. */
#include "qemu_passes/uart.h"

/* This shim's own type declaration. */
#include "hw/opentitan/uart_qp_shim.h"

#define OT_UART_QP_IRQ_NUM 9u

struct OtUARTQpState {
    SysBusDevice parent_obj;
    MemoryRegion mmio;
    IbexIRQ irqs[OT_UART_QP_IRQ_NUM];
    IbexIRQ alert;

    /* Property fields (mirror upstream ot_uart_eg.c so SoC-table-injected
     * values are accepted by QOM).  We accept-but-ignore: the
     * generated model has its own initial state and no chardev wiring. */
    char * ot_id;
    CharFrontend chr;
    char * clock_name;
    DeviceState * clock_src;
    bool oversample_break;
    bool toggle_break;

    /* Embedded auto-generated device state. */
    uart_state core;
};

OBJECT_DEFINE_SIMPLE_TYPE(OtUARTQpState, ot_uart_qp, OT_UART_QP, SYS_BUS_DEVICE)

static uint64_t ot_uart_qp_read(void *opaque, hwaddr addr, unsigned size)
{
    OtUARTQpState *s = OT_UART_QP(opaque);
    return uart_read(&s->core, addr, size);
}

static void ot_uart_qp_write(void *opaque, hwaddr addr, uint64_t value,
                             unsigned size)
{
    OtUARTQpState *s = OT_UART_QP(opaque);
    uart_write(&s->core, addr, value, size);
}

static const MemoryRegionOps ot_uart_qp_ops = {
    .read = ot_uart_qp_read,
    .write = ot_uart_qp_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
};

static const Property ot_uart_qp_properties[] = {
    DEFINE_PROP_STRING(OT_COMMON_DEV_ID, OtUARTQpState, ot_id),
    DEFINE_PROP_CHR("chardev", OtUARTQpState, chr),
    DEFINE_PROP_STRING("clock-name", OtUARTQpState, clock_name),
    DEFINE_PROP_LINK("clock-src", OtUARTQpState, clock_src, TYPE_DEVICE,
                     DeviceState *),
    DEFINE_PROP_BOOL("oversample-break", OtUARTQpState, oversample_break, false),
    DEFINE_PROP_BOOL("toggle-break", OtUARTQpState, toggle_break, false),
};

static void ot_uart_qp_realize(DeviceState *dev, Error **errp)
{
    /* Backend hookup intentionally absent — frontend-only milestone. */
    (void)dev;
    (void)errp;
}

static void ot_uart_qp_init(Object *obj)
{
    OtUARTQpState *s = OT_UART_QP(obj);

    for (unsigned i = 0; i < OT_UART_QP_IRQ_NUM; i++) {
        ibex_sysbus_init_irq(obj, &s->irqs[i]);
    }
    ibex_qdev_init_irq(obj, &s->alert, OT_DEVICE_ALERT);

    memory_region_init_io(&s->mmio, obj, &ot_uart_qp_ops, s,
                          TYPE_OT_UART_QP, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    /* Out-of-reset.  Generated model gates writes on `!rst_ni`,
     * so park rst_ni high or every register stays at its init value. */
    s->core.rst_ni = 1;
}

static void ot_uart_qp_finalize(Object *obj)
{
    (void)obj;
}

static void ot_uart_qp_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    (void)data;

    dc->realize = ot_uart_qp_realize;
    device_class_set_props(dc, ot_uart_qp_properties);
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}
