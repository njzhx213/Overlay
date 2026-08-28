#ifndef HW_OPENTITAN_AON_TIMER_QP_SHIM_H
#define HW_OPENTITAN_AON_TIMER_QP_SHIM_H

#include "qom/object.h"

#define TYPE_OT_AON_TIMER_QP "ot-aon-timer-qp"
OBJECT_DECLARE_SIMPLE_TYPE(OtAonTimerQpState, OT_AON_TIMER_QP)

/* Generic core accessor for machine-level device-to-device bridges:
 * returns the embedded <dev>_state (see qemu_passes/aon_timer.h). */
void *ot_aon_timer_qp_core(DeviceState *dev);

#endif /* HW_OPENTITAN_AON_TIMER_QP_SHIM_H */
