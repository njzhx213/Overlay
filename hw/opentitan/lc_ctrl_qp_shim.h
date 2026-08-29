#ifndef HW_OPENTITAN_LC_CTRL_QP_SHIM_H
#define HW_OPENTITAN_LC_CTRL_QP_SHIM_H

#include "qom/object.h"

#define TYPE_OT_LC_CTRL_QP "ot-lc-ctrl-qp"
OBJECT_DECLARE_SIMPLE_TYPE(OtLcCtrlQpState, OT_LC_CTRL_QP)

/* Generic core accessor for machine-level device-to-device bridges:
 * returns the embedded <dev>_state (see qemu_passes/lc_ctrl.h). */
void *ot_lc_ctrl_qp_core(DeviceState *dev);

/* Settle-hook arming (ring-table form: soc_glue arms members by
 * name, one call per ring member). */
void ot_lc_ctrl_qp_set_settle_hook(DeviceState *dev, int (*fn)(void *), void *ctx);

#endif /* HW_OPENTITAN_LC_CTRL_QP_SHIM_H */
