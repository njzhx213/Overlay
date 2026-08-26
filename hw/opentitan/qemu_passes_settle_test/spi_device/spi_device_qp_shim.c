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
    qemu_irq pin_out[4]; /* data-pin output lines */
    uint64_t pin_out_last; /* last driven output bus (per-bit change dedup) */
    uint64_t pin_in_shadow; /* current data-pin input bus value */

    /* Property fields (mirror upstream ot_spi_device_eg.c so SoC-table-injected
     * values are accepted by QOM).  We accept-but-ignore: the
     * generated model has its own initial state and no chardev wiring. */
    char * ot_id;
    DeviceState * spi_host;

    /* Embedded auto-generated device state. */
    spi_device_state core;
};

OBJECT_DEFINE_SIMPLE_TYPE(OtSPIDeviceQpState, ot_spi_device_qp, OT_SPI_DEVICE_QP, SYS_BUS_DEVICE)

/* === Interrupt delivery ========================================= */
static void ot_spi_device_qp_update_irqs(OtSPIDeviceQpState *s)
{
    uint32_t intr_state  = (uint32_t)spi_device_read(&s->core, 0x0u, 4);
    uint32_t intr_enable = (uint32_t)spi_device_read(&s->core, 0x4u, 4);
    uint32_t masked = intr_state & intr_enable;
    for (unsigned i = 0; i < OT_SPI_DEVICE_QP_IRQ_NUM; i++) {
        ibex_irq_set(&s->irqs[i], (int)((masked >> i) & 1u));
    }
}

/* === Pin output export ========================================== */
static void ot_spi_device_qp_update_pins(OtSPIDeviceQpState *s)
{
    uint64_t data = (uint64_t)s->core.cio_sd_o;
    uint64_t oe   = (uint64_t)s->core.cio_sd_en_o;
    for (unsigned i = 0; i < 4; i++) {
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

/* === Pin input path ============================================= */
static void ot_spi_device_qp_set_pin_in(void *opaque, int n, int level)
{
    OtSPIDeviceQpState *s = OT_SPI_DEVICE_QP(opaque);
    if (level)
        s->pin_in_shadow |= (1ull << n);
    else
        s->pin_in_shadow &= ~(1ull << n);
    spi_device_set_cio_sd_i(&s->core, s->pin_in_shadow);
    ot_spi_device_qp_update_irqs(s);
}

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
    /* Alert line: a write to ALERT_TEST (IR-derived offset) pulses
     * the device alert — mirrors upstream ot_spi_device R_ALERT_TEST. */
    if (addr == 0xCu) {
        ibex_irq_set(&s->alert, (int)(value & 1u));
    }
    ot_spi_device_qp_update_irqs(s);
    ot_spi_device_qp_update_pins(s);
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

    /* Generic data-pin output lines: firmware drives them by writing
     * the device's data-output register; update_pins pushes them out. */
    qdev_init_gpio_out(DEVICE(obj), s->pin_out, 4);
    /* Generic data-pin input lines: external drivers/testbench toggle
     * them; the handler injects into the model + refreshes IRQs/pins. */
    qdev_init_gpio_in(DEVICE(obj), ot_spi_device_qp_set_pin_in, 4);

    /* Pulse reset to commit RESVALs into every prim_subreg.
     * Pattern in generated code:  q = (~rst_ni) ? RESVAL : keep.
     * C-init leaves rst_ni at 0, so a settle round with rst
     * active forces q := RESVAL across every register.  Then
     * release reset by setting rst_ni high.  Without this pulse,
     * registers like REGWEN (RESVAL=1) stay at their C init=0. */
    spi_device_reset(&s->core);
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
