/*
 * QEMU OpenTitan LC_CTRL device — qemu-passes shim (auto-generated)
 *
 * DO NOT EDIT.  Re-emit by re-running qemu-passes with --qemu-emit-c
 * after changing the kKnownDevices catalog in lib/QEMUEmitCPass.cpp.
 *
 * Frontend wrapper around the auto-generated LC_CTRL model from
 *   hw/opentitan/qemu_passes/lc_ctrl.{c,h}
 *
 * MMIO read/write callbacks delegate to the generated entrypoints
 * lc_ctrl_read() / lc_ctrl_write().  The QEMU backend
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

/* Generated lc_ctrl model entrypoints + state struct. */
#include "qemu_passes/lc_ctrl.h"

/* This shim's own type declaration. */
#include "hw/opentitan/lc_ctrl_qp_shim.h"

#define OT_LC_CTRL_QP_IRQ_NUM 0u

struct OtLcCtrlQpState {
    SysBusDevice parent_obj;
    MemoryRegion mmio;
    MemoryRegion mmio_regs_tl;
    IbexIRQ irqs[OT_LC_CTRL_QP_IRQ_NUM];
    IbexIRQ alert;

    /* Property fields (mirror upstream ot_lc_ctrl_eg.c so SoC-table-injected
     * values are accepted by QOM).  We accept-but-ignore: the
     * generated model has its own initial state and no chardev wiring. */
    char * ot_id;

    /* Embedded auto-generated device state. */
    lc_ctrl_state core;
};

OBJECT_DEFINE_SIMPLE_TYPE(OtLcCtrlQpState, ot_lc_ctrl_qp, OT_LC_CTRL_QP, SYS_BUS_DEVICE)

/* Generic core accessor: SoC-integration bridges (device-to-device
 * signal links wired in the machine file) reach the generated state
 * through this + the <dev>.h field API. */
void *ot_lc_ctrl_qp_core(DeviceState *dev)
{
    return &OT_LC_CTRL_QP(dev)->core;
}

void ot_lc_ctrl_qp_set_settle_hook(DeviceState *dev, int (*fn)(void *), void *ctx)
{
    OT_LC_CTRL_QP(dev)->core._qp_settle_hook = fn;
    OT_LC_CTRL_QP(dev)->core._qp_settle_hook_ctx = ctx;
}

static uint64_t ot_lc_ctrl_qp_read(void *opaque, hwaddr addr, unsigned size)
{
    OtLcCtrlQpState *s = OT_LC_CTRL_QP(opaque);
    return lc_ctrl_read(&s->core, addr, size);
}

static void ot_lc_ctrl_qp_write(void *opaque, hwaddr addr, uint64_t value,
                                unsigned size)
{
    OtLcCtrlQpState *s = OT_LC_CTRL_QP(opaque);
    lc_ctrl_write(&s->core, addr, value, size);
}

static uint64_t ot_lc_ctrl_qp_regs_tl_read(void *opaque, hwaddr addr, unsigned size)
{
    OtLcCtrlQpState *s = OT_LC_CTRL_QP(opaque);
    return lc_ctrl_regs_tl_read(&s->core, addr, size);
}

static void ot_lc_ctrl_qp_regs_tl_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
    OtLcCtrlQpState *s = OT_LC_CTRL_QP(opaque);
    lc_ctrl_regs_tl_write(&s->core, addr, value, size);
}

static const MemoryRegionOps ot_lc_ctrl_qp_regs_tl_ops = {
    .read = ot_lc_ctrl_qp_regs_tl_read,
    .write = ot_lc_ctrl_qp_regs_tl_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
};

static const MemoryRegionOps ot_lc_ctrl_qp_ops = {
    .read = ot_lc_ctrl_qp_read,
    .write = ot_lc_ctrl_qp_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
};

static const Property ot_lc_ctrl_qp_properties[] = {
    DEFINE_PROP_STRING(OT_COMMON_DEV_ID, OtLcCtrlQpState, ot_id),
};

static void ot_lc_ctrl_qp_realize(DeviceState *dev, Error **errp)
{
    /* Backend hookup intentionally absent — frontend-only milestone. */
    (void)dev;
    (void)errp;
}

