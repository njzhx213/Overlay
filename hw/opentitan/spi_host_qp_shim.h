#ifndef HW_OPENTITAN_SPI_HOST_QP_SHIM_H
#define HW_OPENTITAN_SPI_HOST_QP_SHIM_H

#include "qom/object.h"

#define TYPE_OT_SPI_HOST_QP "ot-spi-host-qp"
OBJECT_DECLARE_SIMPLE_TYPE(OtSPIHostQpState, OT_SPI_HOST_QP)

/* Generic core accessor for machine-level device-to-device bridges:
 * returns the embedded <dev>_state (see qemu_passes/spi_host.h). */
void *ot_spi_host_qp_core(DeviceState *dev);

/* Settle-hook arming (ring-table form: soc_glue arms members by
 * name, one call per ring member). */
void ot_spi_host_qp_set_settle_hook(DeviceState *dev, int (*fn)(void *), void *ctx);

#endif /* HW_OPENTITAN_SPI_HOST_QP_SHIM_H */
