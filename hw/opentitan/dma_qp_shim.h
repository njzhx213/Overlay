#ifndef HW_OPENTITAN_DMA_QP_SHIM_H
#define HW_OPENTITAN_DMA_QP_SHIM_H

#include "qom/object.h"

#define TYPE_OT_DMA_QP "ot-dma-qp"
OBJECT_DECLARE_SIMPLE_TYPE(OtDmaQpState, OT_DMA_QP)

/* Generic core accessor for machine-level device-to-device bridges:
 * returns the embedded <dev>_state (see qemu_passes/dma.h). */
void *ot_dma_qp_core(DeviceState *dev);

#endif /* HW_OPENTITAN_DMA_QP_SHIM_H */
