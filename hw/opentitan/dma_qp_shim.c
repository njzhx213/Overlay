/*
 * QEMU OpenTitan DMA device — qemu-passes shim (auto-generated)
 *
 * DO NOT EDIT.  Re-emit by re-running qemu-passes with --qemu-emit-c
 * after changing the kKnownDevices catalog in lib/QEMUEmitCPass.cpp.
 *
 * Frontend wrapper around the auto-generated DMA model from
 *   hw/opentitan/qemu_passes/dma.{c,h}
 *
 * MMIO read/write callbacks delegate to the generated entrypoints
 * dma_read() / dma_write().  The QEMU backend
 * interface (chardev, IRQ wires, ptimer, ...) is intentionally NOT wired
 * here — frontend-only milestone.  Backend hookup is a later milestone.
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qapi/error.h"
#include "chardev/char-fe.h"
#include "system/address-spaces.h"
#include "system/memory.h"
#include "hw/sysbus.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-properties-system.h"
#include "hw/irq.h"
#include "hw/opentitan/ot_alert.h"
#include "hw/opentitan/ot_common.h"
#include "hw/riscv/ibex_irq.h"

/* Generated dma model entrypoints + state struct. */
#include "qemu_passes/dma.h"

/* This shim's own type declaration. */
#include "hw/opentitan/dma_qp_shim.h"

#define OT_DMA_QP_IRQ_NUM 3u

struct OtDmaQpState {
    SysBusDevice parent_obj;
    MemoryRegion mmio;
    IbexIRQ irqs[OT_DMA_QP_IRQ_NUM];
    IbexIRQ alert;
    uint64_t pin_in_shadow; /* current data-pin input bus value */

    /* Property fields (mirror upstream ot_dma_eg.c so SoC-table-injected
     * values are accepted by QOM).  We accept-but-ignore: the
     * generated model has its own initial state and no chardev wiring. */
    char * ot_id;

    /* Bus-master bridge (qemu.bus_master_blueprint): request beats on
     * the model's host port(s) are executed against a QEMU AddressSpace
     * and the response is presented on the next model clock. */
    AddressSpace *bm_as_0;     /* 'ot' */
    bool     bm_rsp_pending_0;
    uint8_t  bm_rsp_opcode_0;
    uint32_t bm_rsp_data_0;
    uint8_t  bm_rsp_err_0;
    uint8_t  bm_rsp_size_0;
    uint8_t  bm_rsp_source_0;

    /* Embedded auto-generated device state. */
    dma_state core;
};

OBJECT_DEFINE_SIMPLE_TYPE(OtDmaQpState, ot_dma_qp, OT_DMA_QP, SYS_BUS_DEVICE)

/* === Interrupt delivery ========================================= */
static void ot_dma_qp_update_irqs(OtDmaQpState *s)
{
    /* The IRQ lines reflect the SETTLED device state: an external
     * stimulus (pin edge, chardev push, SPI byte) may have moved the
     * model only one clock (synchronizer / edge-detect / INTR_STATE
     * latch still propagating), and a bus read samples the register
     * on the request clock.  Let the model reach quiescence first —
     * cheap when already settled (one tick that changes nothing). */
    dma_settle(&s->core);
    uint32_t intr_state  = (uint32_t)dma_read(&s->core, 0x0u, 4);
    uint32_t intr_enable = (uint32_t)dma_read(&s->core, 0x4u, 4);
    uint32_t masked = intr_state & intr_enable;
    for (unsigned i = 0; i < OT_DMA_QP_IRQ_NUM; i++) {
        ibex_irq_set(&s->irqs[i], (int)((masked >> i) & 1u));
    }
}

/* === Bus-master bridge ========================================== */
/* Called after EVERY model clock (model hook _qp_on_tick).  For each
 * declared host port: when the model presents a request (a_valid, with
 * our constant a_ready=1), execute it against the port's AddressSpace
 * (Get -> read, Put -> write with byte enables) and present the TL-UL
 * response (d_valid / d_opcode / d_data / d_error, source and size
 * echoed) on the next clock; drop d_valid once the engine takes it
 * (d_ready).  The settle loop needs no help from us: a working
 * transfer never revisits a state, so Brent's cycle exit stays
 * silent and the loop runs until the engine goes quiet. */
