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
    __uint128_t aux_sck_in_shadow; /* aux pin group 'sck' input bus */
    __uint128_t aux_csb_in_shadow; /* aux pin group 'csb' input bus */
    qemu_irq aux_sd_out[4]; /* aux pin group 'sd' output lines */
    __uint128_t aux_sd_out_last;
    __uint128_t aux_sd_in_shadow; /* aux pin group 'sd' input bus */

    /* Property fields (mirror upstream ot_spi_device_eg.c so SoC-table-injected
     * values are accepted by QOM).  We accept-but-ignore: the
     * generated model has its own initial state and no chardev wiring. */
    char * ot_id;
    DeviceState * spi_host;
    CharFrontend chr;

    /* SPI-slave transport (qemu.spi_slave_blueprint): lowRISC spidev
     * framing over the chardev — 8-byte header "/CS" ver mode pad len16,
     * then len MOSI bytes; we answer with len MISO bytes.  Each byte is
     * pumped as 8 sck edges into the model (mode 0, MSB first). */
    uint8_t  spi_hdr[8];
    unsigned spi_hdr_len;
    unsigned spi_remaining;   /* payload bytes still expected */
    bool     spi_release;     /* deassert /CS after this packet */
    bool     spi_cs_low;

    /* Embedded auto-generated device state. */
    spi_device_state core;
};

OBJECT_DEFINE_SIMPLE_TYPE(OtSPIDeviceQpState, ot_spi_device_qp, OT_SPI_DEVICE_QP, SYS_BUS_DEVICE)

/* === Interrupt delivery ========================================= */
static void ot_spi_device_qp_update_irqs(OtSPIDeviceQpState *s)
{
    /* The IRQ lines reflect the SETTLED device state: an external
     * stimulus (pin edge, chardev push, SPI byte) may have moved the
     * model only one clock (synchronizer / edge-detect / INTR_STATE
     * latch still propagating), and a bus read samples the register
     * on the request clock.  Let the model reach quiescence first —
     * cheap when already settled (one tick that changes nothing). */
    spi_device_settle(&s->core);
    uint32_t intr_state  = (uint32_t)spi_device_read(&s->core, 0x0u, 4);
    uint32_t intr_enable = (uint32_t)spi_device_read(&s->core, 0x4u, 4);
    uint32_t masked = intr_state & intr_enable;
    for (unsigned i = 0; i < OT_SPI_DEVICE_QP_IRQ_NUM; i++) {
        ibex_irq_set(&s->irqs[i], (int)((masked >> i) & 1u));
    }
}

/* === SPI-slave transport ======================================== */
/* One full-duplex byte: 8 sck edges on the model's pins (mode 0, MSB
 * first).  MOSI bit lane 0 of `cio_sd_i`, MISO bit lane 1 of `cio_sd_o`.
 * Each setter runs one model tick (= one sck-domain clock); the trailing
 * settle lets the sys-domain synchronisers / command queues catch up. */
static uint8_t ot_spi_device_qp_spi_xfer(OtSPIDeviceQpState *s, uint8_t mosi)
{
    uint8_t miso = 0;
    for (int i = 7; i >= 0; i--) {
        uint64_t sd = (uint64_t)s->core.cio_sd_i & ~(1ull << 0);
        sd |= (uint64_t)((mosi >> i) & 1u) << 0;
        spi_device_set_cio_sd_i(&s->core, sd);
        spi_device_set_cio_sck_i(&s->core, 1);
        miso |= (uint8_t)(((s->core.cio_sd_o >> 1) & 1u) << i);
        spi_device_set_cio_sck_i(&s->core, 0);
    }
    spi_device_settle(&s->core);
    return miso;
}

static void ot_spi_device_qp_spi_cs(OtSPIDeviceQpState *s, bool low)
{
    if (s->spi_cs_low == low) return;
    s->spi_cs_low = low;
    spi_device_set_cio_csb_i(&s->core, low ? 0 : 1);
    spi_device_settle(&s->core);
    ot_spi_device_qp_update_irqs(s);
}

static int ot_spi_device_qp_spi_can_receive(void *opaque)
{
    OtSPIDeviceQpState *s = OT_SPI_DEVICE_QP(opaque);
    if (s->spi_remaining) return (int)(s->spi_remaining > 256u ? 256u : s->spi_remaining);
    return (int)(8u - s->spi_hdr_len);
}

