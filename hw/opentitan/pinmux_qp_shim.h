#ifndef HW_OPENTITAN_PINMUX_QP_SHIM_H
#define HW_OPENTITAN_PINMUX_QP_SHIM_H

#include "qom/object.h"

#define TYPE_OT_PINMUX_QP "ot-pinmux-qp"
OBJECT_DECLARE_SIMPLE_TYPE(OtPinmuxQpState, OT_PINMUX_QP)

/* Generic core accessor for machine-level device-to-device bridges:
 * returns the embedded <dev>_state (see qemu_passes/pinmux.h). */
void *ot_pinmux_qp_core(DeviceState *dev);

#endif /* HW_OPENTITAN_PINMUX_QP_SHIM_H */
