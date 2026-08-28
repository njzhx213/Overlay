#ifndef HW_OPENTITAN_RV_TIMER_QP_SHIM_H
#define HW_OPENTITAN_RV_TIMER_QP_SHIM_H

#include "qom/object.h"

#define TYPE_OT_RV_TIMER_QP "ot-rv-timer-qp"
OBJECT_DECLARE_SIMPLE_TYPE(OtRvTimerQpState, OT_RV_TIMER_QP)

/* Generic core accessor for machine-level device-to-device bridges:
 * returns the embedded <dev>_state (see qemu_passes/rv_timer.h). */
void *ot_rv_timer_qp_core(DeviceState *dev);

#endif /* HW_OPENTITAN_RV_TIMER_QP_SHIM_H */
