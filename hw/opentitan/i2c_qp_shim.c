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
#include "hw/i2c/i2c.h"
#include "qemu/timer.h"
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
    I2CBus *i2c_bus; /* qemu.i2c_wire_blueprint transport bus */
    I2CSlave *i2cw_target; /* byte-level QEMU master -> pin-level model */
    bool i2cw_scl, i2cw_sda, i2cw_slave_sda_low;
    bool i2cw_wire_busy, i2cw_bus_active;
    bool i2cw_external_master, i2cw_master_scl_low, i2cw_master_sda_low;
    bool i2cw_scan_from_rtl;
    bool i2cw_expect_address, i2cw_next_read, i2cw_ack;
    uint8_t i2cw_state, i2cw_byte, i2cw_bit;
    uint32_t i2cw_kick_ticks, i2cw_timer_ticks;
    QEMUTimer *i2cw_timer;

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

#define TYPE_OT_I2C_QP_TARGET "ot-i2c-qp-target"
OBJECT_DECLARE_SIMPLE_TYPE(OtI2CQpStateTarget, OT_I2C_QP_TARGET)

struct OtI2CQpStateTarget {
    I2CSlave parent_obj;
    OtI2CQpState *owner;
    uint8_t pending_address;
};

/* === Interrupt delivery ========================================= */
static void ot_i2c_qp_update_irqs(OtI2CQpState *s)
{
    /* The IRQ lines reflect the SETTLED device state: an external
     * stimulus (pin edge, chardev push, SPI byte) may have moved the
     * model only one clock (synchronizer / edge-detect / INTR_STATE
     * latch still propagating), and a bus read samples the register
     * on the request clock.  Let the model reach quiescence first —
     * cheap when already settled (one tick that changes nothing). */
    i2c_settle(&s->core);
    uint32_t intr_state  = (uint32_t)i2c_read(&s->core, 0x0u, 4);
    uint32_t intr_enable = (uint32_t)i2c_read(&s->core, 0x4u, 4);
    uint32_t masked = intr_state & intr_enable;
    for (unsigned i = 0; i < OT_I2C_QP_IRQ_NUM; i++) {
        ibex_irq_set(&s->irqs[i], (int)((masked >> i) & 1u));
    }
}

/* === I2C wire bridge ============================================ */
/* Generic open-drain, pin-level adapter.  The IP-specific port names
 * below originate only from qemu.i2c_wire_blueprint. */
