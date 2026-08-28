#ifndef HW_OPENTITAN_EDN_QP_SHIM_H
#define HW_OPENTITAN_EDN_QP_SHIM_H

#include "qom/object.h"

#define TYPE_OT_EDN_QP "ot-edn-qp"
OBJECT_DECLARE_SIMPLE_TYPE(OtEdnQpState, OT_EDN_QP)

/* Generic core accessor for machine-level device-to-device bridges:
 * returns the embedded <dev>_state (see qemu_passes/edn.h). */
void *ot_edn_qp_core(DeviceState *dev);

#endif /* HW_OPENTITAN_EDN_QP_SHIM_H */
