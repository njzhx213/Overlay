/*
 * QEMU OpenTitan SPI_DEVICE device — qemu-passes shim (auto-generated)
 *
 * DO NOT EDIT.  Re-emit by re-running qemu-passes with --qemu-emit-c
 * after changing the kKnownDevices catalog in lib/QEMUEmitCPass.cpp.
 *
 * Frontend wrapper around the auto-generated SPI_DEVICE model from
 *   hw/opentitan/qemu_passes/spi_device.{c,h}
 *
 * MMIO read/write callbacks delegate to the generated entrypoints
 * spi_device_read() / spi_device_write().  The QEMU backend
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

/* Generated spi_device model entrypoints + state struct. */
#include "qemu_passes/spi_device.h"

/* This shim's own type declaration. */
#include "hw/opentitan/spi_device_qp_shim.h"

#define OT_SPI_DEVICE_QP_IRQ_NUM 8u

struct OtSPIDeviceQpState {
    SysBusDevice parent_obj;
    MemoryRegion mmio;
    IbexIRQ irqs[OT_SPI_DEVICE_QP_IRQ_NUM];
    IbexIRQ alert;

    /* Property fields (mirror upstream ot_spi_device_eg.c so SoC-table-injected
     * values are accepted by QOM).  We accept-but-ignore: the
     * generated model has its own initial state and no chardev wiring. */
    char * ot_id;
    DeviceState * spi_host;

    /* Embedded auto-generated device state. */
    spi_device_state core;
};

OBJECT_DEFINE_SIMPLE_TYPE(OtSPIDeviceQpState, ot_spi_device_qp, OT_SPI_DEVICE_QP, SYS_BUS_DEVICE)

static uint64_t ot_spi_device_qp_read(void *opaque, hwaddr addr, unsigned size)
{
    OtSPIDeviceQpState *s = OT_SPI_DEVICE_QP(opaque);
    return spi_device_read(&s->core, addr, size);
}

static void ot_spi_device_qp_write(void *opaque, hwaddr addr, uint64_t value,
                                   unsigned size)
{
    OtSPIDeviceQpState *s = OT_SPI_DEVICE_QP(opaque);
    spi_device_write(&s->core, addr, value, size);
}

static const MemoryRegionOps ot_spi_device_qp_ops = {
    .read = ot_spi_device_qp_read,
    .write = ot_spi_device_qp_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
};

static const Property ot_spi_device_qp_properties[] = {
    DEFINE_PROP_STRING(OT_COMMON_DEV_ID, OtSPIDeviceQpState, ot_id),
    DEFINE_PROP_LINK("spi-host", OtSPIDeviceQpState, spi_host, TYPE_DEVICE,
                     DeviceState *),
};

static void ot_spi_device_qp_realize(DeviceState *dev, Error **errp)
{
    /* Backend hookup intentionally absent — frontend-only milestone. */
    (void)dev;
    (void)errp;
}

static void ot_spi_device_qp_init(Object *obj)
{
    OtSPIDeviceQpState *s = OT_SPI_DEVICE_QP(obj);

    for (unsigned i = 0; i < OT_SPI_DEVICE_QP_IRQ_NUM; i++) {
        ibex_sysbus_init_irq(obj, &s->irqs[i]);
    }
    ibex_qdev_init_irq(obj, &s->alert, OT_DEVICE_ALERT);

    memory_region_init_io(&s->mmio, obj, &ot_spi_device_qp_ops, s,
                          TYPE_OT_SPI_DEVICE_QP, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    /* Out-of-reset.  Generated model gates writes on `!rst_ni`,
     * so park rst_ni high or every register stays at its init value. */
    s->core.rst_ni = 1;
}

static void ot_spi_device_qp_finalize(Object *obj)
{
    (void)obj;
}

static void ot_spi_device_qp_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    (void)data;

    dc->realize = ot_spi_device_qp_realize;
    device_class_set_props(dc, ot_spi_device_qp_properties);
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}