static void ot_spi_device_qp_spi_receive(void *opaque, const uint8_t *buf, int size)
{
    OtSPIDeviceQpState *s = OT_SPI_DEVICE_QP(opaque);
    while (size > 0) {
        if (s->spi_remaining == 0) {
            /* header phase */
            s->spi_hdr[s->spi_hdr_len++] = *buf++; size--;
            if (s->spi_hdr_len < 8) continue;
            s->spi_hdr_len = 0;
            if (s->spi_hdr[0] != '/' || s->spi_hdr[1] != 'C' || s->spi_hdr[2] != 'S' || s->spi_hdr[3] != 0) {
                qemu_log_mask(LOG_GUEST_ERROR, "spi_device_qp: bad spidev header\n");
                continue;
            }
            s->spi_remaining = (unsigned)s->spi_hdr[6] | ((unsigned)s->spi_hdr[7] << 8);
            s->spi_release = !((s->spi_hdr[4] >> 7) & 1u);
            if (s->spi_remaining == 0) {
                if (s->spi_release) ot_spi_device_qp_spi_cs(s, false);
                continue;
            }
            ot_spi_device_qp_spi_cs(s, true);
            continue;
        }
        /* payload phase: pump bytes, answer with MISO bytes */
        uint8_t tx[256];
        unsigned n = 0;
        while (size > 0 && s->spi_remaining && n < sizeof(tx)) {
            tx[n++] = ot_spi_device_qp_spi_xfer(s, *buf++);
            size--; s->spi_remaining--;
        }
        if (qemu_chr_fe_backend_connected(&s->chr))
            qemu_chr_fe_write_all(&s->chr, tx, (int)n);
        if (s->spi_remaining == 0 && s->spi_release)
            ot_spi_device_qp_spi_cs(s, false);
    }
    ot_spi_device_qp_update_irqs(s);
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

/* === Aux pin group 'sck' input path =============== */
static void ot_spi_device_qp_set_aux_sck_in(void *opaque, int n, int level)
{
    OtSPIDeviceQpState *s = OT_SPI_DEVICE_QP(opaque);
    if (level)
        s->aux_sck_in_shadow |= ((__uint128_t)1) << n;
    else
        s->aux_sck_in_shadow &= ~(((__uint128_t)1) << n);
    if (s->core._qp_busy) {
        /* Re-entered from our own settle: latch raw, outer loop
         * sees it next tick (asynchronous-input semantics). */
        s->core.cio_sck_i = (uint64_t)s->aux_sck_in_shadow;
        return;
    }
    spi_device_set_cio_sck_i(&s->core, (uint64_t)s->aux_sck_in_shadow);
    ot_spi_device_qp_update_pins(s);
}

/* === Aux pin group 'csb' input path =============== */
static void ot_spi_device_qp_set_aux_csb_in(void *opaque, int n, int level)
{
    OtSPIDeviceQpState *s = OT_SPI_DEVICE_QP(opaque);
    if (level)
        s->aux_csb_in_shadow |= ((__uint128_t)1) << n;
    else
        s->aux_csb_in_shadow &= ~(((__uint128_t)1) << n);
    if (s->core._qp_busy) {
        /* Re-entered from our own settle: latch raw, outer loop
         * sees it next tick (asynchronous-input semantics). */
        s->core.cio_csb_i = (uint64_t)s->aux_csb_in_shadow;
        return;
    }
    spi_device_set_cio_csb_i(&s->core, (uint64_t)s->aux_csb_in_shadow);
    ot_spi_device_qp_update_pins(s);
}

/* === Aux pin group 'sd' output export ============ */
static void ot_spi_device_qp_update_aux_sd(OtSPIDeviceQpState *s)
{
    __uint128_t data = (__uint128_t)s->core.cio_sd_o;
    for (unsigned i = 0; i < 4; i++) {
        __uint128_t bit = ((__uint128_t)1) << i;
        __uint128_t level = (data & bit) ? bit : 0;
        if ((s->aux_sd_out_last & bit) != level) {
            qemu_set_irq(s->aux_sd_out[i], level ? 1 : 0);
            s->aux_sd_out_last = (s->aux_sd_out_last & ~bit) | level;
        }
    }
}

/* === Aux pin group 'sd' input path =============== */
static void ot_spi_device_qp_set_aux_sd_in(void *opaque, int n, int level)
{
    OtSPIDeviceQpState *s = OT_SPI_DEVICE_QP(opaque);
    if (level)
        s->aux_sd_in_shadow |= ((__uint128_t)1) << n;
    else
        s->aux_sd_in_shadow &= ~(((__uint128_t)1) << n);
    if (s->core._qp_busy) {
        /* Re-entered from our own settle: latch raw, outer loop
         * sees it next tick (asynchronous-input semantics). */
        s->core.cio_sd_i = (uint64_t)s->aux_sd_in_shadow;
        return;
    }
    spi_device_set_cio_sd_i(&s->core, (uint64_t)s->aux_sd_in_shadow);
    ot_spi_device_qp_update_aux_sd(s);
    ot_spi_device_qp_update_pins(s);
}

/* === Pin input path ============================================= */
static void ot_spi_device_qp_set_pin_in(void *opaque, int n, int level)
{
    OtSPIDeviceQpState *s = OT_SPI_DEVICE_QP(opaque);
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
        s->core.cio_sd_i = s->pin_in_shadow;
        return;
    }
    spi_device_set_cio_sd_i(&s->core, s->pin_in_shadow);
    ot_spi_device_qp_update_irqs(s);
    ot_spi_device_qp_update_aux_sd(s);
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
    ot_spi_device_qp_update_aux_sd(s);
}