static void ot_lc_ctrl_qp_init(Object *obj)
{
    OtLcCtrlQpState *s = OT_LC_CTRL_QP(obj);

    /* This device has 0 IRQs — no IRQ init loop. */
    ibex_qdev_init_irq(obj, &s->alert, OT_DEVICE_ALERT);

    memory_region_init_io(&s->mmio, obj, &ot_lc_ctrl_qp_ops, s,
                          TYPE_OT_LC_CTRL_QP, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    memory_region_init_io(&s->mmio_regs_tl, obj, &ot_lc_ctrl_qp_regs_tl_ops, s,
                          TYPE_OT_LC_CTRL_QP "-regs_tl", 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio_regs_tl);

    /* Binding-declared input ties: constants on top-level inputs the
     * SoC model does not integrate (e.g. an always-acking EDN
     * entropy bus, a lifecycle-escalate line held at Off) so
     * handshake-waiting FSMs are not deadlocked.  Applied BEFORE
     * the reset pulse as well: sparse security FSMs sample these
     * on the reset-release tick and latch a terminal error state
     * if a tied line still reads its (illegal) zero init. */
    lc_ctrl_set_esc_scrap_state0_tx_i_resp_n(&s->core, 0x1u);
    lc_ctrl_set_esc_scrap_state0_tx_i_resp_p(&s->core, 0x0u);
    lc_ctrl_set_esc_scrap_state1_tx_i_resp_n(&s->core, 0x1u);
    lc_ctrl_set_esc_scrap_state1_tx_i_resp_p(&s->core, 0x0u);
    lc_ctrl_set_jtag_i_tck(&s->core, 0x0u);
    lc_ctrl_set_jtag_i_tdi(&s->core, 0x0u);
    lc_ctrl_set_jtag_i_tms(&s->core, 0x0u);
    lc_ctrl_set_jtag_i_trst_n(&s->core, 0x1u);
    lc_ctrl_set_lc_clk_byp_ack_i(&s->core, 0xAu);
    lc_ctrl_set_lc_nvm_rma_ack_i_0_(&s->core, 0xAu);
    lc_ctrl_set_lc_nvm_rma_ack_i_1_(&s->core, 0xAu);
    lc_ctrl_set_lc_otp_program_i_ack(&s->core, 0x0u);
    lc_ctrl_set_lc_otp_program_i_err(&s->core, 0x0u);
    lc_ctrl_set_lc_otp_vendor_test_i_status(&s->core, 0x0u);
    lc_ctrl_set_otp_lc_data_i_error(&s->core, 0x0u);
    lc_ctrl_set_otp_lc_data_i_rma_token_valid(&s->core, 0xAu);
    lc_ctrl_set_otp_lc_data_i_secrets_valid(&s->core, 0xAu);
    lc_ctrl_set_otp_lc_data_i_test_tokens_valid(&s->core, 0xAu);
    lc_ctrl_set_otp_lc_data_i_valid(&s->core, 0x1u);
    /* Pulse reset to commit RESVALs into every prim_subreg.
     * Pattern in generated code:  q = (~rst_ni) ? RESVAL : keep.
     * C-init leaves rst_ni at 0, so a settle round with rst
     * active forces q := RESVAL across every register.  Then
     * release reset by setting rst_ni high.  Without this pulse,
     * registers like REGWEN (RESVAL=1) stay at their C init=0. */
    lc_ctrl_reset(&s->core);
    lc_ctrl_set_esc_scrap_state0_tx_i_resp_n(&s->core, 0x1u);
    lc_ctrl_set_esc_scrap_state0_tx_i_resp_p(&s->core, 0x0u);
    lc_ctrl_set_esc_scrap_state1_tx_i_resp_n(&s->core, 0x1u);
    lc_ctrl_set_esc_scrap_state1_tx_i_resp_p(&s->core, 0x0u);
    lc_ctrl_set_jtag_i_tck(&s->core, 0x0u);
    lc_ctrl_set_jtag_i_tdi(&s->core, 0x0u);
    lc_ctrl_set_jtag_i_tms(&s->core, 0x0u);
    lc_ctrl_set_jtag_i_trst_n(&s->core, 0x1u);
    lc_ctrl_set_lc_clk_byp_ack_i(&s->core, 0xAu);
    lc_ctrl_set_lc_nvm_rma_ack_i_0_(&s->core, 0xAu);
    lc_ctrl_set_lc_nvm_rma_ack_i_1_(&s->core, 0xAu);
    lc_ctrl_set_lc_otp_program_i_ack(&s->core, 0x0u);
    lc_ctrl_set_lc_otp_program_i_err(&s->core, 0x0u);
    lc_ctrl_set_lc_otp_vendor_test_i_status(&s->core, 0x0u);
    lc_ctrl_set_otp_lc_data_i_error(&s->core, 0x0u);
    lc_ctrl_set_otp_lc_data_i_rma_token_valid(&s->core, 0xAu);
    lc_ctrl_set_otp_lc_data_i_secrets_valid(&s->core, 0xAu);
    lc_ctrl_set_otp_lc_data_i_test_tokens_valid(&s->core, 0xAu);
    lc_ctrl_set_otp_lc_data_i_valid(&s->core, 0x1u);
    lc_ctrl_settle(&s->core);
}

static void ot_lc_ctrl_qp_finalize(Object *obj)
{
    (void)obj;
}

static void ot_lc_ctrl_qp_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    (void)data;

    dc->realize = ot_lc_ctrl_qp_realize;
    device_class_set_props(dc, ot_lc_ctrl_qp_properties);
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}
