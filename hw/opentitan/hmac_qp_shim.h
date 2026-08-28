#ifndef HW_OPENTITAN_HMAC_QP_SHIM_H
#define HW_OPENTITAN_HMAC_QP_SHIM_H

#include "qom/object.h"

#define TYPE_OT_HMAC_QP "ot-hmac-qp"
OBJECT_DECLARE_SIMPLE_TYPE(OtHmacQpState, OT_HMAC_QP)

/* Generic core accessor for machine-level device-to-device bridges:
 * returns the embedded <dev>_state (see qemu_passes/hmac.h). */
void *ot_hmac_qp_core(DeviceState *dev);

#endif /* HW_OPENTITAN_HMAC_QP_SHIM_H */