static const MemoryRegionOps ot_spi_device_qp_ops = {
    .read = ot_spi_device_qp_read,
    .write = ot_spi_device_qp_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
};

static const Property ot_spi_device_qp_properties[] = {
    DEFINE_PROP_STRING(OT_COMMON_DEV_ID, OtSPIDeviceQpState, ot_id),
    DEFINE_PROP_LINK("spi-host", OtSPIDeviceQpState, spi_host, TYPE_DEVICE,
                     DeviceState *),
    DEFINE_PROP_CHR("chardev", OtSPIDeviceQpState, chr),
};

static void ot_spi_device_qp_realize(DeviceState *dev, Error **errp)
{
    OtSPIDeviceQpState *s = OT_SPI_DEVICE_QP(dev);
    (void)errp;
    /* SPI-slave transport: bytes + chip-select from the spidev chardev. */
    s->spi_hdr_len = 0; s->spi_remaining = 0; s->spi_release = true; s->spi_cs_low = false;
    qemu_chr_fe_set_handlers(&s->chr,
                              ot_spi_device_qp_spi_can_receive,
                              ot_spi_device_qp_spi_receive,
                              NULL, NULL,
                              s, NULL, true);
}

static void ot_spi_device_qp_init(Object *obj)
{
    OtSPIDeviceQpState *s = OT_SPI_DEVICE_QP(obj);

    for (unsigned i = 0; i < OT_SPI_DEVICE_QP_IRQ_NUM; i++) {
        ibex_sysbus_init_irq(obj, &s->irqs[i]);
    }
    ibex_qdev_init_irq(obj, &s->alert, OT_DEVICE_ALERT);

    memory_region_init_io(&s->mmio, obj, &ot_spi_device_qp_ops, s,
                          TYPE_OT_SPI_DEVICE_QP, 0x2000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    /* Generic data-pin output lines: firmware drives them by writing
     * the device's data-output register; update_pins pushes them out. */
    qdev_init_gpio_out(DEVICE(obj), s->pin_out, 4);
    /* Generic data-pin input lines: external drivers/testbench toggle
     * them; the handler injects into the model + refreshes IRQs/pins. */
    qdev_init_gpio_in(DEVICE(obj), ot_spi_device_qp_set_pin_in, 4);
    qdev_init_gpio_in_named(DEVICE(obj), ot_spi_device_qp_set_aux_sck_in, "sck-in", 1);
    qdev_init_gpio_in_named(DEVICE(obj), ot_spi_device_qp_set_aux_csb_in, "csb-in", 1);
    qdev_init_gpio_out_named(DEVICE(obj), s->aux_sd_out, "sd-out", 4);
    qdev_init_gpio_in_named(DEVICE(obj), ot_spi_device_qp_set_aux_sd_in, "sd-in", 4);

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