enum { I2CW_IDLE, I2CW_RECV, I2CW_ACK, I2CW_SEND, I2CW_MASTER_ACK };
static void ot_i2c_qp_i2cw_on_tick(void *opaque)
{
    OtI2CQpState *s = OT_I2C_QP(opaque);
    bool scl = !(((s->core.cio_scl_en_o) && !(s->core.cio_scl_o)) || s->i2cw_master_scl_low);
    bool sda = !(((s->core.cio_sda_en_o) && !(s->core.cio_sda_o)) || s->i2cw_slave_sda_low || s->i2cw_master_sda_low);
    bool rising = !s->i2cw_scl && scl, falling = s->i2cw_scl && !scl;
    bool start = s->i2cw_sda && !sda && scl, stop = !s->i2cw_sda && sda && scl;
    /* Keep clocks flowing while the wire is changing.  If an active
     * transaction pauses between FIFO commands, the budget expires so
     * the device timer cannot starve the guest CPU; the next MMIO write
     * supplies a fresh budget. */
    if (!s->i2cw_external_master && (rising || falling || start || stop)) {
        s->i2cw_kick_ticks = 4096u;
        /* Re-arm: once an idle period drained the budget the timer
         * chain is dead — fresh wire activity must restart it or the
         * clock stops right after START. */
        if (s->i2cw_timer && !timer_pending(s->i2cw_timer))
            timer_mod(s->i2cw_timer, qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + 1);
    }
    if (!s->i2cw_external_master) {
    if (start) { qemu_log_mask(LOG_UNIMP, "i2cw: START\n"); s->i2cw_wire_busy = true; s->i2cw_state = I2CW_RECV; s->i2cw_expect_address = true; s->i2cw_byte = s->i2cw_bit = 0; s->i2cw_slave_sda_low = false; }
    if (stop) { qemu_log_mask(LOG_UNIMP, "i2cw: STOP\n"); if (s->i2cw_bus_active) { i2c_end_transfer(s->i2c_bus); i2c_schedule_pending_master(s->i2c_bus); } s->i2cw_bus_active = false; s->i2cw_wire_busy = false; s->i2cw_state = I2CW_IDLE; s->i2cw_slave_sda_low = false; s->i2cw_kick_ticks = 0; if (s->i2cw_timer) timer_del(s->i2cw_timer); }
    if (rising) switch (s->i2cw_state) {
    case I2CW_RECV: s->i2cw_byte = (uint8_t)((s->i2cw_byte << 1) | sda);
      if (++s->i2cw_bit == 8) { int rc; s->i2cw_bit = 0;
        if (s->i2cw_expect_address) { s->i2cw_next_read = s->i2cw_byte & 1u; s->i2cw_scan_from_rtl = true; rc = i2c_start_transfer(s->i2c_bus, s->i2cw_byte >> 1, s->i2cw_next_read); s->i2cw_scan_from_rtl = false; qemu_log_mask(LOG_UNIMP, "i2cw: ADDR byte=0x%02x addr=0x%02x rw=%u rc=%d\n", s->i2cw_byte, s->i2cw_byte >> 1, s->i2cw_next_read, rc); s->i2cw_bus_active = rc == 0; s->i2cw_expect_address = false; }
        else { rc = i2c_send(s->i2c_bus, s->i2cw_byte); qemu_log_mask(LOG_UNIMP, "i2cw: WRITE byte=0x%02x rc=%d\n", s->i2cw_byte, rc); }
        s->i2cw_ack = rc == 0; s->i2cw_state = I2CW_ACK; } break;
    case I2CW_ACK: qemu_log_mask(LOG_UNIMP, "i2cw: ACK sampled low=%u\n", s->i2cw_slave_sda_low); /* Hold ACK through the complete high phase; the
       following falling edge releases SDA or presents read bit 7. */
      if (s->i2cw_next_read && s->i2cw_bus_active) { s->i2cw_byte = i2c_recv(s->i2c_bus); s->i2cw_bit = 0; s->i2cw_state = I2CW_SEND; } else s->i2cw_state = I2CW_RECV; break;
    case I2CW_SEND: if (++s->i2cw_bit == 8) s->i2cw_state = I2CW_MASTER_ACK; break;
    case I2CW_MASTER_ACK: if (!sda) { s->i2cw_byte = i2c_recv(s->i2c_bus); s->i2cw_bit = 0; s->i2cw_state = I2CW_SEND; } else { i2c_nack(s->i2c_bus); s->i2cw_state = I2CW_IDLE; } break;
    default: break; }
    if (falling) { if (s->i2cw_state == I2CW_ACK) { s->i2cw_slave_sda_low = s->i2cw_ack; qemu_log_mask(LOG_UNIMP, "i2cw: ACK drive low=%u\n", s->i2cw_slave_sda_low); } else if (s->i2cw_state == I2CW_SEND) s->i2cw_slave_sda_low = !((s->i2cw_byte >> (7u - s->i2cw_bit)) & 1u); else if (s->i2cw_state == I2CW_RECV || s->i2cw_state == I2CW_MASTER_ACK) s->i2cw_slave_sda_low = false; }
    }
    s->i2cw_scl = scl; s->i2cw_sda = sda;
    s->core.cio_scl_i = scl; s->core.cio_sda_i = sda;
}

/* QEMU byte-level masters (including asynchronous masters such as
 * i2c-echo) enter through a proxy I2CSlave.  Convert their callbacks
 * to ordinary open-drain wire phases so address matching, ACK/NACK,
 * target FIFOs and interrupts remain implemented by the generated
 * hardware model rather than duplicated in this shim. */
#define I2CW_EXT_HALF_TICKS 16u

static void ot_i2c_qp_i2cw_ext_clock_many(OtI2CQpState *s, unsigned count)
{
    /* Explicit wire time must also advance ACCUMULATE rings (input
     * filters/hold counters).  MMIO settle clocks intentionally do not. */
    s->core._qp_pump = 1;
    i2c_step_many(&s->core, count);
    s->core._qp_pump = 0;
}

static void ot_i2c_qp_i2cw_ext_phase(OtI2CQpState *s, bool scl, bool sda)
{
    s->i2cw_master_scl_low = !scl;
    s->i2cw_master_sda_low = !sda;
    ot_i2c_qp_i2cw_ext_clock_many(s, I2CW_EXT_HALF_TICKS);
}

