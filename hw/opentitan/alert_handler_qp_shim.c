/*
 * QEMU OpenTitan ALERT_HANDLER device — qemu-passes shim (auto-generated)
 *
 * DO NOT EDIT.  Re-emit by re-running qemu-passes with --qemu-emit-c
 * after changing the kKnownDevices catalog in lib/QEMUEmitCPass.cpp.
 *
 * Frontend wrapper around the auto-generated ALERT_HANDLER model from
 *   hw/opentitan/qemu_passes/alert_handler.{c,h}
 *
 * MMIO read/write callbacks delegate to the generated entrypoints
 * alert_handler_read() / alert_handler_write().  The QEMU backend
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

/* Generated alert_handler model entrypoints + state struct. */
#include "qemu_passes/alert_handler.h"

/* This shim's own type declaration. */
#include "hw/opentitan/alert_handler_qp_shim.h"

#define OT_ALERT_HANDLER_QP_IRQ_NUM 4u

struct OtAlertHandlerQpState {
    SysBusDevice parent_obj;
    MemoryRegion mmio;
    IbexIRQ irqs[OT_ALERT_HANDLER_QP_IRQ_NUM];
    IbexIRQ alert;

    /* Property fields (mirror upstream ot_alert_handler_eg.c so SoC-table-injected
     * values are accepted by QOM).  We accept-but-ignore: the
     * generated model has its own initial state and no chardev wiring. */
    char * ot_id;

    /* Embedded auto-generated device state. */
    alert_handler_state core;
};

OBJECT_DEFINE_SIMPLE_TYPE(OtAlertHandlerQpState, ot_alert_handler_qp, OT_ALERT_HANDLER_QP, SYS_BUS_DEVICE)

/* === Interrupt delivery ========================================= */
static void ot_alert_handler_qp_update_irqs(OtAlertHandlerQpState *s)
{
    /* The IRQ lines reflect the SETTLED device state: an external
     * stimulus (pin edge, chardev push, SPI byte) may have moved the
     * model only one clock (synchronizer / edge-detect / INTR_STATE
     * latch still propagating), and a bus read samples the register
     * on the request clock.  Let the model reach quiescence first —
     * cheap when already settled (one tick that changes nothing). */
    alert_handler_settle(&s->core);
    uint32_t intr_state  = (uint32_t)alert_handler_read(&s->core, 0x0u, 4);
    uint32_t intr_enable = (uint32_t)alert_handler_read(&s->core, 0x4u, 4);
    uint32_t masked = intr_state & intr_enable;
    for (unsigned i = 0; i < OT_ALERT_HANDLER_QP_IRQ_NUM; i++) {
        ibex_irq_set(&s->irqs[i], (int)((masked >> i) & 1u));
    }
}

