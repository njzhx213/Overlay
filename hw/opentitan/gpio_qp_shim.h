#ifndef HW_OPENTITAN_GPIO_QP_SHIM_H
#define HW_OPENTITAN_GPIO_QP_SHIM_H

#include "qom/object.h"

#define TYPE_OT_GPIO_QP "ot-gpio-qp"
OBJECT_DECLARE_SIMPLE_TYPE(OtGpioQpState, OT_GPIO_QP)

/* Generic core accessor for machine-level device-to-device bridges:
 * returns the embedded <dev>_state (see qemu_passes/gpio.h). */
void *ot_gpio_qp_core(DeviceState *dev);

/* Settle-hook arming (ring-table form: soc_glue arms members by
 * name, one call per ring member). */
void ot_gpio_qp_set_settle_hook(DeviceState *dev, int (*fn)(void *), void *ctx);

#endif /* HW_OPENTITAN_GPIO_QP_SHIM_H */
