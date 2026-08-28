#ifndef HW_OPENTITAN_SPI_DEVICE_QP_SHIM_H
#define HW_OPENTITAN_SPI_DEVICE_QP_SHIM_H

#include "qom/object.h"

#define TYPE_OT_SPI_DEVICE_QP "ot-spi-device-qp"
OBJECT_DECLARE_SIMPLE_TYPE(OtSPIDeviceQpState, OT_SPI_DEVICE_QP)

/* Generic core accessor for machine-level device-to-device bridges:
 * returns the embedded <dev>_state (see qemu_passes/spi_device.h). */
void *ot_spi_device_qp_core(DeviceState *dev);

#endif /* HW_OPENTITAN_SPI_DEVICE_QP_SHIM_H */
