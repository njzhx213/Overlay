#ifndef HW_OPENTITAN_SYSRST_CTRL_QP_SHIM_H
#define HW_OPENTITAN_SYSRST_CTRL_QP_SHIM_H

#include "qom/object.h"

#define TYPE_OT_SYSRST_CTRL_QP "ot-sysrst-ctrl-qp"
OBJECT_DECLARE_SIMPLE_TYPE(OtSysrstCtrlQpState, OT_SYSRST_CTRL_QP)

/* Generic core accessor for machine-level device-to-device bridges:
 * returns the embedded <dev>_state (see qemu_passes/sysrst_ctrl.h). */
void *ot_sysrst_ctrl_qp_core(DeviceState *dev);

/* Settle-hook arming (ring-table form: soc_glue arms members by
 * name, one call per ring member). */
void ot_sysrst_ctrl_qp_set_settle_hook(DeviceState *dev, int (*fn)(void *), void *ctx);

#endif /* HW_OPENTITAN_SYSRST_CTRL_QP_SHIM_H */