static bool ot_i2c_qp_i2cw_ext_write_byte(OtI2CQpState *s, uint8_t value)
{
    for (int bit = 7; bit >= 0; --bit) {
        bool level = (value >> bit) & 1u;
        ot_i2c_qp_i2cw_ext_phase(s, false, level);
        ot_i2c_qp_i2cw_ext_phase(s, true, level);
    }
    ot_i2c_qp_i2cw_ext_phase(s, false, true);
    ot_i2c_qp_i2cw_ext_phase(s, true, true);
    bool ack = !s->i2cw_sda;
    ot_i2c_qp_i2cw_ext_phase(s, false, true);
    return ack;
}

static uint8_t ot_i2c_qp_i2cw_ext_read_byte(OtI2CQpState *s)
{
    uint8_t value = 0;
    for (unsigned bit = 0; bit < 8; ++bit) {
        ot_i2c_qp_i2cw_ext_phase(s, false, true);
        ot_i2c_qp_i2cw_ext_phase(s, true, true);
        value = (uint8_t)((value << 1) | s->i2cw_sda);
    }
    ot_i2c_qp_i2cw_ext_phase(s, false, false);
    ot_i2c_qp_i2cw_ext_phase(s, true, false);
    ot_i2c_qp_i2cw_ext_phase(s, false, true);
    return value;
}

static bool ot_i2c_qp_i2cw_ext_start(OtI2CQpState *s, uint8_t address, bool read)
{
    if (s->i2cw_timer) timer_del(s->i2cw_timer);
    s->i2cw_kick_ticks = 0;
    s->i2cw_external_master = true;
    s->i2cw_slave_sda_low = false;
    /* QEMU may schedule a pending master as soon as STOP is observed,
     * before the RTL host FSM completes its bus-free/release states. */
    s->i2cw_master_scl_low = false;
    s->i2cw_master_sda_low = false;
    ot_i2c_qp_i2cw_ext_clock_many(s, 4096u);
    ot_i2c_qp_i2cw_ext_phase(s, true, true);
    ot_i2c_qp_i2cw_ext_phase(s, true, false);
    ot_i2c_qp_i2cw_ext_phase(s, false, false);
    return ot_i2c_qp_i2cw_ext_write_byte(s, (uint8_t)((address << 1) | read));
}

static void ot_i2c_qp_i2cw_ext_stop(OtI2CQpState *s)
{
    ot_i2c_qp_i2cw_ext_phase(s, false, false);
    ot_i2c_qp_i2cw_ext_phase(s, true, false);
    ot_i2c_qp_i2cw_ext_phase(s, true, true);
    s->i2cw_external_master = false;
    s->i2cw_master_scl_low = false;
    s->i2cw_master_sda_low = false;
    i2c_step_many(&s->core, I2CW_EXT_HALF_TICKS);
}

static bool ot_i2c_qp_target_match_and_add(I2CSlave *candidate, uint8_t address, bool broadcast,
                                            I2CNodeList *current_devs)
{
    OtI2CQpStateTarget *target = OT_I2C_QP_TARGET(candidate);
    (void)broadcast;
    if (!target->owner || target->owner->i2cw_scan_from_rtl) return false;
    target->pending_address = address;
    I2CNode *node = g_new0(I2CNode, 1);
    node->elt = candidate;
    QLIST_INSERT_HEAD(current_devs, node, next);
    return true;
}

static int ot_i2c_qp_target_event(I2CSlave *candidate, enum i2c_event event)
{
    OtI2CQpStateTarget *target = OT_I2C_QP_TARGET(candidate);
    OtI2CQpState *s = target->owner;
    bool ack;
    switch (event) {
    case I2C_START_SEND:
    case I2C_START_SEND_ASYNC:
        ack = ot_i2c_qp_i2cw_ext_start(s, target->pending_address, false);
        if (ack && event == I2C_START_SEND_ASYNC) i2c_ack(s->i2c_bus);
        return ack ? 0 : -1;
    case I2C_START_RECV:
        return ot_i2c_qp_i2cw_ext_start(s, target->pending_address, true) ? 0 : -1;
    case I2C_FINISH:
        ot_i2c_qp_i2cw_ext_stop(s);
        return 0;
    case I2C_NACK:
        return 0;
    default:
        return -1;
    }
}

static int ot_i2c_qp_target_send(I2CSlave *candidate, uint8_t data)
{
    OtI2CQpState *s = OT_I2C_QP_TARGET(candidate)->owner;
    return ot_i2c_qp_i2cw_ext_write_byte(s, data) ? 0 : -1;
}

