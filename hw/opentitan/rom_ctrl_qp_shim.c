/*
 * QEMU OpenTitan ROM_CTRL device — qemu-passes shim (auto-generated)
 *
 * DO NOT EDIT.  Re-emit by re-running qemu-passes with --qemu-emit-c
 * after changing the kKnownDevices catalog in lib/QEMUEmitCPass.cpp.
 *
 * Frontend wrapper around the auto-generated ROM_CTRL model from
 *   hw/opentitan/qemu_passes/rom_ctrl.{c,h}
 *
 * MMIO read/write callbacks delegate to the generated entrypoints
 * rom_ctrl_read() / rom_ctrl_write().  The QEMU backend
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

/* Generated rom_ctrl model entrypoints + state struct. */
#include "qemu_passes/rom_ctrl.h"

/* This shim's own type declaration. */
#include "hw/opentitan/rom_ctrl_qp_shim.h"

#define OT_ROM_CTRL_QP_IRQ_NUM 0u

struct OtRomCtrlQpState {
    SysBusDevice parent_obj;
    MemoryRegion mmio;
    MemoryRegion mmio_regs_tl;
    char *backing0_image;
    IbexIRQ irqs[OT_ROM_CTRL_QP_IRQ_NUM];
    IbexIRQ alert;

    /* Property fields (mirror upstream ot_rom_ctrl_eg.c so SoC-table-injected
     * values are accepted by QOM).  We accept-but-ignore: the
     * generated model has its own initial state and no chardev wiring. */
    char * ot_id;

    /* Embedded auto-generated device state. */
    rom_ctrl_state core;
};

OBJECT_DEFINE_SIMPLE_TYPE(OtRomCtrlQpState, ot_rom_ctrl_qp, OT_ROM_CTRL_QP, SYS_BUS_DEVICE)

/* Generic core accessor: SoC-integration bridges (device-to-device
 * signal links wired in the machine file) reach the generated state
 * through this + the <dev>.h field API. */
void *ot_rom_ctrl_qp_core(DeviceState *dev)
{
    return &OT_ROM_CTRL_QP(dev)->core;
}

void ot_rom_ctrl_qp_set_settle_hook(DeviceState *dev, int (*fn)(void *), void *ctx)
{
    OT_ROM_CTRL_QP(dev)->core._qp_settle_hook = fn;
    OT_ROM_CTRL_QP(dev)->core._qp_settle_hook_ctx = ctx;
}

static uint64_t ot_rom_ctrl_qp_read(void *opaque, hwaddr addr, unsigned size)
{
    OtRomCtrlQpState *s = OT_ROM_CTRL_QP(opaque);
    return rom_ctrl_read(&s->core, addr, size);
}

static void ot_rom_ctrl_qp_write(void *opaque, hwaddr addr, uint64_t value,
                                 unsigned size)
{
    OtRomCtrlQpState *s = OT_ROM_CTRL_QP(opaque);
    rom_ctrl_write(&s->core, addr, value, size);
}

static uint64_t ot_rom_ctrl_qp_regs_tl_read(void *opaque, hwaddr addr, unsigned size)
{
    OtRomCtrlQpState *s = OT_ROM_CTRL_QP(opaque);
    return rom_ctrl_regs_tl_read(&s->core, addr, size);
}

static void ot_rom_ctrl_qp_regs_tl_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
    OtRomCtrlQpState *s = OT_ROM_CTRL_QP(opaque);
    rom_ctrl_regs_tl_write(&s->core, addr, value, size);
}

static const MemoryRegionOps ot_rom_ctrl_qp_regs_tl_ops = {
    .read = ot_rom_ctrl_qp_regs_tl_read,
    .write = ot_rom_ctrl_qp_regs_tl_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
};

static const MemoryRegionOps ot_rom_ctrl_qp_ops = {
    .read = ot_rom_ctrl_qp_read,
    .write = ot_rom_ctrl_qp_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
};

static const Property ot_rom_ctrl_qp_properties[] = {
    DEFINE_PROP_STRING("backing0-image", OtRomCtrlQpState, backing0_image),
    DEFINE_PROP_STRING(OT_COMMON_DEV_ID, OtRomCtrlQpState, ot_id),
};

static void ot_rom_ctrl_qp_realize(DeviceState *dev, Error **errp)
{
    OtRomCtrlQpState *s = OT_ROM_CTRL_QP(dev);
    (void)errp;
    if (s->backing0_image) {
        gchar *_img; gsize _n;
        if (g_file_get_contents(s->backing0_image, &_img, &_n, NULL)) {
            if (rom_ctrl_load_backing(&s->core, 0, _img, _n) < 0)
                error_setg(errp, "backing0 image too large");
            g_free(_img);
        } else {
            error_setg(errp, "cannot read backing0 image %s", s->backing0_image);
        }
    }
}

static void ot_rom_ctrl_qp_init(Object *obj)
{
    OtRomCtrlQpState *s = OT_ROM_CTRL_QP(obj);

    /* This device has 0 IRQs — no IRQ init loop. */
    ibex_qdev_init_irq(obj, &s->alert, OT_DEVICE_ALERT);

    memory_region_init_io(&s->mmio, obj, &ot_rom_ctrl_qp_ops, s,
                          TYPE_OT_ROM_CTRL_QP, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    memory_region_init_io(&s->mmio_regs_tl, obj, &ot_rom_ctrl_qp_regs_tl_ops, s,
                          TYPE_OT_ROM_CTRL_QP "-regs_tl", 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio_regs_tl);

    /* Binding-declared input ties: constants on top-level inputs the
     * SoC model does not integrate (e.g. an always-acking EDN
     * entropy bus, a lifecycle-escalate line held at Off) so
     * handshake-waiting FSMs are not deadlocked.  Applied BEFORE
     * the reset pulse as well: sparse security FSMs sample these
     * on the reset-release tick and latch a terminal error state
     * if a tied line still reads its (illegal) zero init. */
    rom_ctrl_set_rom_cfg_i_cfg(&s->core, 0x0u);
    rom_ctrl_set_rom_cfg_i_cfg_en(&s->core, 0x0u);
    rom_ctrl_set_rom_cfg_i_test(&s->core, 0x0u);
    /* Pulse reset to commit RESVALs into every prim_subreg.
     * Pattern in generated code:  q = (~rst_ni) ? RESVAL : keep.
     * C-init leaves rst_ni at 0, so a settle round with rst
     * active forces q := RESVAL across every register.  Then
     * release reset by setting rst_ni high.  Without this pulse,
     * registers like REGWEN (RESVAL=1) stay at their C init=0. */
    rom_ctrl_reset(&s->core);
    rom_ctrl_set_rom_cfg_i_cfg(&s->core, 0x0u);
    rom_ctrl_set_rom_cfg_i_cfg_en(&s->core, 0x0u);
    rom_ctrl_set_rom_cfg_i_test(&s->core, 0x0u);
    rom_ctrl_settle(&s->core);
}

static void ot_rom_ctrl_qp_finalize(Object *obj)
{
    (void)obj;
}

static void ot_rom_ctrl_qp_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    (void)data;

    dc->realize = ot_rom_ctrl_qp_realize;
    device_class_set_props(dc, ot_rom_ctrl_qp_properties);
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}
