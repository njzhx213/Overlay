#ifndef HW_OPENTITAN_UART_QP_SHIM_H
#define HW_OPENTITAN_UART_QP_SHIM_H

#include "qom/object.h"

#define TYPE_OT_UART_QP "ot-uart-qp"
OBJECT_DECLARE_SIMPLE_TYPE(OtUARTQpState, OT_UART_QP)

/* Generic core accessor for machine-level device-to-device bridges:
 * returns the embedded <dev>_state (see qemu_passes/uart.h). */
void *ot_uart_qp_core(DeviceState *dev);

#endif /* HW_OPENTITAN_UART_QP_SHIM_H */
