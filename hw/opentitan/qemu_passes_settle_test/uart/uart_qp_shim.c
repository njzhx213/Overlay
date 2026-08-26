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
 * uart_read() / uart_write().  Chardev backend
 * is wired by intercepting MMIO at the trigger/pop offsets that
 * BackendCharDevDetectPass identified as TX/RX pivots — see the
 * `qemu.chardev_blueprint` attribute on @uart in the IR.
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qapi/error.h"
#include "chardev/char-fe.h"
#include "qemu/fifo8.h"
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
#define OT_UART_QP_RX_BUF_DEPTH 64u

struct OtUARTQpState {
    SysBusDevice parent_obj;
    MemoryRegion mmio;
    IbexIRQ irqs[OT_UART_QP_IRQ_NUM];
    IbexIRQ alert;

    /* Property fields (mirror upstream ot_uart_eg.c so SoC-table-injected
     * values are accepted by QOM). */
    char * ot_id;
    CharFrontend chr;
    char * clock_name;
    DeviceState * clock_src;
    bool oversample_break;
    bool toggle_break;

    /* Shim-side RX byte buffer.  Bytes pushed in via the chardev
     * `receive` callback land here; the read callback drains one
     * byte per R_RDATA read (overriding the generated model's
     * register value, which has its own deserializer-based timing
     * model that doesn't fit chardev's byte-stream interface). */
    Fifo8 rx_buf;

    /* Embedded auto-generated device state. */
    uart_state core;
};

OBJECT_DEFINE_SIMPLE_TYPE(OtUARTQpState, ot_uart_qp, OT_UART_QP, SYS_BUS_DEVICE)

/* === Interrupt delivery ========================================= */
static void ot_uart_qp_update_irqs(OtUARTQpState *s)
{
    uint32_t intr_state  = (uint32_t)uart_read(&s->core, 0x0u, 4);
    uint32_t intr_enable = (uint32_t)uart_read(&s->core, 0x4u, 4);
    uint32_t masked = intr_state & intr_enable;
    for (unsigned i = 0; i < OT_UART_QP_IRQ_NUM; i++) {
        ibex_irq_set(&s->irqs[i], (int)((masked >> i) & 1u));
    }
}

/* === Chardev RX path ============================================ */
/* QEMU calls `can_receive` to ask how many bytes the device can
 * accept right now; we report free space in our shim-side Fifo8.
 * Once `receive` is called with N bytes, we push them into the
 * Fifo8.  Firmware drains them via R_RDATA reads, which the read
 * callback below intercepts at the blueprint's `pop_offset`. */
static int ot_uart_qp_chr_can_receive(void *opaque)
{
    OtUARTQpState *s = OT_UART_QP(opaque);
    return (int)fifo8_num_free(&s->rx_buf);
}

static void ot_uart_qp_chr_receive(void *opaque, const uint8_t *buf, int size)
{
    OtUARTQpState *s = OT_UART_QP(opaque);
    for (int i = 0; i < size; i++) {
        if (fifo8_is_full(&s->rx_buf)) break;
        fifo8_push(&s->rx_buf, buf[i]);
    }
    /* Mirror occupancy: the shim Fifo8 is the SINGLE source of truth
     * for RX data; set the model RX-FIFO pointers so depth ==
     * fifo8_num_used (wptr = count, rptr = 0).  Re-deriving from the
     * Fifo8 each time (instead of incrementing an independent counter)
     * makes data and occupancy impossible to desync — e.g. a
     * FIFO_CTRL.RXRST that zeroes the model pointer in tick() is
     * harmlessly restored on the next sync.  Safe: wvalid_i stays 0 in
     * the chardev-bypass path, and count <= RX_BUF_DEPTH fits the mask. */
    s->core.uart_core_u_uart_rxfifo_gen_normal_fifo_u_fifo_cnt_wptr_wrap_cnt_q = fifo8_num_used(&s->rx_buf) & 0x7Fu;
    s->core.uart_core_u_uart_rxfifo_gen_normal_fifo_u_fifo_cnt_rptr_wrap_cnt_q = 0;
    /* Settle (update_irqs runs the model + reads INTR_STATE/ENABLE)
     * so the new depth propagates to rx_watermark and drives the
     * PLIC line — this is how a real byte raises the interrupt. */
    ot_uart_qp_update_irqs(s);
}

