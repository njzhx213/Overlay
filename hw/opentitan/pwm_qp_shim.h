#ifndef HW_OPENTITAN_PWM_QP_SHIM_H
#define HW_OPENTITAN_PWM_QP_SHIM_H

#include "qom/object.h"

#define TYPE_OT_PWM_QP "ot-pwm-qp"
OBJECT_DECLARE_SIMPLE_TYPE(OtPwmQpState, OT_PWM_QP)

/* Generic core accessor for machine-level device-to-device bridges:
 * returns the embedded <dev>_state (see qemu_passes/pwm.h). */
void *ot_pwm_qp_core(DeviceState *dev);

#endif /* HW_OPENTITAN_PWM_QP_SHIM_H */