/* === Alert-source inputs (alert_in role) ======================== */
static void ot_alert_handler_qp_set_alert_in(void *opaque, int n, int level)
{
    OtAlertHandlerQpState *s = OT_ALERT_HANDLER_QP(opaque);
    if (n < 0 || n >= 65) return;
    switch (n) {
    case 0: s->core.alert_tx_i_0__alert_p = level ? 1 : 0; s->core.alert_tx_i_0__alert_n = level ? 0 : 1; break;
    case 1: s->core.alert_tx_i_1__alert_p = level ? 1 : 0; s->core.alert_tx_i_1__alert_n = level ? 0 : 1; break;
    case 2: s->core.alert_tx_i_2__alert_p = level ? 1 : 0; s->core.alert_tx_i_2__alert_n = level ? 0 : 1; break;
    case 3: s->core.alert_tx_i_3__alert_p = level ? 1 : 0; s->core.alert_tx_i_3__alert_n = level ? 0 : 1; break;
    case 4: s->core.alert_tx_i_4__alert_p = level ? 1 : 0; s->core.alert_tx_i_4__alert_n = level ? 0 : 1; break;
    case 5: s->core.alert_tx_i_5__alert_p = level ? 1 : 0; s->core.alert_tx_i_5__alert_n = level ? 0 : 1; break;
    case 6: s->core.alert_tx_i_6__alert_p = level ? 1 : 0; s->core.alert_tx_i_6__alert_n = level ? 0 : 1; break;
    case 7: s->core.alert_tx_i_7__alert_p = level ? 1 : 0; s->core.alert_tx_i_7__alert_n = level ? 0 : 1; break;
    case 8: s->core.alert_tx_i_8__alert_p = level ? 1 : 0; s->core.alert_tx_i_8__alert_n = level ? 0 : 1; break;
    case 9: s->core.alert_tx_i_9__alert_p = level ? 1 : 0; s->core.alert_tx_i_9__alert_n = level ? 0 : 1; break;
    case 10: s->core.alert_tx_i_10__alert_p = level ? 1 : 0; s->core.alert_tx_i_10__alert_n = level ? 0 : 1; break;
    case 11: s->core.alert_tx_i_11__alert_p = level ? 1 : 0; s->core.alert_tx_i_11__alert_n = level ? 0 : 1; break;
    case 12: s->core.alert_tx_i_12__alert_p = level ? 1 : 0; s->core.alert_tx_i_12__alert_n = level ? 0 : 1; break;
    case 13: s->core.alert_tx_i_13__alert_p = level ? 1 : 0; s->core.alert_tx_i_13__alert_n = level ? 0 : 1; break;
    case 14: s->core.alert_tx_i_14__alert_p = level ? 1 : 0; s->core.alert_tx_i_14__alert_n = level ? 0 : 1; break;
    case 15: s->core.alert_tx_i_15__alert_p = level ? 1 : 0; s->core.alert_tx_i_15__alert_n = level ? 0 : 1; break;
    case 16: s->core.alert_tx_i_16__alert_p = level ? 1 : 0; s->core.alert_tx_i_16__alert_n = level ? 0 : 1; break;
    case 17: s->core.alert_tx_i_17__alert_p = level ? 1 : 0; s->core.alert_tx_i_17__alert_n = level ? 0 : 1; break;
    case 18: s->core.alert_tx_i_18__alert_p = level ? 1 : 0; s->core.alert_tx_i_18__alert_n = level ? 0 : 1; break;
    case 19: s->core.alert_tx_i_19__alert_p = level ? 1 : 0; s->core.alert_tx_i_19__alert_n = level ? 0 : 1; break;
    case 20: s->core.alert_tx_i_20__alert_p = level ? 1 : 0; s->core.alert_tx_i_20__alert_n = level ? 0 : 1; break;
    case 21: s->core.alert_tx_i_21__alert_p = level ? 1 : 0; s->core.alert_tx_i_21__alert_n = level ? 0 : 1; break;
    case 22: s->core.alert_tx_i_22__alert_p = level ? 1 : 0; s->core.alert_tx_i_22__alert_n = level ? 0 : 1; break;
    case 23: s->core.alert_tx_i_23__alert_p = level ? 1 : 0; s->core.alert_tx_i_23__alert_n = level ? 0 : 1; break;
    case 24: s->core.alert_tx_i_24__alert_p = level ? 1 : 0; s->core.alert_tx_i_24__alert_n = level ? 0 : 1; break;
    case 25: s->core.alert_tx_i_25__alert_p = level ? 1 : 0; s->core.alert_tx_i_25__alert_n = level ? 0 : 1; break;
    case 26: s->core.alert_tx_i_26__alert_p = level ? 1 : 0; s->core.alert_tx_i_26__alert_n = level ? 0 : 1; break;
    case 27: s->core.alert_tx_i_27__alert_p = level ? 1 : 0; s->core.alert_tx_i_27__alert_n = level ? 0 : 1; break;
    case 28: s->core.alert_tx_i_28__alert_p = level ? 1 : 0; s->core.alert_tx_i_28__alert_n = level ? 0 : 1; break;
    case 29: s->core.alert_tx_i_29__alert_p = level ? 1 : 0; s->core.alert_tx_i_29__alert_n = level ? 0 : 1; break;
    case 30: s->core.alert_tx_i_30__alert_p = level ? 1 : 0; s->core.alert_tx_i_30__alert_n = level ? 0 : 1; break;
    case 31: s->core.alert_tx_i_31__alert_p = level ? 1 : 0; s->core.alert_tx_i_31__alert_n = level ? 0 : 1; break;
    case 32: s->core.alert_tx_i_32__alert_p = level ? 1 : 0; s->core.alert_tx_i_32__alert_n = level ? 0 : 1; break;
    case 33: s->core.alert_tx_i_33__alert_p = level ? 1 : 0; s->core.alert_tx_i_33__alert_n = level ? 0 : 1; break;
    case 34: s->core.alert_tx_i_34__alert_p = level ? 1 : 0; s->core.alert_tx_i_34__alert_n = level ? 0 : 1; break;
    case 35: s->core.alert_tx_i_35__alert_p = level ? 1 : 0; s->core.alert_tx_i_35__alert_n = level ? 0 : 1; break;
    case 36: s->core.alert_tx_i_36__alert_p = level ? 1 : 0; s->core.alert_tx_i_36__alert_n = level ? 0 : 1; break;
    case 37: s->core.alert_tx_i_37__alert_p = level ? 1 : 0; s->core.alert_tx_i_37__alert_n = level ? 0 : 1; break;
    case 38: s->core.alert_tx_i_38__alert_p = level ? 1 : 0; s->core.alert_tx_i_38__alert_n = level ? 0 : 1; break;
    case 39: s->core.alert_tx_i_39__alert_p = level ? 1 : 0; s->core.alert_tx_i_39__alert_n = level ? 0 : 1; break;
    case 40: s->core.alert_tx_i_40__alert_p = level ? 1 : 0; s->core.alert_tx_i_40__alert_n = level ? 0 : 1; break;
    case 41: s->core.alert_tx_i_41__alert_p = level ? 1 : 0; s->core.alert_tx_i_41__alert_n = level ? 0 : 1; break;
    case 42: s->core.alert_tx_i_42__alert_p = level ? 1 : 0; s->core.alert_tx_i_42__alert_n = level ? 0 : 1; break;
    case 43: s->core.alert_tx_i_43__alert_p = level ? 1 : 0; s->core.alert_tx_i_43__alert_n = level ? 0 : 1; break;
    case 44: s->core.alert_tx_i_44__alert_p = level ? 1 : 0; s->core.alert_tx_i_44__alert_n = level ? 0 : 1; break;
    case 45: s->core.alert_tx_i_45__alert_p = level ? 1 : 0; s->core.alert_tx_i_45__alert_n = level ? 0 : 1; break;
    case 46: s->core.alert_tx_i_46__alert_p = level ? 1 : 0; s->core.alert_tx_i_46__alert_n = level ? 0 : 1; break;
    case 47: s->core.alert_tx_i_47__alert_p = level ? 1 : 0; s->core.alert_tx_i_47__alert_n = level ? 0 : 1; break;
    case 48: s->core.alert_tx_i_48__alert_p = level ? 1 : 0; s->core.alert_tx_i_48__alert_n = level ? 0 : 1; break;
    case 49: s->core.alert_tx_i_49__alert_p = level ? 1 : 0; s->core.alert_tx_i_49__alert_n = level ? 0 : 1; break;
    case 50: s->core.alert_tx_i_50__alert_p = level ? 1 : 0; s->core.alert_tx_i_50__alert_n = level ? 0 : 1; break;
    case 51: s->core.alert_tx_i_51__alert_p = level ? 1 : 0; s->core.alert_tx_i_51__alert_n = level ? 0 : 1; break;
    case 52: s->core.alert_tx_i_52__alert_p = level ? 1 : 0; s->core.alert_tx_i_52__alert_n = level ? 0 : 1; break;
    case 53: s->core.alert_tx_i_53__alert_p = level ? 1 : 0; s->core.alert_tx_i_53__alert_n = level ? 0 : 1; break;
    case 54: s->core.alert_tx_i_54__alert_p = level ? 1 : 0; s->core.alert_tx_i_54__alert_n = level ? 0 : 1; break;
    case 55: s->core.alert_tx_i_55__alert_p = level ? 1 : 0; s->core.alert_tx_i_55__alert_n = level ? 0 : 1; break;
    case 56: s->core.alert_tx_i_56__alert_p = level ? 1 : 0; s->core.alert_tx_i_56__alert_n = level ? 0 : 1; break;
    case 57: s->core.alert_tx_i_57__alert_p = level ? 1 : 0; s->core.alert_tx_i_57__alert_n = level ? 0 : 1; break;
    case 58: s->core.alert_tx_i_58__alert_p = level ? 1 : 0; s->core.alert_tx_i_58__alert_n = level ? 0 : 1; break;
    case 59: s->core.alert_tx_i_59__alert_p = level ? 1 : 0; s->core.alert_tx_i_59__alert_n = level ? 0 : 1; break;
    case 60: s->core.alert_tx_i_60__alert_p = level ? 1 : 0; s->core.alert_tx_i_60__alert_n = level ? 0 : 1; break;
    case 61: s->core.alert_tx_i_61__alert_p = level ? 1 : 0; s->core.alert_tx_i_61__alert_n = level ? 0 : 1; break;
    case 62: s->core.alert_tx_i_62__alert_p = level ? 1 : 0; s->core.alert_tx_i_62__alert_n = level ? 0 : 1; break;
    case 63: s->core.alert_tx_i_63__alert_p = level ? 1 : 0; s->core.alert_tx_i_63__alert_n = level ? 0 : 1; break;
    case 64: s->core.alert_tx_i_64__alert_p = level ? 1 : 0; s->core.alert_tx_i_64__alert_n = level ? 0 : 1; break;
    }
    if (s->core._qp_busy) return;   /* asynchronous-input rule */
    alert_handler_settle(&s->core);
    ot_alert_handler_qp_update_irqs(s);
}