static uint64_t ot_uart_qp_read(void *opaque, hwaddr addr, unsigned size)
{
    OtUARTQpState *s = OT_UART_QP(opaque);
    /* Override R_RDATA reads to drain the shim-side Fifo8 instead of
     * the generated model's RDATA register.  Tell chardev we can
     * accept more bytes after popping (otherwise can_receive=0
     * stays sticky and the byte stream stalls). */
    if (addr == 0x18) {
        uint64_t val = 0;
        if (!fifo8_is_empty(&s->rx_buf)) {
            val = fifo8_pop(&s->rx_buf);
        }
        /* Re-mirror occupancy from the Fifo8 after the pop so depth
         * drops and rx_watermark de-asserts when drained. */
        s->core.uart_core_u_uart_rxfifo_gen_normal_fifo_u_fifo_cnt_wptr_wrap_cnt_q = fifo8_num_used(&s->rx_buf) & 0x7Fu;
        s->core.uart_core_u_uart_rxfifo_gen_normal_fifo_u_fifo_cnt_rptr_wrap_cnt_q = 0;
        qemu_chr_fe_accept_input(&s->chr);
        ot_uart_qp_update_irqs(s);
        return val;
    }
    return uart_read(&s->core, addr, size);
}

static void ot_uart_qp_write(void *opaque, hwaddr addr, uint64_t value,
                             unsigned size)
{
    OtUARTQpState *s = OT_UART_QP(opaque);
    uart_write(&s->core, addr, value, size);
    /* Forward the byte the firmware just wrote to R_<TX_DATA> out
     * via chardev.  The generated model's TX FIFO simulation still
     * runs (we delegated above), but its serialized bit stream is
     * not connected to anything — the chardev gets the byte
     * directly here. */
    if (addr == 0x1C) {
        uint8_t byte = (uint8_t)(value & 0xFFu);
        (void)qemu_chr_fe_write(&s->chr, &byte, 1);
    }
    /* Alert line: a write to ALERT_TEST (IR-derived offset) pulses
     * the device alert — mirrors upstream ot_uart R_ALERT_TEST. */
    if (addr == 0xCu) {
        ibex_irq_set(&s->alert, (int)(value & 1u));
    }
    /* Re-derive RX occupancy from the Fifo8 after EVERY write: a
     * FIFO_CTRL.RXRST zeroes the model RX pointer in tick(), which would
     * otherwise desync occupancy from the still-buffered data.  Since the
     * Fifo8 is the single source of truth, restoring depth == num_used
     * here keeps them consistent (RXRST is a no-op for the shim queue). */
    s->core.uart_core_u_uart_rxfifo_gen_normal_fifo_u_fifo_cnt_wptr_wrap_cnt_q = fifo8_num_used(&s->rx_buf) & 0x7Fu;
    s->core.uart_core_u_uart_rxfifo_gen_normal_fifo_u_fifo_cnt_rptr_wrap_cnt_q = 0;
    ot_uart_qp_update_irqs(s);
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
    OtUARTQpState *s = OT_UART_QP(dev);
    (void)errp;
    qemu_chr_fe_set_handlers(&s->chr,
                              ot_uart_qp_chr_can_receive,
                              ot_uart_qp_chr_receive,
                              NULL, NULL,
                              s, NULL, true);
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

    /* Chardev RX FIFO buffer init.  qemu_chr_fe_set_handlers is
     * deferred to realize() because s->chr isn't bound to the
     * SoC's chardev backend until properties are applied. */
    fifo8_create(&s->rx_buf, OT_UART_QP_RX_BUF_DEPTH);

    /* Pulse reset to commit RESVALs into every prim_subreg.
     * Pattern in generated code:  q = (~rst_ni) ? RESVAL : keep.
     * C-init leaves rst_ni at 0, so a settle round with rst
     * active forces q := RESVAL across every register.  Then
     * release reset by setting rst_ni high.  Without this pulse,
     * registers like REGWEN (RESVAL=1) stay at their C init=0. */
    uart_reset(&s->core);
}

static void ot_uart_qp_finalize(Object *obj)
{
    OtUARTQpState *s = OT_UART_QP(obj);
    fifo8_destroy(&s->rx_buf);
}

static void ot_uart_qp_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    (void)data;

    dc->realize = ot_uart_qp_realize;
    device_class_set_props(dc, ot_uart_qp_properties);
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}
