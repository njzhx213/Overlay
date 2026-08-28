#ifndef HW_OPENTITAN_RV_PLIC_QP_SHIM_H
#define HW_OPENTITAN_RV_PLIC_QP_SHIM_H

#include "qom/object.h"

#define TYPE_OT_RV_PLIC_QP "ot-rv-plic-qp"
OBJECT_DECLARE_SIMPLE_TYPE(OtRvPlicQpState, OT_RV_PLIC_QP)

/* Generic core accessor for machine-level device-to-device bridges:
 * returns the embedded <dev>_state (see qemu_passes/rv_plic.h). */
void *ot_rv_plic_qp_core(DeviceState *dev);

#endif /* HW_OPENTITAN_RV_PLIC_QP_SHIM_H */