/* Generic core accessor: SoC-integration bridges (device-to-device
 * signal links wired in the machine file) reach the generated state
 * through this + the <dev>.h field API. */
void *ot_alert_handler_qp_core(DeviceState *dev)
{
    return &OT_ALERT_HANDLER_QP(dev)->core;
}

void ot_alert_handler_qp_set_settle_hook(DeviceState *dev, int (*fn)(void *), void *ctx)
{
    OT_ALERT_HANDLER_QP(dev)->core._qp_settle_hook = fn;
    OT_ALERT_HANDLER_QP(dev)->core._qp_settle_hook_ctx = ctx;
}

static uint64_t ot_alert_handler_qp_read(void *opaque, hwaddr addr, unsigned size)
{
    OtAlertHandlerQpState *s = OT_ALERT_HANDLER_QP(opaque);
    return alert_handler_read(&s->core, addr, size);
}

static void ot_alert_handler_qp_write(void *opaque, hwaddr addr, uint64_t value,
                                      unsigned size)
{
    OtAlertHandlerQpState *s = OT_ALERT_HANDLER_QP(opaque);
    alert_handler_write(&s->core, addr, value, size);
    ot_alert_handler_qp_update_irqs(s);
}

static const MemoryRegionOps ot_alert_handler_qp_ops = {
    .read = ot_alert_handler_qp_read,
    .write = ot_alert_handler_qp_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
};

