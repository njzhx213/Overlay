#ifndef HW_OPENTITAN_KEYMGR_QP_SHIM_H
#define HW_OPENTITAN_KEYMGR_QP_SHIM_H

#include "qom/object.h"

#define TYPE_OT_KEYMGR_QP "ot-keymgr-qp"
OBJECT_DECLARE_SIMPLE_TYPE(OtKeymgrQpState, OT_KEYMGR_QP)

/* Generic core accessor for machine-level device-to-device bridges:
 * returns the embedded <dev>_state (see qemu_passes/keymgr.h). */
void *ot_keymgr_qp_core(DeviceState *dev);

#endif /* HW_OPENTITAN_KEYMGR_QP_SHIM_H */