static void ot_i2c_qp_target_send_async(I2CSlave *candidate, uint8_t data)
{
    OtI2CQpState *s = OT_I2C_QP_TARGET(candidate)->owner;
    if (ot_i2c_qp_i2cw_ext_write_byte(s, data)) i2c_ack(s->i2c_bus);
}

static uint8_t ot_i2c_qp_target_recv(I2CSlave *candidate)
{
    return ot_i2c_qp_i2cw_ext_read_byte(OT_I2C_QP_TARGET(candidate)->owner);
}

static void ot_i2c_qp_target_class_init(ObjectClass *klass, const void *data)
{
    I2CSlaveClass *sc = I2C_SLAVE_CLASS(klass);
    (void)data;
    sc->match_and_add = ot_i2c_qp_target_match_and_add;
    sc->event = ot_i2c_qp_target_event;
    sc->send = ot_i2c_qp_target_send;
    sc->send_async = ot_i2c_qp_target_send_async;
    sc->recv = ot_i2c_qp_target_recv;
}

static const TypeInfo ot_i2c_qp_target_info = {
    .name = TYPE_OT_I2C_QP_TARGET,
    .parent = TYPE_I2C_SLAVE,
    .instance_size = sizeof(OtI2CQpStateTarget),
    .class_init = ot_i2c_qp_target_class_init,
};

static void ot_i2c_qp_target_register_types(void)
{
    type_register_static(&ot_i2c_qp_target_info);
}
type_init(ot_i2c_qp_target_register_types)

static void ot_i2c_qp_i2cw_timer(void *opaque)
{
    OtI2CQpState *s = OT_I2C_QP(opaque);
    /* Batch model clocks per QEMU timer event.  _step invokes the
     * per-clock wire observer on every iteration, so no I2C edge is
     * skipped; batching only removes event-loop overhead. */
    unsigned batch = MIN(s->i2cw_kick_ticks, 4096u);
    s->i2cw_kick_ticks = s->i2cw_kick_ticks > batch ? s->i2cw_kick_ticks - batch : 0;
    /* Timer-pumped wire time must advance ACCUMULATE rings (the SCL
     * half-period dividers are pump-gated counters) — same rule as
     * the external-master clocking path. */
    if (batch) { s->core._qp_pump = 1; i2c_step_many(&s->core, batch); s->core._qp_pump = 0; }
    s->i2cw_timer_ticks += batch;
    if (s->i2cw_kick_ticks) timer_mod(s->i2cw_timer, qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + 1);
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
    /* Kick the pin model before START exists; START then switches the
     * timer to wire_busy-driven scheduling. */
    s->i2cw_kick_ticks = 4096u;
    timer_mod(s->i2cw_timer, qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + 1000);
}

static const MemoryRegionOps ot_i2c_qp_ops = {
    .read = ot_i2c_qp_read,
    .write = ot_i2c_qp_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 1,
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
    OtI2CQpState *s = OT_I2C_QP(dev);
    (void)errp;
    char *i2c_bus_name = g_strdup_printf("ot-%s", s->ot_id);
    s->i2c_bus = i2c_init_bus(dev, i2c_bus_name);
    g_free(i2c_bus_name);
    s->i2cw_target = i2c_slave_create_simple(s->i2c_bus, TYPE_OT_I2C_QP_TARGET, 0xff);
    OT_I2C_QP_TARGET(s->i2cw_target)->owner = s;
    s->i2cw_scl = true; s->i2cw_sda = true;
    s->core.cio_scl_i = 1; s->core.cio_sda_i = 1;
    s->i2cw_state = I2CW_IDLE;
    s->i2cw_timer = timer_new_ns(QEMU_CLOCK_VIRTUAL, ot_i2c_qp_i2cw_timer, s);
    s->core._qp_before_tick = ot_i2c_qp_i2cw_on_tick;
    s->core._qp_before_tick_ctx = s;
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
    /* Binding-declared simulation defaults.  These are ordinary MMIO
     * writes, so firmware may replace them exactly as on hardware. */
    i2c_write(&s->core, 0x3Cu, 0x40004u, 4);
    i2c_write(&s->core, 0x40u, 0x10001u, 4);
    i2c_write(&s->core, 0x44u, 0x20002u, 4);
    i2c_write(&s->core, 0x48u, 0x10001u, 4);
    i2c_write(&s->core, 0x4Cu, 0x20002u, 4);
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