static const Property ot_alert_handler_qp_properties[] = {
    DEFINE_PROP_STRING(OT_COMMON_DEV_ID, OtAlertHandlerQpState, ot_id),
};

static void ot_alert_handler_qp_realize(DeviceState *dev, Error **errp)
{
    /* Backend hookup intentionally absent — frontend-only milestone. */
    (void)dev;
    (void)errp;
}

static void ot_alert_handler_qp_init(Object *obj)
{
    OtAlertHandlerQpState *s = OT_ALERT_HANDLER_QP(obj);

    for (unsigned i = 0; i < OT_ALERT_HANDLER_QP_IRQ_NUM; i++) {
        ibex_sysbus_init_irq(obj, &s->irqs[i]);
    }
    qdev_init_gpio_in_named(DEVICE(obj), ot_alert_handler_qp_set_alert_in, OT_DEVICE_ALERT, 65);

    memory_region_init_io(&s->mmio, obj, &ot_alert_handler_qp_ops, s,
                          TYPE_OT_ALERT_HANDLER_QP, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    /* Pulse reset to commit RESVALs into every prim_subreg.
     * Pattern in generated code:  q = (~rst_ni) ? RESVAL : keep.
     * C-init leaves rst_ni at 0, so a settle round with rst
     * active forces q := RESVAL across every register.  Then
     * release reset by setting rst_ni high.  Without this pulse,
     * registers like REGWEN (RESVAL=1) stay at their C init=0. */
    alert_handler_reset(&s->core);
}

static void ot_alert_handler_qp_finalize(Object *obj)
{
    (void)obj;
}

static void ot_alert_handler_qp_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    (void)data;

    dc->realize = ot_alert_handler_qp_realize;
    device_class_set_props(dc, ot_alert_handler_qp_properties);
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}
