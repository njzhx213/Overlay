/*
 * QEMU OpenTitan SPI_HOST device — qemu-passes shim (auto-generated)
 *
 * DO NOT EDIT.  Re-emit by re-running qemu-passes with --qemu-emit-c
 * after changing the kKnownDevices catalog in lib/QEMUEmitCPass.cpp.
 *
 * Frontend wrapper around the auto-generated SPI_HOST model from
 *   hw/opentitan/qemu_passes/spi_host.{c,h}
 *
 * MMIO read/write callbacks delegate to the generated entrypoints
 * spi_host_read() / spi_host_write().  The QEMU backend
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

/* Generated spi_host model entrypoints + state struct. */
#include "qemu_passes/spi_host.h"

/* This shim's own type declaration. */
#include "hw/opentitan/spi_host_qp_shim.h"

#define OT_SPI_HOST_QP_IRQ_NUM 2u

struct OtSPIHostQpState {
    SysBusDevice parent_obj;
    MemoryRegion mmio;
    IbexIRQ irqs[OT_SPI_HOST_QP_IRQ_NUM];
    IbexIRQ alert;

    /* Property fields (mirror upstream ot_spi_host_eg.c so SoC-table-injected
     * values are accepted by QOM).  We accept-but-ignore: the
     * generated model has its own initial state and no chardev wiring. */
    char * ot_id;
    uint32_t num_cs;
    uint32_t bus_num;
    char * clock_name;
    DeviceState * clock_src;
    uint32_t start_delay_ns;
    uint32_t completion_delay_ns;
    uint8_t version;

    /* Embedded auto-generated device state. */
    spi_host_state core;
};

OBJECT_DEFINE_SIMPLE_TYPE(OtSPIHostQpState, ot_spi_host_qp, OT_SPI_HOST_QP, SYS_BUS_DEVICE)

static uint64_t ot_spi_host_qp_read(void *opaque, hwaddr addr, unsigned size)
{
    OtSPIHostQpState *s = OT_SPI_HOST_QP(opaque);
    return spi_host_read(&s->core, addr, size);
}

static void ot_spi_host_qp_write(void *opaque, hwaddr addr, uint64_t value,
                                 unsigned size)
{
    OtSPIHostQpState *s = OT_SPI_HOST_QP(opaque);
    spi_host_write(&s->core, addr, value, size);
}

static const MemoryRegionOps ot_spi_host_qp_ops = {
    .read = ot_spi_host_qp_read,
    .write = ot_spi_host_qp_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
};

static const Property ot_spi_host_qp_properties[] = {
    DEFINE_PROP_STRING(OT_COMMON_DEV_ID, OtSPIHostQpState, ot_id),
    DEFINE_PROP_UINT32("num-cs", OtSPIHostQpState, num_cs, 1u),
    DEFINE_PROP_UINT32("bus-num", OtSPIHostQpState, bus_num, 0u),
    DEFINE_PROP_STRING("clock-name", OtSPIHostQpState, clock_name),
    DEFINE_PROP_LINK("clock-src", OtSPIHostQpState, clock_src, TYPE_DEVICE,
                     DeviceState *),
    DEFINE_PROP_UINT32("start-delay", OtSPIHostQpState, start_delay_ns, 0u),
    DEFINE_PROP_UINT32("completion-delay", OtSPIHostQpState, completion_delay_ns, 0u),
    DEFINE_PROP_UINT8("version", OtSPIHostQpState, version, 0),
};

static void ot_spi_host_qp_realize(DeviceState *dev, Error **errp)
{
    /* Backend hookup intentionally absent — frontend-only milestone. */
    (void)dev;
    (void)errp;
}

static void ot_spi_host_qp_init(Object *obj)
{
    OtSPIHostQpState *s = OT_SPI_HOST_QP(obj);

    for (unsigned i = 0; i < OT_SPI_HOST_QP_IRQ_NUM; i++) {
        ibex_sysbus_init_irq(obj, &s->irqs[i]);
    }
    ibex_qdev_init_irq(obj, &s->alert, OT_DEVICE_ALERT);

    memory_region_init_io(&s->mmio, obj, &ot_spi_host_qp_ops, s,
                          TYPE_OT_SPI_HOST_QP, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    /* Out-of-reset.  Generated model gates writes on `!rst_ni`,
     * so park rst_ni high or every register stays at its init value. */
    s->core.rst_ni = 1;
}

static void ot_spi_host_qp_finalize(Object *obj)
{
    (void)obj;
}

static void ot_spi_host_qp_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    (void)data;

    dc->realize = ot_spi_host_qp_realize;
    device_class_set_props(dc, ot_spi_host_qp_properties);
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}
