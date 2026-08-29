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
#include "hw/ssi/ssi.h"
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
    qemu_irq pin_out[4]; /* data-pin output lines */
    uint64_t pin_out_last; /* last driven output bus (per-bit change dedup) */
    uint64_t pin_in_shadow; /* current data-pin input bus value */

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

    /* SPI-master bridge (qemu.spi_master_blueprint): the model's sck/csb/sd
     * pins are watched every model clock (per-tick observer) and the bit
     * stream is bridged to a byte-level QEMU SSI bus. */
    SSIBus  *ssi;
    IbexIRQ *cs_lines;
    uint32_t spim_csb_prev;
    uint8_t  spim_sck_prev, spim_mosi_prev;
    bool     spim_active;      /* some CS asserted */
    bool     spim_first_edge;  /* first sck edge after CS assert pending (CPHA detect) */
    uint8_t  spim_cpol, spim_cpha;
    uint8_t  spim_bitcnt;      /* sample edges seen in the current byte */
    uint8_t  spim_txsh;        /* MOSI bits shifted in so far (MSB first) */
    uint8_t  spim_rx;          /* MISO byte being presented (replay pass) */
    bool     spim_replay;      /* second pass of the current byte (rx known) */
    /* Speculate/rewind: snapshot of the model + organ state at the byte
     * start; the first pass only learns the MOSI byte, the transfer to
     * the byte-level bus happens at its end, then the byte is replayed
     * from the snapshot with the slave's reply on MISO. */
    spi_host_state *spim_snap;
    bool     spim_snap_valid;
    uint32_t spim_snap_gen;
    uint8_t  spim_snap_sck_prev, spim_snap_mosi_prev;
    bool     spim_snap_first_edge;
    uint8_t  spim_snap_cpol, spim_snap_cpha;

    /* Embedded auto-generated device state. */
    spi_host_state core;
};

OBJECT_DEFINE_SIMPLE_TYPE(OtSPIHostQpState, ot_spi_host_qp, OT_SPI_HOST_QP, SYS_BUS_DEVICE)

/* === Interrupt delivery ========================================= */
static void ot_spi_host_qp_update_irqs(OtSPIHostQpState *s)
{
    /* The IRQ lines reflect the SETTLED device state: an external
     * stimulus (pin edge, chardev push, SPI byte) may have moved the
     * model only one clock (synchronizer / edge-detect / INTR_STATE
     * latch still propagating), and a bus read samples the register
     * on the request clock.  Let the model reach quiescence first —
     * cheap when already settled (one tick that changes nothing). */
    spi_host_settle(&s->core);
    uint32_t intr_state  = (uint32_t)spi_host_read(&s->core, 0x0u, 4);
    uint32_t intr_enable = (uint32_t)spi_host_read(&s->core, 0x4u, 4);
    uint32_t masked = intr_state & intr_enable;
    for (unsigned i = 0; i < OT_SPI_HOST_QP_IRQ_NUM; i++) {
        ibex_irq_set(&s->irqs[i], (int)((masked >> i) & 1u));
    }
}

/* === SPI-master bridge ========================================== */
/* Called after EVERY model clock (model hook _qp_on_tick), so it sees each
 * sck edge the FSM produces inside the MMIO settles.  Bit lanes: MOSI = bit
 * 0 of `cio_sd_o`, MISO = bit 1 of `cio_sd_i`.
 *  - CS lines follow `cio_csb_o` (level; SSI convention 1 = deselected).
 *  - CPOL = sck level at CS assert; CPHA = 1 iff the master launched MOSI on
 *    the very first sck edge (data changed with that edge), else 0.  Sample
 *    edge = rising when CPOL == CPHA, falling otherwise.
 *  - A byte-level bus can only answer once it has the whole byte, but the
 *    bit-level master samples MISO from the first edge.  So every byte runs
 *    TWICE: a first (speculative) pass from a snapshot of the model state
 *    taken at the byte start collects the MOSI byte, hands it to
 *    ssi_transfer at the 8th sample edge, restores the snapshot and replays
 *    the byte with the slave's reply presented on MISO bit by bit.  The
 *    model is deterministic, so the replay is identical except MISO.  The
 *    settle is held while a byte is in flight (`_qp_hold_settle`) so bytes
 *    do not straddle MMIO accesses; a snapshot older than the last MMIO
 *    access (`_qp_access_gen`) is stale -> that byte is transferred late
 *    (reply not replayed) and logged. */
static void ot_spi_host_qp_spim_present_miso(OtSPIHostQpState *s, unsigned bit)
{
    uint64_t sd = (uint64_t)s->core.cio_sd_i & ~(1ull << 1);
    sd |= (uint64_t)((s->spim_rx >> bit) & 1u) << 1;
    s->core.cio_sd_i = sd;
}

