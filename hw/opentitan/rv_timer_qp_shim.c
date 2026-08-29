/*
 * QEMU OpenTitan RV_TIMER device — qemu-passes shim (auto-generated)
 *
 * DO NOT EDIT.  Re-emit by re-running qemu-passes with --qemu-emit-c
 * after changing the kKnownDevices catalog in lib/QEMUEmitCPass.cpp.
 *
 * Frontend wrapper around the auto-generated RV_TIMER model from
 *   hw/opentitan/qemu_passes/rv_timer.{c,h}
 *
 * MMIO read/write callbacks delegate to the generated entrypoints
 * rv_timer_read() / rv_timer_write().  The QEMU backend
 * interface (chardev, IRQ wires, ptimer, ...) is intentionally NOT wired
 * here — frontend-only milestone.  Backend hookup is a later milestone.
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qapi/error.h"
#include "chardev/char-fe.h"
#include "hw/ptimer.h"
#include "hw/sysbus.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-properties-system.h"
#include "hw/irq.h"
#include "hw/opentitan/ot_alert.h"
#include "hw/opentitan/ot_common.h"
#include "hw/riscv/ibex_irq.h"

/* Generated rv_timer model entrypoints + state struct. */
#include "qemu_passes/rv_timer.h"

/* This shim's own type declaration. */
#include "hw/opentitan/rv_timer_qp_shim.h"

#define OT_RV_TIMER_QP_IRQ_NUM 1u
#define OT_RV_TIMER_QP_PUMP_HZ 1000u

struct OtRvTimerQpState {
    SysBusDevice parent_obj;
    MemoryRegion mmio;
    IbexIRQ irqs[OT_RV_TIMER_QP_IRQ_NUM];
    IbexIRQ alert;

    /* Property fields (mirror upstream ot_rv_timer_eg.c so SoC-table-injected
     * values are accepted by QOM).  We accept-but-ignore: the
     * generated model has its own initial state and no chardev wiring. */
    char * ot_id;
    char * clock_name;
    DeviceState * clock_src;

    /* Virtual-time pump: periodic ptimer that advances the model's
     * free-running counters (via rv_timer_advance_tick) and
     * redelivers interrupts, so timer-style devices keep time + fire
     * autonomously (during CPU wfi, no MMIO). */
    ptimer_state *qp_pump;

    /* Embedded auto-generated device state. */
    rv_timer_state core;
};

OBJECT_DEFINE_SIMPLE_TYPE(OtRvTimerQpState, ot_rv_timer_qp, OT_RV_TIMER_QP, SYS_BUS_DEVICE)

/* === Interrupt delivery ========================================= */
static void ot_rv_timer_qp_update_irqs(OtRvTimerQpState *s)
{
    /* The IRQ lines reflect the SETTLED device state: an external
     * stimulus (pin edge, chardev push, SPI byte) may have moved the
     * model only one clock (synchronizer / edge-detect / INTR_STATE
     * latch still propagating), and a bus read samples the register
     * on the request clock.  Let the model reach quiescence first —
     * cheap when already settled (one tick that changes nothing). */
    rv_timer_settle(&s->core);
    uint32_t intr_state  = (uint32_t)rv_timer_read(&s->core, 0x104u, 4);
    uint32_t intr_enable = (uint32_t)rv_timer_read(&s->core, 0x100u, 4);
    uint32_t masked = intr_state & intr_enable;
    for (unsigned i = 0; i < OT_RV_TIMER_QP_IRQ_NUM; i++) {
        ibex_irq_set(&s->irqs[i], (int)((masked >> i) & 1u));
    }
}

/* === Virtual-time pump ========================================== */
static void ot_rv_timer_qp_pump(void *opaque)
{
    OtRvTimerQpState *s = OT_RV_TIMER_QP(opaque);
    rv_timer_advance_tick(&s->core);
    ot_rv_timer_qp_update_irqs(s);
}

/* Generic core accessor: SoC-integration bridges (device-to-device
 * signal links wired in the machine file) reach the generated state
 * through this + the <dev>.h field API. */
