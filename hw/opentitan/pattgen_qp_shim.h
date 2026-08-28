#ifndef HW_OPENTITAN_PATTGEN_QP_SHIM_H
#define HW_OPENTITAN_PATTGEN_QP_SHIM_H

#include "qom/object.h"

#define TYPE_OT_PATTGEN_QP "ot-pattgen-qp"
OBJECT_DECLARE_SIMPLE_TYPE(OtPattgenQpState, OT_PATTGEN_QP)

/* Generic core accessor for machine-level device-to-device bridges:
 * returns the embedded <dev>_state (see qemu_passes/pattgen.h). */
void *ot_pattgen_qp_core(DeviceState *dev);

#endif /* HW_OPENTITAN_PATTGEN_QP_SHIM_H */