static void ot_spi_host_qp_spim_snapshot(OtSPIHostQpState *s)
{
    if (s->core._qp_in_request) { s->spim_snap_valid = false; return; }   /* transient bus inputs: retake next clock */
    memcpy(s->spim_snap, &s->core, sizeof(s->core));
    s->spim_snap_valid = true;
    s->spim_snap_gen = s->core._qp_access_gen;
    s->spim_snap_sck_prev = s->spim_sck_prev; s->spim_snap_mosi_prev = s->spim_mosi_prev;
    s->spim_snap_first_edge = s->spim_first_edge;
    s->spim_snap_cpol = s->spim_cpol; s->spim_snap_cpha = s->spim_cpha;
}

static void ot_spi_host_qp_spim_on_tick(void *opaque)
{
    OtSPIHostQpState *s = OT_SPI_HOST_QP(opaque);
    uint32_t csb = (uint32_t)s->core.cio_csb_o;
    uint8_t  sck = (uint8_t)(s->core.cio_sck_o & 1u);
    bool mosi = ((s->core.cio_sd_o >> 0) & 1u) != 0;
    bool oe   = ((s->core.cio_sd_en_o >> 0) & 1u) != 0;
    uint32_t csmask = (s->num_cs >= 32u) ? 0xFFFFFFFFu : ((1u << s->num_cs) - 1u);
    if ((csb & csmask) != (s->spim_csb_prev & csmask)) {
        for (unsigned i = 0; i < s->num_cs; i++) {
            if (((csb ^ s->spim_csb_prev) >> i) & 1u)
                ibex_irq_set(&s->cs_lines[i], (int)((csb >> i) & 1u));
        }
        bool active = (csb & csmask) != csmask;
        if (active && !s->spim_active) {
            /* transaction start: idle clock level = CPOL, CPHA unknown */
            s->spim_cpol = sck; s->spim_cpha = 0; s->spim_first_edge = true;
            s->spim_bitcnt = 0; s->spim_txsh = 0; s->spim_replay = false;
            s->spim_sck_prev = sck; s->spim_mosi_prev = mosi;
            s->spim_snap_valid = false;   /* taken below, never on a request clock */
        }
        s->spim_active = active;
        s->spim_csb_prev = csb;
        if (!active) { s->spim_snap_valid = false; s->core._qp_hold_settle = 0; }
    }
    if (!s->spim_active) { s->spim_sck_prev = sck; s->spim_mosi_prev = mosi; return; }
    /* Between bytes (nothing in flight) an MMIO access invalidates the
     * snapshot (its effects would be undone by a rewind): retake it now,
     * still before the next byte's first sample edge. */
    if (s->spim_bitcnt == 0 && !s->spim_replay && !s->core._qp_in_request &&
        (!s->spim_snap_valid || s->spim_snap_gen != s->core._qp_access_gen)) {
        s->spim_sck_prev = sck; s->spim_mosi_prev = mosi;
        ot_spi_host_qp_spim_snapshot(s);
    }
    if (sck != s->spim_sck_prev) {
        bool rising = sck != 0;
        if (s->spim_first_edge) {
            s->spim_cpha = (oe && mosi != s->spim_mosi_prev) ? 1u : 0u;
            s->spim_first_edge = false;
        }
        bool sample_edge = (s->spim_cpol == s->spim_cpha) ? rising : !rising;
        if (sample_edge) {
            s->spim_txsh = (uint8_t)((s->spim_txsh << 1) | (mosi ? 1u : 0u));
            s->spim_bitcnt++;
            s->core._qp_hold_settle = 1;   /* byte in flight */
            if (s->spim_bitcnt < 8u) {
                if (s->spim_replay) ot_spi_host_qp_spim_present_miso(s, 7u - s->spim_bitcnt);
            } else if (!s->spim_replay) {
                /* first pass done: the MOSI byte is known -> byte-level transfer */
                s->spim_rx = (uint8_t)ssi_transfer(s->ssi, s->spim_txsh);
                if (s->spim_snap_valid && s->spim_snap_gen == s->core._qp_access_gen) {
                    /* rewind to the byte start and replay with the reply on MISO */
                    memcpy(&s->core, s->spim_snap, sizeof(s->core));
                    s->core._qp_rewound = 1;
                    s->core._qp_hold_settle = 1;
                    s->spim_sck_prev = s->spim_snap_sck_prev; s->spim_mosi_prev = s->spim_snap_mosi_prev;
                    s->spim_first_edge = s->spim_snap_first_edge;
                    s->spim_cpol = s->spim_snap_cpol; s->spim_cpha = s->spim_snap_cpha;
                    s->spim_bitcnt = 0; s->spim_txsh = 0; s->spim_replay = true;
                    ot_spi_host_qp_spim_present_miso(s, 7);
                    return;
                }
                qemu_log_mask(LOG_UNIMP, "ot_spi_host_qp: SPI byte straddled an MMIO access; slave reply 0x%02x not replayed\n", s->spim_rx);
                s->spim_bitcnt = 0; s->spim_txsh = 0;
                s->core._qp_hold_settle = 0;
                ot_spi_host_qp_spim_snapshot(s);
            } else {
                /* replay pass complete: next byte starts from here */
                s->spim_bitcnt = 0; s->spim_txsh = 0; s->spim_replay = false;
                s->core._qp_hold_settle = 0;
                s->spim_sck_prev = sck; s->spim_mosi_prev = mosi;
                ot_spi_host_qp_spim_snapshot(s);
                return;
            }
        }
    }
    s->spim_sck_prev = sck;
    s->spim_mosi_prev = mosi;
}