void *ot_rv_timer_qp_core(DeviceState *dev)
{
    return &OT_RV_TIMER_QP(dev)->core;
}

void ot_rv_timer_qp_set_settle_hook(DeviceState *dev, int (*fn)(void *), void *ctx)
{
    OT_RV_TIMER_QP(dev)->core._qp_settle_hook = fn;
    OT_RV_TIMER_QP(dev)->core._qp_settle_hook_ctx = ctx;
}

static uint64_t ot_rv_timer_qp_read(void *opaque, hwaddr addr, unsigned size)
{
    OtRvTimerQpState *s = OT_RV_TIMER_QP(opaque);
    return rv_timer_read(&s->core, addr, size);
}

static void ot_rv_timer_qp_write(void *opaque, hwaddr addr, uint64_t value,
                                 unsigned size)
{
    OtRvTimerQpState *s = OT_RV_TIMER_QP(opaque);
    rv_timer_write(&s->core, addr, value, size);
    /* Alert line: a write to ALERT_TEST (IR-derived offset) pulses
     * the device alert — mirrors upstream ot_rv_timer R_ALERT_TEST. */
    if (addr == 0x0u) {
        ibex_irq_set(&s->alert, (int)(value & 1u));
    }
    ot_rv_timer_qp_update_irqs(s);
}

static const MemoryRegionOps ot_rv_timer_qp_ops = {
    .read = ot_rv_timer_qp_read,
    .write = ot_rv_timer_qp_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
};

static const Property ot_rv_timer_qp_properties[] = {
    DEFINE_PROP_STRING(OT_COMMON_DEV_ID, OtRvTimerQpState, ot_id),
    DEFINE_PROP_STRING("clock-name", OtRvTimerQpState, clock_name),
    DEFINE_PROP_LINK("clock-src", OtRvTimerQpState, clock_src, TYPE_DEVICE,
                     DeviceState *),
};

static void ot_rv_timer_qp_realize(DeviceState *dev, Error **errp)
{
    OtRvTimerQpState *s = OT_RV_TIMER_QP(dev);
    (void)errp;
    /* Arm the virtual-time pump: a free-running periodic ptimer that
     * advances the model's counters + redelivers interrupts so the
     * device keeps time and fires autonomously (functional, not
     * cycle-accurate). */
    s->qp_pump = ptimer_init(ot_rv_timer_qp_pump, s, PTIMER_POLICY_CONTINUOUS_TRIGGER);
    ptimer_transaction_begin(s->qp_pump);
    ptimer_set_freq(s->qp_pump, OT_RV_TIMER_QP_PUMP_HZ);
    ptimer_set_limit(s->qp_pump, 1, 1);
    ptimer_run(s->qp_pump, 0);
    ptimer_transaction_commit(s->qp_pump);
}

static void ot_rv_timer_qp_init(Object *obj)
{
    OtRvTimerQpState *s = OT_RV_TIMER_QP(obj);

    for (unsigned i = 0; i < OT_RV_TIMER_QP_IRQ_NUM; i++) {
        ibex_sysbus_init_irq(obj, &s->irqs[i]);
    }
    ibex_qdev_init_irq(obj, &s->alert, OT_DEVICE_ALERT);

    memory_region_init_io(&s->mmio, obj, &ot_rv_timer_qp_ops, s,
                          TYPE_OT_RV_TIMER_QP, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    /* Pulse reset to commit RESVALs into every prim_subreg.
     * Pattern in generated code:  q = (~rst_ni) ? RESVAL : keep.
     * C-init leaves rst_ni at 0, so a settle round with rst
     * active forces q := RESVAL across every register.  Then
     * release reset by setting rst_ni high.  Without this pulse,
     * registers like REGWEN (RESVAL=1) stay at their C init=0. */
    rv_timer_reset(&s->core);
}

static void ot_rv_timer_qp_finalize(Object *obj)
{
    (void)obj;
}

static void ot_rv_timer_qp_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    (void)data;

    dc->realize = ot_rv_timer_qp_realize;
    device_class_set_props(dc, ot_rv_timer_qp_properties);
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}
