#ifndef HW_OPENTITAN_ENTROPY_SRC_QP_SHIM_H
#define HW_OPENTITAN_ENTROPY_SRC_QP_SHIM_H

#include "qom/object.h"

#define TYPE_OT_ENTROPY_SRC_QP "ot-entropy-src-qp"
OBJECT_DECLARE_SIMPLE_TYPE(OtEntropySrcQpState, OT_ENTROPY_SRC_QP)

/* Generic core accessor for machine-level device-to-device bridges:
 * returns the embedded <dev>_state (see qemu_passes/entropy_src.h). */
void *ot_entropy_src_qp_core(DeviceState *dev);

/* Settle-hook arming (ring-table form: soc_glue arms members by
 * name, one call per ring member). */
void ot_entropy_src_qp_set_settle_hook(DeviceState *dev, int (*fn)(void *), void *ctx);

#endif /* HW_OPENTITAN_ENTROPY_SRC_QP_SHIM_H */