static void ot_dma_qp_bm_on_tick(void *opaque)
{
    OtDmaQpState *s = OT_DMA_QP(opaque);

    /* --- port 0: host_tl_h_o -> AddressSpace 'ot' --- */
    {
        s->core.host_tl_h_i_a_ready = 1;
        /* response consumed? (d_valid && engine d_ready this clock) */
        if (s->bm_rsp_pending_0 && s->core.host_tl_h_o_d_ready) {
            s->bm_rsp_pending_0 = false;
            s->core.host_tl_h_i_d_valid = 0;
        }
        if (s->bm_rsp_pending_0) {
            /* keep presenting the response */
            s->core.host_tl_h_i_d_valid = 1;
        } else if (s->core.host_tl_h_o_a_valid) {
            uint32_t addr = (uint32_t)s->core.host_tl_h_o_a_address;
            uint32_t wdata = (uint32_t)s->core.host_tl_h_o_a_data;
            uint8_t  mask = (uint8_t)s->core.host_tl_h_o_a_mask;
            uint8_t  opc  = (uint8_t)s->core.host_tl_h_o_a_opcode;
            bool     is_get = (opc == 4);
            MemTxResult r = MEMTX_OK;
            uint32_t rdata = 0;
            if (is_get) {
                r = address_space_read(s->bm_as_0, addr & ~3u,
                                       MEMTXATTRS_UNSPECIFIED, &rdata, 4);
            } else {
                /* byte-enable write: read-modify-write unless full word */
                if (mask == 0xFu) {
                    r = address_space_write(s->bm_as_0, addr & ~3u,
                                            MEMTXATTRS_UNSPECIFIED, &wdata, 4);
                } else {
                    uint32_t cur = 0;
                    r = address_space_read(s->bm_as_0, addr & ~3u,
                                           MEMTXATTRS_UNSPECIFIED, &cur, 4);
                    for (unsigned b = 0; b < 4; b++)
                        if ((mask >> b) & 1u) {
                            cur &= ~(0xFFu << (8u * b));
                            cur |= (wdata & (0xFFu << (8u * b)));
                        }
                    if (r == MEMTX_OK)
                        r = address_space_write(s->bm_as_0, addr & ~3u,
                                                MEMTXATTRS_UNSPECIFIED, &cur, 4);
                }
            }
            s->bm_rsp_pending_0 = true;
            s->bm_rsp_err_0 = (r != MEMTX_OK);
            s->core.host_tl_h_i_d_valid = 1;
            s->core.host_tl_h_i_d_opcode = is_get ? 1 : 0;  /* AccessAckData : AccessAck */
            s->core.host_tl_h_i_d_data = rdata;
            s->core.host_tl_h_i_d_error = (r != MEMTX_OK);
            s->core.host_tl_h_i_d_size = s->core.host_tl_h_o_a_size;
            s->core.host_tl_h_i_d_source = s->core.host_tl_h_o_a_source;
        }
    }
}

/* === Pin input path ============================================= */
static void ot_dma_qp_set_pin_in(void *opaque, int n, int level)
{
    OtDmaQpState *s = OT_DMA_QP(opaque);
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
        s->core.lsio_trigger_i = s->pin_in_shadow;
        return;
    }
    dma_set_lsio_trigger_i(&s->core, s->pin_in_shadow);
    ot_dma_qp_update_irqs(s);
}

/* Generic core accessor: SoC-integration bridges (device-to-device
 * signal links wired in the machine file) reach the generated state
 * through this + the <dev>.h field API. */
void *ot_dma_qp_core(DeviceState *dev)
{
    return &OT_DMA_QP(dev)->core;
}

void ot_dma_qp_set_settle_hook(DeviceState *dev, int (*fn)(void *), void *ctx)
{
    OT_DMA_QP(dev)->core._qp_settle_hook = fn;
    OT_DMA_QP(dev)->core._qp_settle_hook_ctx = ctx;
}

static uint64_t ot_dma_qp_read(void *opaque, hwaddr addr, unsigned size)
{
    OtDmaQpState *s = OT_DMA_QP(opaque);
    return dma_read(&s->core, addr, size);
}

static void ot_dma_qp_write(void *opaque, hwaddr addr, uint64_t value,
                            unsigned size)
{
    OtDmaQpState *s = OT_DMA_QP(opaque);
    dma_write(&s->core, addr, value, size);
    /* Alert line: a write to ALERT_TEST (IR-derived offset) pulses
     * the device alert — mirrors upstream ot_dma R_ALERT_TEST. */
    if (addr == 0xCu) {
        ibex_irq_set(&s->alert, (int)(value & 1u));
    }
    ot_dma_qp_update_irqs(s);
}

static const MemoryRegionOps ot_dma_qp_ops = {
    .read = ot_dma_qp_read,
    .write = ot_dma_qp_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
};

static const Property ot_dma_qp_properties[] = {
    DEFINE_PROP_STRING(OT_COMMON_DEV_ID, OtDmaQpState, ot_id),
};

static void ot_dma_qp_realize(DeviceState *dev, Error **errp)
{
    OtDmaQpState *s = OT_DMA_QP(dev);
    (void)errp;
    /* Bus-master bridge: bind each declared port's AddressSpace and
     * register the per-clock observer.  The SoC glue may later pass
     * ot_address_space links ([AI-2]); until then the system memory
     * address space is the functional default. */
    s->bm_as_0 = &address_space_memory;
    s->bm_rsp_pending_0 = false;
    s->core._qp_on_tick = ot_dma_qp_bm_on_tick;
    s->core._qp_on_tick_ctx = s;
}

static void ot_dma_qp_init(Object *obj)
{
    OtDmaQpState *s = OT_DMA_QP(obj);

    for (unsigned i = 0; i < OT_DMA_QP_IRQ_NUM; i++) {
        ibex_sysbus_init_irq(obj, &s->irqs[i]);
    }
    ibex_qdev_init_irq(obj, &s->alert, OT_DEVICE_ALERT);

    memory_region_init_io(&s->mmio, obj, &ot_dma_qp_ops, s,
                          TYPE_OT_DMA_QP, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    /* Generic data-pin input lines: external drivers/testbench toggle
     * them; the handler injects into the model + refreshes IRQs/pins. */
    qdev_init_gpio_in(DEVICE(obj), ot_dma_qp_set_pin_in, 11);

    /* Pulse reset to commit RESVALs into every prim_subreg.
     * Pattern in generated code:  q = (~rst_ni) ? RESVAL : keep.
     * C-init leaves rst_ni at 0, so a settle round with rst
     * active forces q := RESVAL across every register.  Then
     * release reset by setting rst_ni high.  Without this pulse,
     * registers like REGWEN (RESVAL=1) stay at their C init=0. */
    dma_reset(&s->core);
}

static void ot_dma_qp_finalize(Object *obj)
{
    (void)obj;
}

static void ot_dma_qp_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    (void)data;

    dc->realize = ot_dma_qp_realize;
    device_class_set_props(dc, ot_dma_qp_properties);
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}
