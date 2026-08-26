/*
 * QEMU SSI-to-pin bridge (qemu-passes test scaffolding)
 *
 * A generic SSI peripheral that re-emits each transferred byte as raw
 * SPI pin activity on qdev GPIO lines: "sck" / "csb" / "mosi" outputs,
 * one "miso" input.  Mode 0, MSB first.  It knows nothing about the
 * device on the far side — the SoC wires the lines to any pin-level
 * SPI slave (first user: the generated spi_device's aux pin groups,
 * closing the spi_host -> spi_device self-loop with two generated
 * models and no external client).
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "qemu/osdep.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "hw/ssi/ssi.h"
#include "qemu/log.h"
#include "qemu/module.h"

#define TYPE_OT_SSI_PIN_BRIDGE "ot-ssi-pin-bridge"
OBJECT_DECLARE_SIMPLE_TYPE(OtSSIPinBridgeState, OT_SSI_PIN_BRIDGE)

struct OtSSIPinBridgeState {
    SSIPeripheral parent_obj;

    qemu_irq sck;   /* serial clock out */
    qemu_irq csb;   /* chip select out (active low) */
    qemu_irq mosi;  /* data out to the slave */
    int miso_level; /* last level the slave drove back */
    bool cs_out_low; /* current level we drive on csb */
};

static void ot_ssi_pin_bridge_set_miso(void *opaque, int n, int level)
{
    OtSSIPinBridgeState *s = OT_SSI_PIN_BRIDGE(opaque);
    (void)n;
    s->miso_level = level ? 1 : 0;
}

static uint32_t ot_ssi_pin_bridge_transfer(SSIPeripheral *dev, uint32_t val)
{
    OtSSIPinBridgeState *s = OT_SSI_PIN_BRIDGE(dev);
    uint32_t miso = 0;

    /* Byte-implied select: the SPI-master organ's byte replay can reach us
     * BEFORE the corresponding CS edge is forwarded, so a byte arriving
     * while our csb output is high would be ignored by the slave.  Assert
     * on first byte; the deassert (frame boundary) still comes through
     * set_cs and is forwarded immediately. */
    if (!s->cs_out_low) {
        qemu_set_irq(s->csb, 0);
        s->cs_out_low = true;
    }

    for (int i = 7; i >= 0; i--) {
        qemu_set_irq(s->mosi, (int)((val >> i) & 1u));
        /* Rising edge: the slave shifts; sample its MISO afterwards —
         * qemu_set_irq is a synchronous call chain, so by the time it
         * returns the slave has settled and (if MISO changed) already
         * called our miso handler. */
        qemu_set_irq(s->sck, 1);
        miso |= ((uint32_t)(s->miso_level & 1)) << i;
        qemu_set_irq(s->sck, 0);
    }
    qemu_log_mask(LOG_UNIMP, "ssi-pin-bridge: mosi=%02x miso=%02x\n",
                  (unsigned)val, (unsigned)miso);
    return miso;
}

static int ot_ssi_pin_bridge_set_cs(SSIPeripheral *dev, bool select)
{
    OtSSIPinBridgeState *s = OT_SSI_PIN_BRIDGE(dev);
    /* select=true means CS asserted (SSI_CS_LOW polarity) -> csb line 0. */
    qemu_log_mask(LOG_UNIMP, "ssi-pin-bridge: cs %s\n", select ? "assert" : "deassert");
    if (!select) {
        /* Frame boundary: forward the deassert immediately. */
        qemu_set_irq(s->csb, 1);
        s->cs_out_low = false;
    }
    /* Asserts are byte-implied (see transfer). */
    return 0;
}

static void ot_ssi_pin_bridge_realize(SSIPeripheral *dev, Error **errp)
{
    OtSSIPinBridgeState *s = OT_SSI_PIN_BRIDGE(dev);
    DeviceState *d = DEVICE(dev);
    qdev_init_gpio_out_named(d, &s->sck, "sck", 1);
    qdev_init_gpio_out_named(d, &s->csb, "csb", 1);
    qdev_init_gpio_out_named(d, &s->mosi, "mosi", 1);
    qdev_init_gpio_in_named(d, ot_ssi_pin_bridge_set_miso, "miso", 1);
    s->miso_level = 0;
    s->cs_out_low = false;
}

static void ot_ssi_pin_bridge_class_init(ObjectClass *klass, const void *data)
{
    SSIPeripheralClass *k = SSI_PERIPHERAL_CLASS(klass);
    k->realize = ot_ssi_pin_bridge_realize;
    k->transfer = ot_ssi_pin_bridge_transfer;
    k->set_cs = ot_ssi_pin_bridge_set_cs;
    k->cs_polarity = SSI_CS_LOW;
}

static const TypeInfo ot_ssi_pin_bridge_info = {
    .name = TYPE_OT_SSI_PIN_BRIDGE,
    .parent = TYPE_SSI_PERIPHERAL,
    .instance_size = sizeof(OtSSIPinBridgeState),
    .class_init = ot_ssi_pin_bridge_class_init,
};

static void ot_ssi_pin_bridge_register_types(void)
{
    type_register_static(&ot_ssi_pin_bridge_info);
}

type_init(ot_ssi_pin_bridge_register_types)
