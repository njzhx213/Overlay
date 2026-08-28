#ifndef HW_OPENTITAN_KMAC_QP_SHIM_H
#define HW_OPENTITAN_KMAC_QP_SHIM_H

#include "qom/object.h"

#define TYPE_OT_KMAC_QP "ot-kmac-qp"
OBJECT_DECLARE_SIMPLE_TYPE(OtKmacQpState, OT_KMAC_QP)

/* Generic core accessor for machine-level device-to-device bridges:
 * returns the embedded <dev>_state (see qemu_passes/kmac.h). */
void *ot_kmac_qp_core(DeviceState *dev);

#endif /* HW_OPENTITAN_KMAC_QP_SHIM_H */