/* === Pin output export ========================================== */
static void ot_spi_host_qp_update_pins(OtSPIHostQpState *s)
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
static void ot_spi_host_qp_set_pin_in(void *opaque, int n, int level)
{
    OtSPIHostQpState *s = OT_SPI_HOST_QP(opaque);
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
    spi_host_set_cio_sd_i(&s->core, s->pin_in_shadow);
    ot_spi_host_qp_update_irqs(s);
}

/* Generic core accessor: SoC-integration bridges (device-to-device
 * signal links wired in the machine file) reach the generated state
 * through this + the <dev>.h field API. */
void *ot_spi_host_qp_core(DeviceState *dev)
{
    return &OT_SPI_HOST_QP(dev)->core;
}

void ot_spi_host_qp_set_settle_hook(DeviceState *dev, int (*fn)(void *), void *ctx)
{
    OT_SPI_HOST_QP(dev)->core._qp_settle_hook = fn;
    OT_SPI_HOST_QP(dev)->core._qp_settle_hook_ctx = ctx;
}

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
    /* Alert line: a write to ALERT_TEST (IR-derived offset) pulses
     * the device alert — mirrors upstream ot_spi_host R_ALERT_TEST. */
    if (addr == 0xCu) {
        ibex_irq_set(&s->alert, (int)(value & 1u));
    }
    ot_spi_host_qp_update_irqs(s);
    ot_spi_host_qp_update_pins(s);
}

static const MemoryRegionOps ot_spi_host_qp_ops = {
    .read = ot_spi_host_qp_read,
    .write = ot_spi_host_qp_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 1,
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
    OtSPIHostQpState *s = OT_SPI_HOST_QP(dev);
    (void)errp;
    /* SPI-master bridge: SSI bus `spi<bus-num>` + CS lines (upstream
     * naming, so the board's spiflash<N> attach + CS wiring apply), and
     * the per-clock observer that drives them from the model's pins. */
    s->cs_lines = g_new0(IbexIRQ, (size_t)s->num_cs);
    ibex_qdev_init_irqs(OBJECT(dev), &s->cs_lines[0], SSI_GPIO_CS, s->num_cs);
    char busname[16u];
    if (snprintf(busname, sizeof(busname), "spi%u", s->bus_num) >= (int)sizeof(busname)) {
        error_setg(errp, "Invalid SSI bus num %u", s->bus_num);
        return;
    }
    s->ssi = ssi_create_bus(DEVICE(s), busname);
    s->spim_csb_prev = 0xFFFFFFFFu;   /* all deselected */
    s->spim_sck_prev = 0; s->spim_mosi_prev = 0; s->spim_active = false;
    s->spim_snap = g_malloc0(sizeof(s->core));
    s->spim_snap_valid = false; s->spim_replay = false;
    s->core._qp_on_tick = ot_spi_host_qp_spim_on_tick;
    s->core._qp_on_tick_ctx = s;
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

    /* Generic data-pin output lines: firmware drives them by writing
     * the device's data-output register; update_pins pushes them out. */
    qdev_init_gpio_out(DEVICE(obj), s->pin_out, 4);
    /* Generic data-pin input lines: external drivers/testbench toggle
     * them; the handler injects into the model + refreshes IRQs/pins. */
    qdev_init_gpio_in(DEVICE(obj), ot_spi_host_qp_set_pin_in, 4);

    /* Pulse reset to commit RESVALs into every prim_subreg.
     * Pattern in generated code:  q = (~rst_ni) ? RESVAL : keep.
     * C-init leaves rst_ni at 0, so a settle round with rst
     * active forces q := RESVAL across every register.  Then
     * release reset by setting rst_ni high.  Without this pulse,
     * registers like REGWEN (RESVAL=1) stay at their C init=0. */
    spi_host_reset(&s->core);
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
