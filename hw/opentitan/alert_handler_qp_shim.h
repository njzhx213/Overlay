#ifndef HW_OPENTITAN_ALERT_HANDLER_QP_SHIM_H
#define HW_OPENTITAN_ALERT_HANDLER_QP_SHIM_H

#include "qom/object.h"

#define TYPE_OT_ALERT_HANDLER_QP "ot-alert-handler-qp"
OBJECT_DECLARE_SIMPLE_TYPE(OtAlertHandlerQpState, OT_ALERT_HANDLER_QP)

/* Generic core accessor for machine-level device-to-device bridges:
 * returns the embedded <dev>_state (see qemu_passes/alert_handler.h). */
void *ot_alert_handler_qp_core(DeviceState *dev);

#endif /* HW_OPENTITAN_ALERT_HANDLER_QP_SHIM_H */
