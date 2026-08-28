/*
 * QEMU RISC-V Board Compatible with OpenTitan EarlGrey FPGA platform
 *
 * Copyright (c) 2022-2025 Rivos, Inc.
 * Copyright (c) 2024-2025 lowRISC contributors.
 *
 * Author(s):
 *  Emmanuel Blot <eblot@rivosinc.com>
 *  Loïc Lefort <loic@rivosinc.com>
 *
 * This implementation is based on OpenTitan RTL version:
 *  <lowRISC/opentitan@caa3bd0a14ddebbf60760490f7c917901482c8fd>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2 or later, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "qemu/error-report.h"
#include "qemu/typedefs.h"
#include "qapi/error.h"
#include "qom/object.h"
#include "hw/block/flash.h"
#include "hw/boards.h"
#include "hw/core/split-irq.h"
#include "hw/intc/sifive_plic.h"
#include "hw/jtag/tap_ctrl.h"
#include "hw/jtag/tap_ctrl_rbb.h"
#include "hw/misc/pulp_rv_dm.h"
#include "hw/opentitan/ot_address_space.h"
#include "hw/opentitan/ot_aes.h"
#include "hw/opentitan/ot_alert.h"
#include "hw/opentitan/ot_aon_timer.h"
#include "hw/opentitan/ot_ast_eg.h"
#include "hw/opentitan/ot_clkmgr.h"
#include "hw/opentitan/ot_common.h"
#include "hw/opentitan/ot_csrng.h"
#include "hw/opentitan/ot_dm_tl.h"
#include "hw/opentitan/ot_edn.h"
#include "hw/opentitan/ot_eg_pad_ring.h"
#include "hw/opentitan/ot_entropy_src.h"
#include "hw/opentitan/ot_flash.h"
#include "hw/opentitan/ot_gpio_eg.h"
#include "hw/opentitan/gpio_qp_shim.h"
#include "hw/opentitan/ot_hmac.h"
#include "hw/opentitan/ot_i2c.h"
#include "hw/opentitan/i2c_qp_shim.h"
#include "hw/opentitan/ot_ibex_wrapper.h"
#include "hw/opentitan/ot_keymgr.h"
#include "hw/opentitan/ot_kmac.h"
#include "hw/opentitan/ot_lc_ctrl.h"
#include "hw/opentitan/ot_otbn.h"
#include "hw/opentitan/ot_otp_eg.h"
#include "hw/opentitan/ot_otp_if.h"
#include "hw/opentitan/ot_otp_ot_be.h"
#include "hw/opentitan/ot_pinmux_eg.h"
#include "hw/opentitan/ot_plic_ext.h"
#include "hw/opentitan/ot_pwrmgr.h"
#include "hw/opentitan/ot_rom_ctrl.h"
#include "hw/opentitan/ot_rstmgr.h"
#include "hw/opentitan/ot_sensor_eg.h"
#include "hw/opentitan/aes_qp_shim.h"
#include "hw/opentitan/qemu_passes/aes.h"
#include "hw/opentitan/kmac_qp_shim.h"
#include "hw/opentitan/rom_ctrl_qp_shim.h"
#include "hw/opentitan/rom_ctrl_qp_boot.h"
#include "hw/opentitan/keymgr_qp_shim.h"
#include "hw/opentitan/lc_ctrl_qp_shim.h"
#include "hw/opentitan/qemu_passes/lc_ctrl.h"
#include "hw/opentitan/csrng_qp_shim.h"
#include "hw/opentitan/qemu_passes/csrng.h"
#include "hw/opentitan/entropy_src_qp_shim.h"
#include "hw/opentitan/qemu_passes/entropy_src.h"
#include "hw/opentitan/edn_qp_shim.h"
#include "hw/opentitan/qemu_passes/edn.h"
#include "hw/opentitan/qemu_passes/keymgr.h"
#include "hw/opentitan/qemu_passes/rom_ctrl.h"
#include "hw/opentitan/qemu_passes/kmac.h"
#include "hw/opentitan/aon_timer_qp_shim.h"
#include "hw/opentitan/dma_qp_shim.h"
#include "hw/opentitan/rv_plic_qp_shim.h"
#include "hw/opentitan/hmac_qp_shim.h"
#include "hw/opentitan/adc_ctrl_qp_shim.h"
#include "hw/opentitan/sysrst_ctrl_qp_shim.h"
#include "hw/opentitan/alert_handler_qp_shim.h"
#include "hw/opentitan/pattgen_qp_shim.h"
#include "hw/opentitan/pinmux_qp_shim.h"
#include "hw/opentitan/pwm_qp_shim.h"
#include "hw/opentitan/ot_spi_device.h"
#include "hw/opentitan/spi_device_qp_shim.h"
#include "hw/opentitan/ot_spi_host.h"
#include "hw/opentitan/spi_host_qp_shim.h"
#include "hw/opentitan/ot_sram_ctrl.h"
#include "hw/opentitan/ot_timer.h"
#include "hw/opentitan/rv_timer_qp_shim.h"
#include "hw/opentitan/ot_uart.h"
#include "hw/opentitan/uart_qp_shim.h"
#include "hw/opentitan/ot_unimp.h"
#include "hw/opentitan/ot_usbdev.h"
#include "hw/opentitan/ot_vmapper.h"
#include "hw/qdev-properties.h"
#include "hw/riscv/dm.h"
#include "hw/riscv/dtm.h"
#include "hw/riscv/ibex_common.h"
#include "hw/riscv/ot_earlgrey.h"
#include "hw/ssi/ssi.h"
#include "qobject/qlist.h"
#include "system/address-spaces.h"
#include "system/blockdev.h"
#include "system/hw_accel.h"
#include "system/reset.h"
#include "system/system.h"

/* ------------------------------------------------------------------------ */
/* Forward Declarations */
/* ------------------------------------------------------------------------ */

static void ot_eg_soc_ast_configure(DeviceState *dev, const IbexDeviceDef *def,
                                    DeviceState *parent);
static void ot_eg_soc_dm_configure(DeviceState *dev, const IbexDeviceDef *def,
                                   DeviceState *parent);
static void ot_eg_soc_flash_ctrl_configure(
    DeviceState *dev, const IbexDeviceDef *def, DeviceState *parent);
static void ot_eg_soc_hart_configure(DeviceState *dev, const IbexDeviceDef *def,
                                     DeviceState *parent);
static void ot_eg_soc_otp_ctrl_configure(
    DeviceState *dev, const IbexDeviceDef *def, DeviceState *parent);
static void ot_eg_soc_tap_ctrl_configure(
    DeviceState *dev, const IbexDeviceDef *def, DeviceState *parent);
static void ot_eg_soc_lc_ctrl_tap_ctrl_configure(
    DeviceState *dev, const IbexDeviceDef *def, DeviceState *parent);
static void ot_eg_soc_spi_device_configure(
    DeviceState *dev, const IbexDeviceDef *def, DeviceState *parent);
static void ot_eg_soc_uart_configure(DeviceState *dev, const IbexDeviceDef *def,
                                     DeviceState *parent);
static void ot_eg_soc_usbdev_configure(
    DeviceState *dev, const IbexDeviceDef *def, DeviceState *parent);

/* ------------------------------------------------------------------------ */
/* Constants */
/* ------------------------------------------------------------------------ */

enum OtEgMemoryRegion {
    OT_EG_DEFAULT_MEMORY_REGION,
    OT_EG_LC_CTRL_TAP_MEMORY_REGION,
};

#define LC_CTRL_TAP_MEMORY(_addr_) \
    IBEX_MEMMAP_MAKE_REG((_addr_), OT_EG_LC_CTRL_TAP_MEMORY_REGION)

enum OtEGSocDevice {
    OT_EG_SOC_DEV_ADC_CTRL,
    OT_EG_SOC_DEV_AES,
    OT_EG_SOC_DEV_ALERT_HANDLER,
    OT_EG_SOC_DEV_AON_TIMER,
    OT_EG_SOC_DEV_AST,
    OT_EG_SOC_DEV_CLKMGR,
    OT_EG_SOC_DEV_CSRNG,
    OT_EG_SOC_DEV_DM,
    OT_EG_SOC_DEV_DMA,
    OT_EG_SOC_DEV_DTM,
    OT_EG_SOC_DEV_LC_CTRL_DTM,
    OT_EG_SOC_DEV_EDN0,
    OT_EG_SOC_DEV_EDN1,
    OT_EG_SOC_DEV_ENTROPY_SRC,
    OT_EG_SOC_DEV_FLASH_CTRL,
    OT_EG_SOC_DEV_GPIO,
    OT_EG_SOC_DEV_HART,
    OT_EG_SOC_DEV_HMAC,
    OT_EG_SOC_DEV_I2C0,
    OT_EG_SOC_DEV_I2C1,
    OT_EG_SOC_DEV_I2C2,
    OT_EG_SOC_DEV_IBEX_WRAPPER,
    OT_EG_SOC_DEV_KEYMGR,
    OT_EG_SOC_DEV_KMAC,
    OT_EG_SOC_DEV_LC_CTRL,
    OT_EG_SOC_DEV_OTBN,
    OT_EG_SOC_DEV_OTP_CTRL,
    OT_EG_SOC_DEV_OTP_BACKEND,
    OT_EG_SOC_DEV_PAD_RING,
    OT_EG_SOC_DEV_PATTGEN,
    OT_EG_SOC_DEV_PINMUX,
    OT_EG_SOC_DEV_PLIC,
    OT_EG_SOC_DEV_PLIC_EXT,
    OT_EG_SOC_DEV_PWM,
    OT_EG_SOC_DEV_PWRMGR,
    OT_EG_SOC_DEV_SRAM_RET_CTRL,
    OT_EG_SOC_DEV_ROM_CTRL,
    OT_EG_SOC_DEV_RSTMGR,
    OT_EG_SOC_DEV_RV_DM,
    OT_EG_SOC_DEV_DM_LC_CTRL,
    OT_EG_SOC_DEV_SENSOR_CTRL,
    OT_EG_SOC_DEV_SPI_DEVICE,
    OT_EG_SOC_DEV_SPI_HOST0,
    OT_EG_SOC_DEV_SPI_HOST1,
    OT_EG_SOC_DEV_SRAM_MAIN_CTRL,
    OT_EG_SOC_DEV_SYSRST_CTRL,
    OT_EG_SOC_DEV_TAP_CTRL,
    OT_EG_SOC_DEV_LC_CTRL_TAP_CTRL,
    OT_EG_SOC_DEV_TIMER,
    OT_EG_SOC_DEV_UART0,
    OT_EG_SOC_DEV_UART1,
    OT_EG_SOC_DEV_UART2,
    OT_EG_SOC_DEV_UART3,
    OT_EG_SOC_DEV_USBDEV,
    OT_EG_SOC_DEV_VMAPPER,
    OT_EG_SOC_DEV_KMAC_APP_SVC,
    OT_EG_SOC_DEV_ROM_CTRL_QP,
    /* [qemu-passes] HEART SWAP: the primary generated rom_ctrl (CSRs at
     * the native base) + the boot front-end that owns the CPU-fetch ROM
     * window and drives the pwrmgr done/good handshake from the
     * generated model's digest verdict. */
    OT_EG_SOC_DEV_ROM_CTRL_QPP,
    OT_EG_SOC_DEV_ROM_CTRL_QP_BOOT,
    OT_EG_SOC_DEV_KEYMGR_QP,
    OT_EG_SOC_DEV_LC_CTRL_QP,
    /* [qemu-passes] the entropy ring, PARALLEL mounts (natives
     * untouched): entropy_src (fw_ov -> SHA3-384 conditioner), csrng
     * (CTR_DRBG), edn (SW command port + endpoints).  Signal-level
     * ring bridge in ot_eg_entropy_ring_pump(). */
    OT_EG_SOC_DEV_ENTROPY_SRC_QP,
    OT_EG_SOC_DEV_CSRNG_QP,
    OT_EG_SOC_DEV_EDN_QP,
    /* IRQ splitters, i.e. 1-to-N signal dispatchers */
    OT_EG_SOC_SPLITTER_LC_HW_DEBUG,
    OT_EG_SOC_SPLITTER_LC_ESCALATE,
    OT_EG_SOC_SPLITTER_LC_SEED_HW_RD,
    OT_EG_SOC_SPLITTER_LC_CREATOR_SEED_SW_RW,
    /* [qemu-passes] irq tap: uart0 rx_watermark fans out to its PLIC slot
     * AND the generated DMA's hardware-handshake trigger 0 (lsio_trigger_i,
     * qdev gpio-in of ot-dma-qp).  soc_glue.py understands this indirection
     * (an "irq tap"): verify still requires the PLIC slot to be fed. */
    OT_EG_SOC_SPLITTER_UART0_RX_DMA,
};

enum OtEgResetRequest {
    OT_EG_RESET_SYSRST_CTRL,
    OT_EG_RESET_AON_TIMER,
    OT_EG_RESET_SENSOR_CTRL,
    OT_EG_RESET_COUNT
};

/* Data flash buses */
enum OtEgMtdBus {
    OT_EG_MTD_SPI0,
    OT_EG_MTD_SPI1,
    OT_EG_MTD_SPI_COUNT,
    OT_EG_MTD_EFLASH = OT_EG_MTD_SPI_COUNT,
};

/* "Parallel" flash buses */
enum OtEgPflashBus {
    OT_EG_PFLASH_OTP,
};

enum OtEGBoardDevice {
    OT_EG_BOARD_DEV_SOC,
    OT_EG_BOARD_DEV_FLASH0,
    OT_EG_BOARD_DEV_FLASH1,
    OT_EG_BOARD_DEV_COUNT,
};

/*
 * <opentitan>/hw/ip/lc_ctrl/rtl/lc_ctrl.sv instantiates a DMI module (with
 * abits=7) and a DMI to TL-UL adapter. Together, they create a private bus,
 * exposing the LC Ctrl registers over JTAG <-> DTM <-> DMI <-> TL-UL. On the
 * DMI side we have the address space [0 .. 2^7); those addresses are mapped to
 * words on the TL-UL side, addressing [0 .. 2^7*4) bytes, and accessing the LC
 * Ctrl registers at the appropriate (documented) offset.
 */
#define OT_EG_LC_CTRL_TAP_DMI_ABITS 7u
#define OT_EG_LC_CTRL_TAP_DMI_ADDR  0x0u
#define OT_EG_LC_CTRL_TAP_DMI_SIZE  (1u << OT_EG_LC_CTRL_TAP_DMI_ABITS)
#define OT_EG_LC_CTRL_TAP_TL_ADDR   0x0u
#define OT_EG_LC_CTRL_TAP_TL_SIZE   ((1u << OT_EG_LC_CTRL_TAP_DMI_ABITS) * 4u)

#define OT_EG_LC_CTRL_TAP      "ot-lc_ctrl-tap"
#define OT_EG_LC_CTRL_TAP_XBAR OT_EG_LC_CTRL_TAP ".xbar"
#define OT_EG_LC_CTRL_TAP_AS   OT_EG_LC_CTRL_TAP ".as"

#define OT_EG_IBEX_WRAPPER_NUM_REGIONS 2u

static const uint8_t ot_eg_pmp_cfgs[] = {
    /* clang-format off */
    IBEX_PMP_CFG(0, IBEX_PMP_MODE_OFF, 0, 0, 0),
    IBEX_PMP_CFG(0, IBEX_PMP_MODE_OFF, 0, 0, 0),
    IBEX_PMP_CFG(1, IBEX_PMP_MODE_NAPOT, 1, 0, 1), /* rgn 2  [ROM: LRX] */
    IBEX_PMP_CFG(0, IBEX_PMP_MODE_OFF, 0, 0, 0),
    IBEX_PMP_CFG(0, IBEX_PMP_MODE_OFF, 0, 0, 0),
    IBEX_PMP_CFG(0, IBEX_PMP_MODE_OFF, 0, 0, 0),
    IBEX_PMP_CFG(0, IBEX_PMP_MODE_OFF, 0, 0, 0),
    IBEX_PMP_CFG(0, IBEX_PMP_MODE_OFF, 0, 0, 0),
    IBEX_PMP_CFG(0, IBEX_PMP_MODE_OFF, 0, 0, 0),
    IBEX_PMP_CFG(0, IBEX_PMP_MODE_OFF, 0, 0, 0),
    IBEX_PMP_CFG(0, IBEX_PMP_MODE_OFF, 0, 0, 0),
    IBEX_PMP_CFG(1, IBEX_PMP_MODE_TOR, 0, 1, 1), /* rgn 11 [MMIO: LRW] */
    IBEX_PMP_CFG(0, IBEX_PMP_MODE_OFF, 0, 0, 0),
    IBEX_PMP_CFG(1, IBEX_PMP_MODE_NAPOT, 1, 1, 1), /* rgn 13 [DV_ROM: LRWX] */
    IBEX_PMP_CFG(0, IBEX_PMP_MODE_OFF, 0, 0, 0),
    IBEX_PMP_CFG(0, IBEX_PMP_MODE_OFF, 0, 0, 0)
    /* clang-format on */
};

static const uint32_t ot_eg_pmp_addrs[] = {
    /* clang-format off */
    IBEX_PMP_ADDR(0x00000000),
    IBEX_PMP_ADDR(0x00000000),
    IBEX_PMP_ADDR(0x000083fc), /* rgn 2 [ROM: base=0x0000_8000 sz (2KiB)] */
    IBEX_PMP_ADDR(0x00000000),
    IBEX_PMP_ADDR(0x00000000),
    IBEX_PMP_ADDR(0x00000000),
    IBEX_PMP_ADDR(0x00000000),
    IBEX_PMP_ADDR(0x00000000),
    IBEX_PMP_ADDR(0x00000000),
    IBEX_PMP_ADDR(0x00000000),
    IBEX_PMP_ADDR(0x40000000), /* rgn 10 [MMIO: lo=0x4000_0000] */
    IBEX_PMP_ADDR(0x42010000), /* rgn 11 [MMIO: hi=0x4201_0000] */
    IBEX_PMP_ADDR(0x00000000),
    IBEX_PMP_ADDR(0x000107fc), /* rgn 13 [DV_ROM: base=0x0001_0000 sz (4KiB)] */
    IBEX_PMP_ADDR(0x00000000),
    IBEX_PMP_ADDR(0x00000000)
    /* clang-format on */
};

#define OT_EG_MSECCFG IBEX_MSECCFG(1, 1, 0)

#define OT_EG_SOC_RST_REQ TYPE_RISCV_OT_EG_SOC "-reset"

#define OT_EG_SOC_GPIO(_irq_, _target_, _num_) \
    IBEX_GPIO(_irq_, OT_EG_SOC_DEV_##_target_, _num_)

#define OT_EG_SOC_GPIO_SYSBUS_IRQ(_irq_, _target_, _num_) \
    IBEX_GPIO_SYSBUS_IRQ(_irq_, OT_EG_SOC_DEV_##_target_, _num_)

#define OT_EG_SOC_GPIO_ALERT(_snum_, _tnum_) \
    OT_EG_SOC_SIGNAL(OT_DEVICE_ALERT, _snum_, ALERT_HANDLER, OT_DEVICE_ALERT, \
                     _tnum_)

#define OT_EG_SOC_GPIO_ESCALATE(_snum_, _tgt_, _tnum_) \
    OT_EG_SOC_SIGNAL(OT_ALERT_ESCALATE, _snum_, _tgt_, OT_ALERT_ESCALATE, \
                     _tnum_)

#define OT_EG_SOC_DEVLINK(_pname_, _target_) \
    IBEX_DEVLINK(_pname_, OT_EG_SOC_DEV_##_target_)

/* Device named signal to device named signal */
#define OT_EG_SOC_SIGNAL(_sname_, _snum_, _tgt_, _tname_, _tnum_) \
    { \
        .out = { \
            .name = (_sname_), \
            .num = (_snum_), \
        }, \
        .in = { \
            .name = (_tname_), \
            .index = (OT_EG_SOC_DEV_ ## _tgt_), \
            .num = (_tnum_), \
        } \
    }

/* Device named signal to splitter input */
#define OT_EG_SOC_D2S(_sname_, _snum_, _tgt_) \
    { \
        .out = { \
            .name = (_sname_), \
            .num = (_snum_), \
        }, \
        .in = { \
            .index = (OT_EG_SOC_SPLITTER_ ## _tgt_), \
        } \
    }

/* Splitter output to device named signal */
#define OT_EG_SOC_S2D(_snum_, _tgt_, _tname_, _tnum_) \
    { \
        .out = { \
            .num = (_snum_), \
        }, \
        .in = { \
            .name = (_tname_), \
            .index = (OT_EG_SOC_DEV_ ## _tgt_), \
            .num = (_tnum_), \
        } \
    }

/* Request link */
#define OT_EG_SOC_REQ(_req_, _tgt_) \
    OT_EG_SOC_SIGNAL(_req_##_REQ, 0, _tgt_, _req_##_REQ, 0)

/* Response link */
#define OT_EG_SOC_RSP(_rsp_, _tgt_) \
    OT_EG_SOC_SIGNAL(_rsp_##_RSP, 0, _tgt_, _rsp_##_RSP, 0)

#define OT_EG_SOC_DM_CONNECTION(_dst_dev_, _num_) \
    { \
        .out = { \
            .name = PULP_RV_DM_ACK_OUT_LINES, \
            .num = (_num_), \
        }, \
        .in = { \
            .name = RISCV_DM_ACK_LINES, \
            .index = (_dst_dev_), \
            .num = (_num_), \
        } \
    }

/*
 * Earlgrey 1.0.0 TAPs
 * See https://github.com/lowRISC/part-number-registry/blob/main/jtag_partno.md
 */
#define EG_RV_DM_TAP_IDCODE    IBEX_JTAG_IDCODE(0, 1, 1)
#define EG_LC_CTRL_TAP_IDCODE  IBEX_JTAG_IDCODE(0, 2, 1)
#define EG_COMBINED_TAP_IDCODE IBEX_JTAG_IDCODE(0, 3, 1)

#define PULP_DM_BASE   0x00010000u
#define SRAM_MAIN_SIZE 0x20000u

/*
 * MMIO/interrupt mapping as per:
 * lowRISC/opentitan: hw/top_earlgrey/sw/autogen/top_earlgrey_memory.h
 * and
 * lowRISC/opentitan: hw/top_earlgrey/sw/autogen/top_earlgrey.h
 */
static const IbexDeviceDef ot_eg_soc_devices[] = {
    /* clang-format off */
    [OT_EG_SOC_DEV_HART] = {
        .type = TYPE_RISCV_CPU_LOWRISC_OPENTITAN,
        .cfg = &ot_eg_soc_hart_configure,
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_UINT_PROP("resetvec", 0x8080u),
            IBEX_DEV_UINT_PROP("mtvec", 0x8001u),
            IBEX_DEV_UINT_PROP("dmhaltvec", PULP_DM_BASE +
                PULP_RV_DM_ROM_BASE + PULP_RV_DM_HALT_OFFSET),
            IBEX_DEV_UINT_PROP("dmexcpvec", PULP_DM_BASE +
                PULP_RV_DM_ROM_BASE + PULP_RV_DM_EXCEPTION_OFFSET),
            IBEX_DEV_BOOL_PROP("start-powered-off", true)
        ),
    },
    [OT_EG_SOC_DEV_TAP_CTRL] = {
        .type = TYPE_TAP_CTRL_RBB,
        .cfg = &ot_eg_soc_tap_ctrl_configure,
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_UINT_PROP("ir_length", IBEX_TAP_IR_LENGTH),
            IBEX_DEV_UINT_PROP("idcode", EG_RV_DM_TAP_IDCODE)
        ),
    },
    [OT_EG_SOC_DEV_LC_CTRL_TAP_CTRL] = {
        .type = TYPE_TAP_CTRL_RBB,
        .cfg = &ot_eg_soc_lc_ctrl_tap_ctrl_configure,
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_UINT_PROP("ir_length", IBEX_TAP_IR_LENGTH),
            IBEX_DEV_UINT_PROP("idcode", EG_LC_CTRL_TAP_IDCODE)
        ),
    },
    [OT_EG_SOC_DEV_DTM] = {
        .type = TYPE_RISCV_DTM,
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("tap-ctrl", TAP_CTRL)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_UINT_PROP("abits", 7u)
        ),
    },
    [OT_EG_SOC_DEV_LC_CTRL_DTM] = {
        .type = TYPE_RISCV_DTM,
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("tap-ctrl", LC_CTRL_TAP_CTRL)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_UINT_PROP("abits", OT_EG_LC_CTRL_TAP_DMI_ABITS)
        ),
    },
    [OT_EG_SOC_DEV_DM] = {
        .type = TYPE_RISCV_DM,
        .cfg = &ot_eg_soc_dm_configure,
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("dtm", DTM)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_UINT_PROP("nscratch", PULP_RV_DM_NSCRATCH_COUNT),
            IBEX_DEV_UINT_PROP("progbuf_count",
                PULP_RV_DM_PROGRAM_BUFFER_COUNT),
            IBEX_DEV_UINT_PROP("data_count", PULP_RV_DM_DATA_COUNT),
            IBEX_DEV_UINT_PROP("abstractcmd_count",
                PULP_RV_DM_ABSTRACTCMD_COUNT),
            IBEX_DEV_UINT_PROP("dm_phyaddr", PULP_DM_BASE),
            IBEX_DEV_UINT_PROP("rom_phyaddr",
                PULP_DM_BASE + PULP_RV_DM_ROM_BASE),
            IBEX_DEV_UINT_PROP("whereto_phyaddr",
                PULP_DM_BASE + PULP_RV_DM_WHERETO_OFFSET),
            IBEX_DEV_UINT_PROP("data_phyaddr",
                PULP_DM_BASE + PULP_RV_DM_DATAADDR_OFFSET),
            IBEX_DEV_UINT_PROP("progbuf_phyaddr",
                PULP_DM_BASE + PULP_RV_DM_PROGRAM_BUFFER_OFFSET),
            IBEX_DEV_UINT_PROP("resume_offset", PULP_RV_DM_RESUME_OFFSET),
            IBEX_DEV_BOOL_PROP("sysbus_access", true),
            IBEX_DEV_BOOL_PROP("abstractauto", true)
        ),
    },
    [OT_EG_SOC_DEV_UART0] = {
        /* [soc_gen] generated from soc/earlgrey.soc.json (+ soc.digest.json); regenerate with `python3 soc/soc_glue.py write` — do not hand-edit. */
        /* qemu-passes drop-in: UART0 routes through the auto-generated
         * frontend wrapped by ot_uart_qp.c.  UART1/2/3 below stay on
         * TYPE_OT_UART so a regression here is isolated to UART0. */
        .type = TYPE_OT_UART_QP,
        .instance = IBEX_MAKE_INSTANCE_NUM(0),
        .cfg = &ot_eg_soc_uart_configure,
        .memmap = MEMMAPENTRIES(
            { .base = 0x40000000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 1),
            /* [qemu-passes irq tap] rx_watermark (irq 1) routes through a
             * 1-to-2 splitter: out0 -> PLIC input 2 (its normal slot),
             * out1 -> DMA lsio_trigger_i[0] (hardware handshake). */
            IBEX_GPIO_SYSBUS_IRQ(1, OT_EG_SOC_SPLITTER_UART0_RX_DMA, 0),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(2, PLIC, 3),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(3, PLIC, 4),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(4, PLIC, 5),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(5, PLIC, 6),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(6, PLIC, 7),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(7, PLIC, 8),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(8, PLIC, 9),
            OT_EG_SOC_GPIO_ALERT(0, 0)
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("clock-src", CLKMGR)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "u0"),
            IBEX_DEV_STRING_PROP("clock-name", "peri.io_div4")
        ),
    },
    [OT_EG_SOC_DEV_UART1] = {
        .type = TYPE_OT_UART,
        .cfg = &ot_eg_soc_uart_configure,
        .instance = IBEX_MAKE_INSTANCE_NUM(1),
        .memmap = MEMMAPENTRIES(
            { .base = 0x40010000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 10),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(1, PLIC, 11),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(2, PLIC, 12),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(3, PLIC, 13),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(4, PLIC, 14),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(5, PLIC, 15),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(6, PLIC, 16),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(7, PLIC, 17),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(8, PLIC, 18),
            OT_EG_SOC_GPIO_ALERT(0, 1)
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("clock-src", CLKMGR)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "u1"),
            IBEX_DEV_STRING_PROP("clock-name", "peri.io_div4")
        ),
    },
    [OT_EG_SOC_DEV_UART2] = {
        .type = TYPE_OT_UART,
        .cfg = &ot_eg_soc_uart_configure,
        .instance = IBEX_MAKE_INSTANCE_NUM(2),
        .memmap = MEMMAPENTRIES(
            { .base = 0x40020000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 19),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(1, PLIC, 20),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(2, PLIC, 21),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(3, PLIC, 22),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(4, PLIC, 23),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(5, PLIC, 24),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(6, PLIC, 25),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(7, PLIC, 26),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(8, PLIC, 27),
            OT_EG_SOC_GPIO_ALERT(0, 2)
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("clock-src", CLKMGR)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "u2"),
            IBEX_DEV_STRING_PROP("clock-name", "peri.io_div4")
        ),
    },
    [OT_EG_SOC_DEV_UART3] = {
        .type = TYPE_OT_UART,
        .cfg = &ot_eg_soc_uart_configure,
        .instance = IBEX_MAKE_INSTANCE_NUM(3),
        .memmap = MEMMAPENTRIES(
            { .base = 0x40030000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 28),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(1, PLIC, 29),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(2, PLIC, 30),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(3, PLIC, 31),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(4, PLIC, 32),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(5, PLIC, 33),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(6, PLIC, 34),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(7, PLIC, 35),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(8, PLIC, 36),
            OT_EG_SOC_GPIO_ALERT(0, 3)
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("clock-src", CLKMGR)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "u3"),
            IBEX_DEV_STRING_PROP("clock-name", "peri.io_div4")
        ),
    },
    [OT_EG_SOC_DEV_GPIO] = {
        /* [soc_gen] generated from soc/earlgrey.soc.json (+ soc.digest.json); regenerate with `python3 soc/soc_glue.py write` — do not hand-edit. */
        /* qemu-passes drop-in: route GPIO through ot_gpio_qp.c, which
         * wraps the auto-generated model in qemu_passes/gpio.c.  Switch
         * back to TYPE_OT_GPIO_EG to fall back to the upstream model. */
        .type = TYPE_OT_GPIO_QP,
        .memmap = MEMMAPENTRIES(
            { .base = 0x40040000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 37),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(1, PLIC, 38),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(2, PLIC, 39),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(3, PLIC, 40),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(4, PLIC, 41),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(5, PLIC, 42),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(6, PLIC, 43),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(7, PLIC, 44),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(8, PLIC, 45),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(9, PLIC, 46),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(10, PLIC, 47),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(11, PLIC, 48),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(12, PLIC, 49),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(13, PLIC, 50),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(14, PLIC, 51),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(15, PLIC, 52),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(16, PLIC, 53),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(17, PLIC, 54),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(18, PLIC, 55),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(19, PLIC, 56),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(20, PLIC, 57),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(21, PLIC, 58),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(22, PLIC, 59),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(23, PLIC, 60),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(24, PLIC, 61),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(25, PLIC, 62),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(26, PLIC, 63),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(27, PLIC, 64),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(28, PLIC, 65),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(29, PLIC, 66),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(30, PLIC, 67),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(31, PLIC, 68),
            OT_EG_SOC_GPIO_ALERT(0, 4)
        ),
    },
    [OT_EG_SOC_DEV_SPI_DEVICE] = {
        /* [soc_gen] generated from soc/earlgrey.soc.json (+ soc.digest.json); regenerate with `python3 soc/soc_glue.py write` — do not hand-edit. */
        /* qemu-passes drop-in: route through spi_device_qp_shim.c which
         * wraps the auto-generated spi_device model.  shim exposes the
         * upstream `chardev` property for the spidev transport (SPI-slave
         * organ, qemu.spi_slave_blueprint) plus `ot_id` and `spi-host` link.  Link is typed TYPE_DEVICE so it accepts
         * either the QP or upstream SPI_HOST flavour. */
        .type = TYPE_OT_SPI_DEVICE_QP,
        .instance = IBEX_MAKE_INSTANCE_NUM(0),
        .cfg = &ot_eg_soc_spi_device_configure,
        .memmap = MEMMAPENTRIES(
            { .base = 0x40050000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 69),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(1, PLIC, 70),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(2, PLIC, 71),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(3, PLIC, 72),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(4, PLIC, 73),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(5, PLIC, 74),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(6, PLIC, 75),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(7, PLIC, 76),
            OT_EG_SOC_GPIO_ALERT(0, 5)
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("spi-host", SPI_HOST1)
        ),
    },
    [OT_EG_SOC_DEV_I2C0] = {
        /* [soc_gen] generated from soc/earlgrey.soc.json (+ soc.digest.json); regenerate with `python3 soc/soc_glue.py write` — do not hand-edit. */
        /* qemu-passes drop-in: route I2C0 through i2c_qp_shim.c, which
         * wraps the auto-generated model in qemu_passes/i2c.c.  I2C1/I2C2
         * stay on TYPE_OT_I2C upstream so a regression here is isolated.
         *
         * IRQ 10..14 (TX_STRETCH / TX_THRESHOLD / ACQ_STRETCH / UNEXP_STOP /
         * HOST_TIMEOUT) dropped: they don't exist in the LLHD IR (older OT
         * version) and the auto-emitted shim has irqCount=10.  PLIC lines
         * 87..91 stay unconnected. */
        .type = TYPE_OT_I2C_QP,
        .instance = IBEX_MAKE_INSTANCE_NUM(0),
        .memmap = MEMMAPENTRIES(
            { .base = 0x40080000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 77),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(1, PLIC, 78),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(2, PLIC, 79),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(3, PLIC, 80),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(4, PLIC, 81),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(5, PLIC, 82),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(6, PLIC, 83),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(7, PLIC, 84),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(8, PLIC, 85),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(9, PLIC, 86),
            OT_EG_SOC_GPIO_ALERT(0, 6)
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("clock-src", CLKMGR)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "i2c0"),
            IBEX_DEV_STRING_PROP("clock-name", "peri.io_div4")
        ),
    },
    [OT_EG_SOC_DEV_I2C1] = {
        .type = TYPE_OT_I2C,
        .instance = IBEX_MAKE_INSTANCE_NUM(1),
        .memmap = MEMMAPENTRIES(
            { .base = 0x40090000u }
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "i2c1"),
            IBEX_DEV_STRING_PROP("clock-name", "peri.io_div4")
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 92),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(1, PLIC, 93),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(2, PLIC, 94),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(3, PLIC, 95),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(4, PLIC, 96),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(5, PLIC, 97),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(6, PLIC, 98),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(7, PLIC, 99),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(8, PLIC, 100),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(9, PLIC, 101),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(10, PLIC, 102),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(11, PLIC, 103),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(12, PLIC, 104),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(13, PLIC, 105),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(14, PLIC, 106),
            OT_EG_SOC_GPIO_ALERT(0, 7)
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("clock-src", CLKMGR)
        )
    },
    [OT_EG_SOC_DEV_I2C2] = {
        .type = TYPE_OT_I2C,
        .instance = IBEX_MAKE_INSTANCE_NUM(2),
        .memmap = MEMMAPENTRIES(
            { .base = 0x400a0000u }
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "i2c2"),
            IBEX_DEV_STRING_PROP("clock-name", "peri.io_div4")
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 107),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(1, PLIC, 108),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(2, PLIC, 109),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(3, PLIC, 110),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(4, PLIC, 111),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(5, PLIC, 112),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(6, PLIC, 113),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(7, PLIC, 114),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(8, PLIC, 115),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(9, PLIC, 116),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(10, PLIC, 117),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(11, PLIC, 118),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(12, PLIC, 119),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(13, PLIC, 120),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(14, PLIC, 121),
            OT_EG_SOC_GPIO_ALERT(0, 8)
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("clock-src", CLKMGR)
        )
    },
    [OT_EG_SOC_DEV_PAD_RING] = {
        .type = TYPE_OT_EG_PAD_RING,
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_SIGNAL(OT_EG_PAD_RING_POR_REQ, 0, RSTMGR,
                             OT_RSTMGR_RST_REQ, 0)
        ),
    },
    [OT_EG_SOC_DEV_PATTGEN] = {
        /* [soc_gen] generated from soc/earlgrey.soc.json (+ soc.digest.json); regenerate with `python3 soc/soc_glue.py write` — do not hand-edit. */
        /* qemu-passes drop-in: pattgen was unimp upstream; we replace with
         * the auto-generated model.  2 IRQ outputs (intr_done_ch{0,1}) are
         * unwired in frontend mode (upstream UNIMP didn't connect them to
         * PLIC either).  cio_p{c,d}{a,l}{0,1}_tx_o channels go nowhere. */
        .type = TYPE_OT_PATTGEN_QP,
        .memmap = MEMMAPENTRIES(
            { .base = 0x400e0000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 122),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(1, PLIC, 123),
            OT_EG_SOC_GPIO_ALERT(0, 9)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "pattgen")
        ),
    },
    [OT_EG_SOC_DEV_TIMER] = {
        /* [soc_gen] generated from soc/earlgrey.soc.json (+ soc.digest.json); regenerate with `python3 soc/soc_glue.py write` — do not hand-edit. */
        /* qemu-passes drop-in: route rv_timer through rv_timer_qp_shim.c,
         * which wraps the auto-generated model in qemu_passes/rv_timer.c.
         * Switch back to TYPE_OT_TIMER to fall back to the upstream model.
         *
         * The HART/M_TIMER connection (`OT_EG_SOC_GPIO(0, HART, IRQ_M_TIMER)`)
         * is intentionally dropped: it requires an unnamed gpio_out pin
         * which the auto-emitted shim does not register (it only sets up
         * sysbus IRQs + named alert).  Frontend-only doesn't fire IRQs
         * anyway, so HART won't get a timer wakeup — fine for register
         * round-trip testing.  Backend wiring milestone will revisit. */
        .type = TYPE_OT_RV_TIMER_QP,
        .memmap = MEMMAPENTRIES(
            { .base = 0x40100000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 124),
            OT_EG_SOC_GPIO_ALERT(0, 10)
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("clock-src", CLKMGR)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP("clock-name", "timers.io_div4")
        ),
    },
    [OT_EG_SOC_DEV_OTP_CTRL] = {
        .type = TYPE_OT_OTP_EG,
        .cfg = &ot_eg_soc_otp_ctrl_configure,
        .memmap = MEMMAPENTRIES(
            { .base = 0x40130000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 125),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(1, PLIC, 126),
            OT_EG_SOC_GPIO_ALERT(0, 11),
            OT_EG_SOC_GPIO_ALERT(1, 12),
            OT_EG_SOC_GPIO_ALERT(2, 13),
            OT_EG_SOC_GPIO_ALERT(3, 14),
            OT_EG_SOC_GPIO_ALERT(4, 15),
            OT_EG_SOC_RSP(OT_PWRMGR_OTP, PWRMGR)
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("edn", EDN0),
            OT_EG_SOC_DEVLINK("backend", OTP_BACKEND)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_UINT_PROP("edn-ep", 1u)
        ),
    },
    [OT_EG_SOC_DEV_OTP_BACKEND] = {
        .type = TYPE_OT_OTP_OT_BE,
        .memmap = MEMMAPENTRIES(
            { .base = 0x40132000u }
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("parent", OTP_CTRL)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_UINT_PROP("write_ns", 25000u), /* 25 us */
            IBEX_DEV_UINT_PROP("read_ns", 5000u) /* 5 us */
        )
    },
    [OT_EG_SOC_DEV_LC_CTRL] = {
        .type = TYPE_OT_LC_CTRL,
        .memmap = MEMMAPENTRIES(
            { .base = 0x40140000u },
            { .base = LC_CTRL_TAP_MEMORY(OT_EG_LC_CTRL_TAP_TL_ADDR) }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_RSP(OT_PWRMGR_LC, PWRMGR),
            OT_EG_SOC_GPIO_ALERT(0, 16),
            OT_EG_SOC_GPIO_ALERT(1, 17),
            OT_EG_SOC_GPIO_ALERT(2, 18),
            /*
             * @todo: check for missing life cycle broadcast signal connections
             * and add them when the required supporting HW is available.
             */
            /* Splitters for signals that go to many blocks. */
            OT_EG_SOC_D2S(OT_LC_BROADCAST, OT_LC_HW_DEBUG_EN, LC_HW_DEBUG),
            OT_EG_SOC_D2S(OT_LC_BROADCAST, OT_LC_ESCALATE_EN, LC_ESCALATE),
            OT_EG_SOC_D2S(OT_LC_BROADCAST, OT_LC_SEED_HW_RD_EN, LC_SEED_HW_RD),
            OT_EG_SOC_D2S(OT_LC_BROADCAST, OT_LC_CREATOR_SEED_SW_RW_EN,
                          LC_CREATOR_SEED_SW_RW),
            /* Signals to ibex_wrapper */
            OT_EG_SOC_SIGNAL(OT_LC_BROADCAST, OT_LC_CPU_EN, IBEX_WRAPPER,
                             OT_IBEX_WRAPPER_CPU_EN, OT_IBEX_LC_CTRL_CPU_EN),
            /* Signals to keymgr */
            OT_EG_SOC_SIGNAL(OT_LC_BROADCAST, OT_LC_KEYMGR_EN, KEYMGR,
                             OT_KEYMGR_ENABLE, 0),
            /* Signals to flash_ctrl */
            OT_EG_SOC_SIGNAL(OT_LC_BROADCAST, OT_LC_OWNER_SEED_SW_RW_EN,
                             FLASH_CTRL, OT_LC_BROADCAST,
                             OT_FLASH_LC_OWNER_SEED_SW_RW_EN),
            OT_EG_SOC_SIGNAL(OT_LC_BROADCAST, OT_LC_SEED_HW_RD_EN, FLASH_CTRL,
                             OT_LC_BROADCAST, OT_FLASH_LC_SEED_HW_RD_EN),
            OT_EG_SOC_SIGNAL(OT_LC_BROADCAST, OT_LC_ISO_PART_SW_RD_EN,
                             FLASH_CTRL, OT_LC_BROADCAST,
                             OT_FLASH_LC_ISO_PART_SW_RD_EN),
            OT_EG_SOC_SIGNAL(OT_LC_BROADCAST, OT_LC_ISO_PART_SW_WR_EN,
                             FLASH_CTRL, OT_LC_BROADCAST,
                             OT_FLASH_LC_ISO_PART_SW_WR_EN),
            OT_EG_SOC_SIGNAL(OT_LC_BROADCAST, OT_LC_NVM_DEBUG_EN, FLASH_CTRL,
                             OT_LC_BROADCAST, OT_FLASH_LC_NVM_DEBUG_EN),
            /* Signals to OTP */
            OT_EG_SOC_SIGNAL(OT_LC_BROADCAST, OT_LC_CHECK_BYP_EN, OTP_CTRL,
                             OT_LC_BROADCAST, OT_OTP_LC_CHECK_BYP_EN)
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("otp-ctrl", OTP_CTRL),
            OT_EG_SOC_DEVLINK("kmac", KMAC_APP_SVC)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_UINT_PROP("silicon_creator_id", 0x4001u),
            IBEX_DEV_UINT_PROP("product_id", 0x0002u),
            IBEX_DEV_UINT_PROP("revision_id", 0x1u),
            IBEX_DEV_BOOL_PROP("volatile_raw_unlock", true),
            IBEX_DEV_UINT_PROP("kmac-app", 1u)
        )
    },
    [OT_EG_SOC_DEV_ALERT_HANDLER] = {
        /* qemu-passes drop-in: the SoC's alert COLLECTOR is now the
         * auto-generated model — every peripheral's OT_DEVICE_ALERT line
         * lands on the generated alert_tx_i diff pairs (alert_in organ).
         * Escalation outputs / ping-timer semantics: later phase. */
        .type = TYPE_OT_ALERT_HANDLER_QP,
        .memmap = MEMMAPENTRIES(
            { .base = 0x40150000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 127),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(1, PLIC, 128),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(2, PLIC, 129),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(3, PLIC, 130)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "alert_handler")
        ),
    },
    [OT_EG_SOC_DEV_SPI_HOST0] = {
        /* [soc_gen] generated from soc/earlgrey.soc.json (+ soc.digest.json); regenerate with `python3 soc/soc_glue.py write` — do not hand-edit. */
        /* qemu-passes drop-in: route SPI_HOST0 through spi_host_qp_shim.c,
         * which wraps the auto-generated model in qemu_passes/spi_host.c.
         * SPI_HOST1 stays on TYPE_OT_SPI_HOST upstream so a regression
         * here is isolated to SPI_HOST0. */
        .type = TYPE_OT_SPI_HOST_QP,
        .instance = IBEX_MAKE_INSTANCE_NUM(0),
        .memmap = MEMMAPENTRIES(
            { .base = 0x40300000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 131),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(1, PLIC, 132),
            OT_EG_SOC_GPIO_ALERT(0, 19)
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("clock-src", CLKMGR)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "spi0"),
            IBEX_DEV_UINT_PROP("bus-num", 0),
            IBEX_DEV_STRING_PROP("clock-name", "peri.io_div4"),
            IBEX_DEV_UINT_PROP("version", 2u)
        ),
    },
    [OT_EG_SOC_DEV_SPI_HOST1] = {
        .type = TYPE_OT_SPI_HOST,
        .instance = IBEX_MAKE_INSTANCE_NUM(1),
        .memmap = MEMMAPENTRIES(
            { .base = 0x40310000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 133),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(1, PLIC, 134),
            OT_EG_SOC_GPIO_ALERT(0, 20)
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("clock-src", CLKMGR)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "spi1"),
            IBEX_DEV_UINT_PROP("bus-num", 1),
            IBEX_DEV_STRING_PROP("clock-name", "peri.io_div4"),
            IBEX_DEV_UINT_PROP("version", 2u)
        ),
    },
    [OT_EG_SOC_DEV_USBDEV] = {
        .type = TYPE_OT_USBDEV,
        .cfg = &ot_eg_soc_usbdev_configure,
        .memmap = MEMMAPENTRIES(
            { .base = 0x40320000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 135),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(1, PLIC, 136),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(2, PLIC, 137),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(3, PLIC, 138),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(4, PLIC, 139),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(5, PLIC, 140),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(6, PLIC, 141),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(7, PLIC, 142),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(8, PLIC, 143),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(9, PLIC, 144),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(10, PLIC, 145),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(11, PLIC, 146),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(12, PLIC, 147),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(13, PLIC, 148),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(14, PLIC, 149),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(15, PLIC, 150),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(16, PLIC, 151),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(17, PLIC, 152),
            OT_EG_SOC_GPIO_ALERT(0, 21)
            /* The VBUS sense pin is handled by a chardev */
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("clock-src", CLKMGR)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "usbdev"),
            IBEX_DEV_STRING_PROP("clock-name", "usb"),
            IBEX_DEV_STRING_PROP("clock-name-aon", "aon")
        ),
    },
    [OT_EG_SOC_DEV_PWRMGR] = {
        .type = TYPE_OT_PWRMGR,
        .memmap = MEMMAPENTRIES(
            { .base = 0x40400000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 153),
            OT_EG_SOC_GPIO_ALERT(0, 22),
            OT_EG_SOC_REQ(OT_PWRMGR_OTP, OTP_CTRL),
            OT_EG_SOC_REQ(OT_PWRMGR_LC, LC_CTRL),
            OT_EG_SOC_SIGNAL(OT_PWRMGR_CPU_EN, 0, IBEX_WRAPPER,
                             OT_IBEX_WRAPPER_CPU_EN,
                             OT_IBEX_PWRMGR_CPU_EN),
            /* todo: add pwmgr GPIO strap when Earlgrey GPIO is updated */
            OT_EG_SOC_SIGNAL(OT_PWRMGR_RST_REQ, 0, RSTMGR,
                             OT_RSTMGR_RST_REQ, 0)
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("clock-ctrl", AST)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP("clocks", "main,io,usb"),
            IBEX_DEV_UINT_PROP("num-rom", 1u),
            IBEX_DEV_UINT_PROP("version", OT_PWRMGR_VERSION_EG_1_0_0)
        ),
    },
    [OT_EG_SOC_DEV_RSTMGR] = {
        .type = TYPE_OT_RSTMGR,
        .memmap = MEMMAPENTRIES(
            { .base = 0x40410000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_SIGNAL(OT_RSTMGR_SW_RST, 0, PWRMGR, \
                                   OT_PWRMGR_SW_RST, 0),
            OT_EG_SOC_GPIO_ALERT(0, 23),
            OT_EG_SOC_GPIO_ALERT(1, 24)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_UINT_PROP("version", OT_RSTMGR_VERSION_EG_1_0_0)
        ),
    },
    [OT_EG_SOC_DEV_CLKMGR] = {
        .type = TYPE_OT_CLKMGR,
        .memmap = MEMMAPENTRIES(
            { .base = 0x40420000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_ALERT(0, 25),
            OT_EG_SOC_GPIO_ALERT(1, 26)
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("clock-src", AST)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_UINT_PROP("version", OT_CLKMGR_VERSION_EG_1_0_0)
        ),
    },
    [OT_EG_SOC_DEV_SYSRST_CTRL] = {
        /* qemu-passes drop-in (was TYPE_OT_UNIMP): auto-generated sysrst
         * controller.  event_detected -> PLIC 154 per the digest. */
        .type = TYPE_OT_SYSRST_CTRL_QP,
        .memmap = MEMMAPENTRIES(
            { .base = 0x40430000u }
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "sysrst_ctrl")
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 154),
            OT_EG_SOC_GPIO_ALERT(0, 27)
        )
    },
    [OT_EG_SOC_DEV_ADC_CTRL] = {
        /* qemu-passes drop-in (was TYPE_OT_UNIMP): auto-generated ADC
         * controller.  match_pending -> PLIC 155 per the digest. */
        .type = TYPE_OT_ADC_CTRL_QP,
        .memmap = MEMMAPENTRIES(
            { .base = 0x40440000u }
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "adc_ctrl")
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 155),
            OT_EG_SOC_GPIO_ALERT(0, 28)
        )
    },
    [OT_EG_SOC_DEV_PWM] = {
        /* [soc_gen] generated from soc/earlgrey.soc.json (+ soc.digest.json); regenerate with `python3 soc/soc_glue.py write` — do not hand-edit. */
        /* qemu-passes drop-in: pwm was an unimp placeholder upstream;
         * we replace it with a real auto-generated model.  6 cio_pwm
         * outputs go nowhere in frontend-only mode (no external pad
         * receiver), so dropping the cio* GPIO connections is fine. */
        .type = TYPE_OT_PWM_QP,
        .memmap = MEMMAPENTRIES(
            { .base = 0x40450000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_ALERT(0, 29)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "pwm")
        ),
    },
    [OT_EG_SOC_DEV_DMA] = {
        /* Experimental: qemu-passes drop-in for the OT secure DMA.  Upstream
         * EarlGrey doesn't include DMA (it's only in TopDarjeeling), so we
         * place it at an unused base 0x404a0000.  COPY-mode transfers run
         * for real: the generated bus_master organ executes the engine's
         * TL-UL host beats against the system AddressSpace.  Hardware
         * handshake: lsio_trigger_i is a qdev gpio-in (pin_io role); see
         * the UART0_RX_DMA splitter for the declared trigger-0 wiring.
         * prim_sha2_32 stays stubbed (opentitan-overlay/), so SHA opcode
         * modes are no-ops. */
        .type = TYPE_OT_DMA_QP,
        .memmap = MEMMAPENTRIES(
            { .base = 0x404a0000u }
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "dma")
        )
    },
    [OT_EG_SOC_DEV_PINMUX] = {
        /* [soc_gen] generated from soc/earlgrey.soc.json (+ soc.digest.json); regenerate with `python3 soc/soc_glue.py write` — do not hand-edit. */
        /* qemu-passes drop-in: routes pinmux through pinmux_qp_shim.c
         * which wraps the auto-generated model.  Pinmux's 568-bit
         * addr_hit / 2047-bit one-hot trees exercise Bug G's wide-array
         * (uint64_t arr[N]) storage path. */
        .type = TYPE_OT_PINMUX_QP,
        .memmap = MEMMAPENTRIES(
            { .base = 0x40460000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_ALERT(0, 30)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "pinmux")
        ),
    },
    [OT_EG_SOC_DEV_AON_TIMER] = {
        /* [soc_gen] generated from soc/earlgrey.soc.json (+ soc.digest.json); regenerate with `python3 soc/soc_glue.py write` — do not hand-edit. */
        /* qemu-passes drop-in: routes aon_timer through aon_timer_qp_shim.c
         * which wraps the auto-generated model.  AON_TIMER_WKUP /
         * AON_TIMER_BITE named-GPIO outputs to PWRMGR are dropped — the
         * QP shim doesn't register those output GPIO lines (frontend-only
         * doesn't exercise PWRMGR wakeup/reset paths). */
        .type = TYPE_OT_AON_TIMER_QP,
        .memmap = MEMMAPENTRIES(
            { .base = 0x40470000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 156),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(1, PLIC, 157),
            OT_EG_SOC_GPIO_ALERT(0, 31)
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("clock-src", CLKMGR)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP("clock-name", "timers.io_div4"),
            IBEX_DEV_STRING_PROP("clock-name-aon", "timers.aon")
        ),
    },
    [OT_EG_SOC_DEV_AST] = {
        .type = TYPE_OT_AST_EG,
        .cfg = &ot_eg_soc_ast_configure,
        .memmap = MEMMAPENTRIES(
            { .base = 0x40480000u }
        ),
    },
    [OT_EG_SOC_DEV_SENSOR_CTRL] = {
        .type = TYPE_OT_SENSOR_EG,
        .memmap = MEMMAPENTRIES(
            { .base = 0x40490000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_ALERT(0, 32),
            OT_EG_SOC_GPIO_ALERT(1, 33)
        ),
    },
    [OT_EG_SOC_DEV_SRAM_RET_CTRL] = {
        .type = TYPE_OT_SRAM_CTRL,
        .memmap = MEMMAPENTRIES(
            { .base = 0x40500000u },
            { .base = 0x40600000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_ALERT(0, 34)
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("otp-ctrl", OTP_CTRL),
            OT_EG_SOC_DEVLINK("vmapper", VMAPPER)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_UINT_PROP("size", 0x1000u),
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "ret"),
            IBEX_DEV_BOOL_PROP("ifetch", false)
        ),
    },
    [OT_EG_SOC_DEV_FLASH_CTRL] = {
        .type = TYPE_OT_FLASH,
        .cfg = &ot_eg_soc_flash_ctrl_configure,
        .memmap = MEMMAPENTRIES(
            { .base = 0x41000000u },
            { .base = 0x41008000u },
            { .base = 0x20000000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 160),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(1, PLIC, 161),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(2, PLIC, 162),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(3, PLIC, 163),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(4, PLIC, 164),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(5, PLIC, 165),
            OT_EG_SOC_GPIO_ALERT(0, 35),
            OT_EG_SOC_GPIO_ALERT(1, 36),
            OT_EG_SOC_GPIO_ALERT(2, 37),
            OT_EG_SOC_GPIO_ALERT(3, 38),
            OT_EG_SOC_GPIO_ALERT(4, 39)
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("vmapper", VMAPPER)
        ),
    },
    [OT_EG_SOC_DEV_AES] = {
        /* qemu-passes drop-in: auto-generated AES model (aes_qp_shim.c),
         * FIPS-197 KAT-verified.  Translated with the Earl Grey instance
         * configuration (GCM disabled) and the functional unmasked
         * simplification (ciphertext is bit-identical; masking is a
         * side-channel countermeasure the MMIO model cannot observe).
         * clock/edn links and the keymgr sideload sink are not modeled. */
        .type = TYPE_OT_AES_QP,
        .memmap = MEMMAPENTRIES(
            { .base = 0x41100000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_ALERT(0, 42)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "aes")
        ),
    },
    [OT_EG_SOC_DEV_HMAC] = {
        /* qemu-passes drop-in: HMAC routes through the auto-generated
         * SHA-256 engine (hmac_qp_shim.c), model-level NIST-verified
         * (SHA-256("abc") byte-exact).  A pure COMPUTE device: no
         * chardev / pins / pump / bus-master — MMIO + the 3 irq lines
         * + alert are its entire boundary.  HMAC keyed mode is campaign
         * phase 2 (i_pad WIDE-DROP + 512-bit variable shifts). */
        .type = TYPE_OT_HMAC_QP,
        .memmap = MEMMAPENTRIES(
            { .base = 0x41110000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 166),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(1, PLIC, 167),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(2, PLIC, 168),
            OT_EG_SOC_GPIO_ALERT(0, 44)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "hmac")
        ),
    },
    [OT_EG_SOC_DEV_KMAC] = {
        /* qemu-passes drop-in: auto-generated KMAC/SHA-3 model
         * (kmac_qp_shim.c), SHA3-256 NIST KAT-verified on the host
         * harness.  Translated unmasked (-G EnMasking=0: digests are
         * bit-identical; masking is a side-channel countermeasure the
         * MMIO model cannot observe).  EDN and lc_escalate are tied in
         * the shim; the keymgr/lc/rom app interfaces and clock/edn
         * links are not modeled (software SHA-3 path only). */
        .type = TYPE_OT_KMAC_QP,
        .memmap = MEMMAPENTRIES(
            { .base = 0x41120000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 169),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(1, PLIC, 170),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(2, PLIC, 171),
            /* single collapsed alert line (shim exposes one OT_DEVICE_ALERT;
             * recov/fatal split is an alert-plane phase-2 item) */
            OT_EG_SOC_GPIO_ALERT(0, 45)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "kmac")
        ),
    },
    [OT_EG_SOC_DEV_KMAC_APP_SVC] = {
        /* TRANSITIONAL: unmapped native ot-kmac serving only the C-level
         * hardware-app interface (rom_ctrl boot digest, keymgr KDF,
         * lc_ctrl), which the generated model does not yet expose.  No
         * MMIO mapping: software always reaches the generated
         * TYPE_OT_KMAC_QP above.  Real app channel = integrity-group
         * phase 2. */
        .type = TYPE_OT_KMAC,
        .memmap = MEMMAPENTRIES(
            /* parked in the unused hole behind the real kmac window;
             * firmware never addresses it */
            { .base = 0x41128000u }
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("clock-src", CLKMGR),
            OT_EG_SOC_DEVLINK("edn", EDN0)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP("clock-name", "trans.kmac"),
            IBEX_DEV_UINT_PROP("edn-ep", 3u),
            IBEX_DEV_UINT_PROP("num-app", 3u)
        ),
    },
    [OT_EG_SOC_DEV_ROM_CTRL_QP] = {
        /* qemu-passes generated rom_ctrl, PARALLEL mount (native rom_ctrl
         * untouched): region 0 = scrambled-ROM window (rom_tl, primary
         * port), region 1 = CSRs (regs_tl aux port).  The boot digest
         * flow runs against the generated kmac via the signal-level
         * bridge in ot_eg_soc_reset_exit; the ROM image is injected via
         * -global ot-rom-ctrl-qp.backing0-image=<file>. */
        .type = TYPE_OT_ROM_CTRL_QP,
        .memmap = MEMMAPENTRIES(
            { .base = 0x411b0000u },
            { .base = 0x411a0000u }
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "rom_ctrl_qp")
        ),
    },
    [OT_EG_SOC_DEV_ROM_CTRL_QPP] = {
        /* HEART SWAP primary: second instance of the generated
         * rom_ctrl.  region 1 (regs_tl CSRs) sits at the NATIVE
         * rom_ctrl CSR base — DIGEST/EXP_DIGEST of the actual boot
         * image are served by the generated model.  region 0 (the
         * model's own TL ROM-window port) is parked: CPU fetch is
         * served by the cleartext rom_device in ROM_CTRL_QP_BOOT. */
        .type = TYPE_OT_ROM_CTRL_QP,
        .memmap = MEMMAPENTRIES(
            { .base = 0x411ac000u },
            { .base = 0x411e0000u }
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "rom_ctrl_qpp")
        ),
    },
    [OT_EG_SOC_DEV_ROM_CTRL_QP_BOOT] = {
        /* HEART SWAP boot front-end: cleartext CPU-fetch ROM window at
         * 0x8000 (rom_device, ROMD after the check passes), the native
         * image loaders (-object ot-rom_img,id=rom), and the pwrmgr
         * done/good lines — raised from the GENERATED model's verdict
         * (see ot_rom_ctrl_qp_boot.c). */
        .type = TYPE_OT_ROM_CTRL_QP_BOOT,
        .memmap = MEMMAPENTRIES(
            { .base = 0x00008000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_SIGNAL(OT_ROM_CTRL_GOOD, 0, PWRMGR, \
                                   OT_PWRMGR_ROM_GOOD, 0),
            OT_EG_SOC_SIGNAL(OT_ROM_CTRL_DONE, 0, PWRMGR, \
                                   OT_PWRMGR_ROM_DONE, 0)
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("rom-ctrl-qp", ROM_CTRL_QPP),
            OT_EG_SOC_DEVLINK("kmac-qp", KMAC)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "rom_boot"),
            IBEX_DEV_STRING_PROP("img-id", "rom"),
            IBEX_DEV_UINT_PROP("size", 0x8000u)
        ),
    },
    [OT_EG_SOC_DEV_KEYMGR_QP] = {
        /* qemu-passes generated keymgr, PARALLEL mount.  Wide OTP/flash
         * seed inputs and the rom_ctrl digest are injected by
         * ot_eg_keymgr_qp_wire() at reset-exit; kmac app channel 0 is
         * bridged per-tick via the core's _qp_before_tick hook. */
        .type = TYPE_OT_KEYMGR_QP,
        .memmap = MEMMAPENTRIES(
            { .base = 0x411d0000u }
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "keymgr_qp")
        ),
    },
    [OT_EG_SOC_DEV_LC_CTRL_QP] = {
        /* qemu-passes generated lc_ctrl, PARALLEL mount.  region 0 =
         * dmi_tl (primary port), region 1 = regs_tl CSRs (aux port).
         * The PROD OTP life-cycle image, lc_tx handshake acks and esc
         * differential idles are injected at reset-exit; the kmac app
         * channel 1 (transition token) bridge is a v2 item. */
        .type = TYPE_OT_LC_CTRL_QP,
        .memmap = MEMMAPENTRIES(
            { .base = 0x41198000u },
            { .base = 0x41190000u }
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "lc_ctrl_qp")
        ),
    },
    [OT_EG_SOC_DEV_ENTROPY_SRC_QP] = {
        /* qemu-passes generated entropy_src, PARALLEL mount.  fw_ov
         * insert -> SHA3-384 conditioner; seed leaves over the es hw
         * wire in the ring pump. */
        .type = TYPE_OT_ENTROPY_SRC_QP,
        .memmap = MEMMAPENTRIES(
            { .base = 0x411a4000u }
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "entropy_src_qp")
        ),
    },
    [OT_EG_SOC_DEV_CSRNG_QP] = {
        /* qemu-passes generated csrng (CTR_DRBG with the real AES
         * cipher core), PARALLEL mount. */
        .type = TYPE_OT_CSRNG_QP,
        .memmap = MEMMAPENTRIES(
            { .base = 0x411a5000u }
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "csrng_qp")
        ),
    },
    [OT_EG_SOC_DEV_EDN_QP] = {
        /* qemu-passes generated edn, PARALLEL mount.  SW command port
         * drives the generated csrng over the ring bridge. */
        .type = TYPE_OT_EDN_QP,
        .memmap = MEMMAPENTRIES(
            { .base = 0x411a6000u }
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "edn_qp")
        ),
    },
    [OT_EG_SOC_DEV_OTBN] = {
        .type = TYPE_OT_OTBN,
        .memmap = MEMMAPENTRIES(
            { .base = 0x41130000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 172),
            OT_EG_SOC_GPIO_ALERT(0, 47),
            OT_EG_SOC_GPIO_ALERT(1, 48)
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("clock-src", CLKMGR),
            OT_EG_SOC_DEVLINK("edn-u", EDN0),
            OT_EG_SOC_DEVLINK("edn-r", EDN1)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP("clock-name", "trans.otbn"),
            IBEX_DEV_UINT_PROP("edn-u-ep", 6u),
            IBEX_DEV_UINT_PROP("edn-r-ep", 0u)
        ),
    },
    [OT_EG_SOC_DEV_KEYMGR] = {
        .type = TYPE_OT_KEYMGR,
        .memmap = MEMMAPENTRIES(
            { .base = 0x41140000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 173),
            OT_EG_SOC_GPIO_ALERT(0, 49),
            OT_EG_SOC_GPIO_ALERT(1, 50)
        ),
        .link = IBEXDEVICELINKDEFS(
            /* QP AES implements TYPE_OT_KEY_SINK_IF with a no-op push_key
             * (sideload key material not modeled; the keymgr asserts if the
             * sink link is missing). */
            OT_EG_SOC_DEVLINK("aes", AES),
            OT_EG_SOC_DEVLINK("edn", EDN0),
            OT_EG_SOC_DEVLINK("flash_ctrl", FLASH_CTRL),
            OT_EG_SOC_DEVLINK("lc-ctrl", LC_CTRL),
            OT_EG_SOC_DEVLINK("otbn", OTBN),
            OT_EG_SOC_DEVLINK("otp-ctrl", OTP_CTRL),
            OT_EG_SOC_DEVLINK("rom_ctrl", ROM_CTRL),
            OT_EG_SOC_DEVLINK("kmac", KMAC_APP_SVC)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_UINT_PROP("edn-ep", 0u),
            IBEX_DEV_UINT_PROP("kmac-app", 0u)
        ),
    },
    [OT_EG_SOC_DEV_CSRNG] = {
        .type = TYPE_OT_CSRNG,
        .memmap = MEMMAPENTRIES(
            { .base = 0x41150000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 174),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(1, PLIC, 175),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(2, PLIC, 176),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(3, PLIC, 177),
            OT_EG_SOC_GPIO_ALERT(0, 51),
            OT_EG_SOC_GPIO_ALERT(1, 52)
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("entropy-src", ENTROPY_SRC),
            OT_EG_SOC_DEVLINK("otp-ctrl", OTP_CTRL)
        ),
    },
    [OT_EG_SOC_DEV_ENTROPY_SRC] = {
        .type = TYPE_OT_ENTROPY_SRC,
        .memmap = MEMMAPENTRIES(
            { .base = 0x41160000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 178),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(1, PLIC, 179),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(2, PLIC, 180),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(3, PLIC, 181),
            OT_EG_SOC_GPIO_ALERT(0, 53),
            OT_EG_SOC_GPIO_ALERT(1, 54)
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("noise-src", AST)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_UINT_PROP("version", 2)
        ),
    },
    [OT_EG_SOC_DEV_EDN0] = {
        .type = TYPE_OT_EDN,
        .memmap = MEMMAPENTRIES(
            { .base = 0x41170000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 182),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(1, PLIC, 183),
            OT_EG_SOC_GPIO_ALERT(0, 55),
            OT_EG_SOC_GPIO_ALERT(1, 56)
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("csrng", CSRNG)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_UINT_PROP("csrng-app", 0u)
        ),
    },
    [OT_EG_SOC_DEV_EDN1] = {
        .type = TYPE_OT_EDN,
        .memmap = MEMMAPENTRIES(
            { .base = 0x41180000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_SYSBUS_IRQ(0, PLIC, 184),
            OT_EG_SOC_GPIO_SYSBUS_IRQ(1, PLIC, 185),
            OT_EG_SOC_GPIO_ALERT(0, 57),
            OT_EG_SOC_GPIO_ALERT(1, 58)
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("csrng", CSRNG)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_UINT_PROP("csrng-app", 1u)
        ),
    },
    [OT_EG_SOC_DEV_SRAM_MAIN_CTRL] = {
        .type = TYPE_OT_SRAM_CTRL,
        .memmap = MEMMAPENTRIES(
            { .base = 0x411c0000u },
            { .base = 0x10000000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_ALERT(0, 59)
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("otp-ctrl", OTP_CTRL),
            OT_EG_SOC_DEVLINK("vmapper", VMAPPER)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_UINT_PROP("size", SRAM_MAIN_SIZE),
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "ram"),
            IBEX_DEV_BOOL_PROP("ifetch", true)
        ),
    },
    [OT_EG_SOC_DEV_ROM_CTRL] = {
        /* HEART SWAP: PARKED (regs/mem moved to unused holes, pwrmgr
         * signals disconnected, "load" no longer triggered).  Kept
         * instantiated only because the native keymgr links to it
         * (get_rom_digest class API).  The machine's real rom_ctrl is
         * the generated ROM_CTRL_QPP + ROM_CTRL_QP_BOOT pair. */
        .type = TYPE_OT_ROM_CTRL,
        .memmap = MEMMAPENTRIES(
            { .base = 0x411a8000u },
            { .base = 0x411b8000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_ALERT(0, 60)
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("kmac", KMAC_APP_SVC)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "rom"),
            IBEX_DEV_UINT_PROP("size", 0x8000u),
            IBEX_DEV_UINT_PROP("kmac-app", 2u)
        ),
    },
    [OT_EG_SOC_DEV_IBEX_WRAPPER] = {
        .type = TYPE_OT_IBEX_WRAPPER,
        .memmap = MEMMAPENTRIES(
            { .base = 0x411f0000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO_ALERT(0, 61),
            OT_EG_SOC_GPIO_ALERT(1, 62),
            OT_EG_SOC_GPIO_ALERT(2, 63),
            OT_EG_SOC_GPIO_ALERT(3, 64)
        ),
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("edn", EDN0),
            OT_EG_SOC_DEVLINK("vmapper", VMAPPER)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_UINT_PROP("edn-ep", 7u),
            IBEX_DEV_UINT_PROP("num-regions", OT_EG_IBEX_WRAPPER_NUM_REGIONS)
        ),
    },
    [OT_EG_SOC_DEV_RV_DM] = {
        .type = TYPE_PULP_RV_DM,
        .memmap = MEMMAPENTRIES(
            { .base = PULP_DM_BASE },
            { .base = 0x41200000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_DM_CONNECTION(OT_EG_SOC_DEV_DM, 0),
            OT_EG_SOC_DM_CONNECTION(OT_EG_SOC_DEV_DM, 1),
            OT_EG_SOC_DM_CONNECTION(OT_EG_SOC_DEV_DM, 2),
            OT_EG_SOC_DM_CONNECTION(OT_EG_SOC_DEV_DM, 3),
            OT_EG_SOC_GPIO_ALERT(0, 40)
        ),
    },
    [OT_EG_SOC_DEV_DM_LC_CTRL] = {
        .type = TYPE_OT_DM_TL,
        .link = IBEXDEVICELINKDEFS(
            OT_EG_SOC_DEVLINK("dtm", LC_CTRL_DTM),
            OT_EG_SOC_DEVLINK("tl_dev", LC_CTRL)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_UINT_PROP("dmi_addr", OT_EG_LC_CTRL_TAP_DMI_ADDR),
            IBEX_DEV_UINT_PROP("dmi_size", OT_EG_LC_CTRL_TAP_DMI_SIZE),
            IBEX_DEV_UINT_PROP("tl_addr", OT_EG_LC_CTRL_TAP_TL_ADDR),
            IBEX_DEV_STRING_PROP("tl_as_name", OT_EG_LC_CTRL_TAP_AS)
        )
    },
    [OT_EG_SOC_DEV_PLIC] = {
        /* qemu-passes drop-in: the machine's interrupt hub is the
         * auto-generated OT rv_plic wrapped by rv_plic_qp_shim.c.  One
         * generated model covers what upstream split across TWO devices:
         * TYPE_SIFIVE_PLIC (prio/ie/threshold/claim @48000000) plus
         * TYPE_OT_PLIC_EXT (OT's MSIP + alert extension @4c000000 — that is
         * offset 0x4000000 INSIDE the real rv_plic register block, which the
         * generated model decodes natively; the EXT entry below is disabled
         * to free the address range).  Interrupt sources arrive on qdev
         * gpio-in lines 0..185 (the pin_io.data_in role); sysbus IRQ 0 is
         * the external interrupt to the hart, 1 the software interrupt
         * (out_lines role: irq_o / msip_o); the alert-test alert stays on
         * escalation line 41 as with the EXT device. */
        .type = TYPE_OT_RV_PLIC_QP,
        .memmap = MEMMAPENTRIES(
            { .base = 0x48000000u }
        ),
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO(0, HART, IRQ_M_EXT),
            OT_EG_SOC_GPIO(1, HART, IRQ_M_SOFT),
            OT_EG_SOC_GPIO_ALERT(0, 41)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "rv_plic")
        ),
    },
    /* [qp] OT_EG_SOC_DEV_PLIC_EXT intentionally absent: its register space
     * (0x4c000000 = rv_plic + 0x4000000, the MSIP bank) and both of its
     * lines (IRQ_M_SOFT, alert 41) are covered by the generated rv_plic
     * above.  The enum slot stays; an empty definition creates no device. */
    [OT_EG_SOC_DEV_VMAPPER] = {
        .type = TYPE_OT_VMAPPER,
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_STRING_PROP(OT_COMMON_DEV_ID, "soc"),
            IBEX_DEV_UINT_PROP("trans_count", OT_EG_IBEX_WRAPPER_NUM_REGIONS)
        ),
    },
    /* IRQ splitters */
    [OT_EG_SOC_SPLITTER_LC_HW_DEBUG] = {
        .type = TYPE_SPLIT_IRQ,
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_S2D(0, SRAM_MAIN_CTRL, OT_SRAM_CTRL_HW_DEBUG_EN, 0),
            OT_EG_SOC_S2D(1, SRAM_RET_CTRL, OT_SRAM_CTRL_HW_DEBUG_EN, 0)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_UINT_PROP("num-lines", 2u) /* @todo to be changed */
        )
    },
    [OT_EG_SOC_SPLITTER_LC_ESCALATE] = {
        .type = TYPE_SPLIT_IRQ,
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_S2D(0, OTP_CTRL, OT_LC_BROADCAST,
                          OT_OTP_LC_ESCALATE_EN),
            OT_EG_SOC_S2D(1, FLASH_CTRL, OT_LC_BROADCAST,
                          OT_FLASH_LC_ESCALATE_EN)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_UINT_PROP("num-lines", 2u) /* @todo to be changed */
        )
    },
    [OT_EG_SOC_SPLITTER_LC_SEED_HW_RD] = {
        .type = TYPE_SPLIT_IRQ,
        .gpio = IBEXGPIOCONNDEFS(
          OT_EG_SOC_S2D(0, OTP_CTRL, OT_LC_BROADCAST,
                        OT_OTP_LC_SEED_HW_RD_EN),
          OT_EG_SOC_S2D(1, FLASH_CTRL, OT_LC_BROADCAST,
                        OT_FLASH_LC_SEED_HW_RD_EN)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_UINT_PROP("num-lines", 2u)
        )
    },
    [OT_EG_SOC_SPLITTER_LC_CREATOR_SEED_SW_RW] = {
        .type = TYPE_SPLIT_IRQ,
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_S2D(0, OTP_CTRL, OT_LC_BROADCAST,
                          OT_OTP_LC_CREATOR_SEED_SW_RW_EN),
            OT_EG_SOC_S2D(1, FLASH_CTRL, OT_LC_BROADCAST,
                          OT_FLASH_LC_CREATOR_SEED_SW_RW_EN)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_UINT_PROP("num-lines", 2u)
        )
    },
    [OT_EG_SOC_SPLITTER_UART0_RX_DMA] = {
        /* [qemu-passes irq tap] uart0 rx_watermark -> {PLIC 2, DMA hs
         * trigger 0}.  The PLIC leg preserves the digest-checked slot;
         * the DMA leg is the declared lsio trigger mapping (trigger 0 =
         * uart0 rx watermark) driving the generated model's
         * hardware-handshake mode through its unnamed gpio-in 0. */
        .type = TYPE_SPLIT_IRQ,
        .gpio = IBEXGPIOCONNDEFS(
            OT_EG_SOC_GPIO(0, PLIC, 2),
            OT_EG_SOC_GPIO(1, DMA, 0)
        ),
        .prop = IBEXDEVICEPROPDEFS(
            IBEX_DEV_UINT_PROP("num-lines", 2u)
        )
    },
    /* clang-format on */
};

/* ------------------------------------------------------------------------ */
/* Type definitions */
/* ------------------------------------------------------------------------ */

/* Temporary storage for reset iteration */
typedef struct {
    OtEGSoCState *soc;
    ResettableChildCallback cb; /* the callback to call for each child */
    void *opaque; /* opaque data for the callback */
    ResetType type; /* type of reset */
} OtEgSocChildReset;

struct OtEGSoCClass {
    DeviceClass parent_class;
    DeviceRealize parent_realize;
    ResettablePhases parent_phases;
};

struct OtEGSoCState {
    SysBusDevice parent_obj;

    BusState *ot_bus; /* private OpenTitan bus */

    DeviceState **devices;
};

struct OtEGBoardState {
    DeviceState parent_obj;

    DeviceState **devices;

    /* optional SPI data flash (type of device) */
    char *spiflash[OT_EG_MTD_SPI_COUNT];
};

struct OtEGMachineState {
    MachineState parent_obj;

    ResettableState reset;

    bool no_epmp_cfg;
    bool ignore_elf_entry;
    bool verilator;
};

struct OtEGMachineClass {
    MachineClass parent_class;
    ResettablePhases parent_phases;
};

/* ------------------------------------------------------------------------ */
/* Device Configuration */
/* ------------------------------------------------------------------------ */

static void ot_eg_soc_ast_configure(DeviceState *dev, const IbexDeviceDef *def,
                                    DeviceState *parent)
{
    (void)def;
    (void)parent;

    bool verilator_mode =
        object_property_get_bool(qdev_get_machine(), "verilator", NULL);
    const char *clock_cfg;
    if (!verilator_mode) {
        /* EarlGrey/CW310 */
        clock_cfg = "main:24000000,io:24000000,usb:48000000,aon:250000";
    } else {
        clock_cfg = "main:500000,io:500000,usb:500000,aon:125000";
    }

    qdev_prop_set_string(dev, "topclocks", clock_cfg);
}

static void ot_eg_soc_dm_configure(DeviceState *dev, const IbexDeviceDef *def,
                                   DeviceState *parent)
{
    (void)def;
    (void)parent;

    QList *hart = qlist_new();
    qlist_append_int(hart, 0);
    qdev_prop_set_array(dev, "hart", hart);

    RISCVDMMemAttrs pulp_attrs = {
        .attrs = {
            .requester_id = PULP_RV_DM_REQUESTER_ID,
        },
    };
    qdev_prop_set_uint64(dev, "mta_dm", pulp_attrs.value);
}

static void ot_eg_soc_flash_ctrl_configure(
    DeviceState *dev, const IbexDeviceDef *def, DeviceState *parent)
{
    DriveInfo *dinfo = drive_get(IF_MTD, OT_EG_MTD_EFLASH, 0);
    (void)def;
    (void)parent;

    if (dinfo) {
        qdev_prop_set_drive_err(dev, "drive", blk_by_legacy_dinfo(dinfo),
                                &error_fatal);
    }
}

static void ot_eg_soc_hart_configure(DeviceState *dev, const IbexDeviceDef *def,
                                     DeviceState *parent)
{
    OtEGMachineState *ms = RISCV_OT_EG_MACHINE(qdev_get_machine());
    QList *pmp_cfg, *pmp_addr;
    (void)def;
    (void)parent;

    if (ms->no_epmp_cfg) {
        /* skip default PMP config */
        return;
    }

    pmp_cfg = qlist_new();
    for (unsigned ix = 0; ix < ARRAY_SIZE(ot_eg_pmp_cfgs); ix++) {
        qlist_append_int(pmp_cfg, ot_eg_pmp_cfgs[ix]);
    }
    qdev_prop_set_array(dev, "pmp_cfg", pmp_cfg);

    pmp_addr = qlist_new();
    for (unsigned ix = 0; ix < ARRAY_SIZE(ot_eg_pmp_addrs); ix++) {
        qlist_append_int(pmp_addr, ot_eg_pmp_addrs[ix]);
    }
    qdev_prop_set_array(dev, "pmp_addr", pmp_addr);

    qdev_prop_set_uint64(dev, "mseccfg", (uint64_t)OT_EG_MSECCFG);
}

static void ot_eg_soc_otp_ctrl_configure(
    DeviceState *dev, const IbexDeviceDef *def, DeviceState *parent)
{
    DriveInfo *dinfo = drive_get(IF_PFLASH, OT_EG_PFLASH_OTP, 0);
    (void)def;
    (void)parent;

    if (dinfo) {
        qdev_prop_set_drive_err(dev, "drive", blk_by_legacy_dinfo(dinfo),
                                &error_fatal);
    }
}

static void ot_eg_soc_tap_ctrl_configure(
    DeviceState *dev, const IbexDeviceDef *def, DeviceState *parent)
{
    (void)parent;
    (void)def;

    Chardev *chr;

    chr = ibex_get_chardev_by_id("taprbb");
    if (chr) {
        qdev_prop_set_chr(dev, "chardev", chr);
    }
}

static void ot_eg_soc_lc_ctrl_tap_ctrl_configure(
    DeviceState *dev, const IbexDeviceDef *def, DeviceState *parent)
{
    (void)parent;
    (void)def;

    Chardev *chr;

    chr = ibex_get_chardev_by_id("taprbb-lc-ctrl");
    if (chr) {
        qdev_prop_set_chr(dev, "chardev", chr);
    }
}

/* qemu-passes: the QP spi_device shim exposes the upstream `chardev`
 * property for its SPI-slave transport (spidev "/CS" framing, same host-side
 * protocol/tooling as ot_spi_device.c: `-chardev socket,id=spidev,...`). */
static void ot_eg_soc_spi_device_configure(
    DeviceState *dev, const IbexDeviceDef *def, DeviceState *parent)
{
    (void)parent;
    (void)def;

    Chardev *chr;

    chr = ibex_get_chardev_by_id("spidev");
    if (chr) {
        qdev_prop_set_chr(dev, "chardev", chr);
    }
}

static void ot_eg_soc_uart_configure(DeviceState *dev, const IbexDeviceDef *def,
                                     DeviceState *parent)
{
    (void)def;
    (void)parent;
    qdev_prop_set_chr(dev, "chardev", serial_hd(IBEX_GET_INSTANCE_NUM(def)));
}

static void ot_eg_soc_usbdev_configure(
    DeviceState *dev, const IbexDeviceDef *def, DeviceState *parent)
{
    (void)parent;
    (void)def;

    Chardev *chr;

    chr = ibex_get_chardev_by_id("usbdev-cmd");
    if (chr) {
        qdev_prop_set_chr(dev, "chardev-cmd", chr);
    }
    chr = ibex_get_chardev_by_id("usbdev-host");
    if (chr) {
        qdev_prop_set_chr(dev, "chardev-usb", chr);
    }
}

/* ------------------------------------------------------------------------ */
/* SoC */
/* ------------------------------------------------------------------------ */

static int ot_eg_soc_reset_child(Object *child, void *opaque)
{
    OtEgSocChildReset *cr = opaque;

    if (object_dynamic_cast(child, TYPE_SYS_BUS_DEVICE)) {
        /*
         * sysbus devices being connected to the OT bus, and the bus performing
         * its own children traversal to perform reset, skip this kind of
         * devices to avoid resetting each of them twice
         */
        return 0;
    }

    if (object_dynamic_cast(child, TYPE_RESETTABLE_INTERFACE)) {
        cr->cb(child, cr->opaque, cr->type);
    }

    /* resume with next child */
    return 0;
}

static void ot_eg_soc_reset_child_foreach(
    Object *obj, ResettableChildCallback cb, void *opaque, ResetType type)
{
    OtEGSoCState *s = RISCV_OT_EG_SOC(obj);

    OtEgSocChildReset r = {
        .soc = s,
        .cb = cb,
        .opaque = opaque,
        .type = type,
    };

    /* execute reset stage for each child */
    object_child_foreach(obj, &ot_eg_soc_reset_child, &r);
}

static void ot_eg_soc_hw_reset(void *opaque, int irq, int level)
{
    OtEGSoCState *s = opaque;

    g_assert(irq == 0);

    if (level) {
        resettable_reset(OBJECT(s), RESET_TYPE_COLD);
    }
}

static void ot_eg_soc_reset_hold(Object *obj, ResetType type)
{
    OtEGSoCClass *c = RISCV_OT_EG_SOC_GET_CLASS(obj);
    OtEGSoCState *s = RISCV_OT_EG_SOC(obj);

    if (c->parent_phases.hold) {
        c->parent_phases.hold(obj, type);
    }

    /*
     * This function is called after all children have been reset_hold,
     * before any child has been reset_exit.
     *
     * Power-On-Reset: leave hart disabled on reset
     * PowerManager takes care of managing Ibex reset when ready
     */
    CPUState *cs = CPU(s->devices[OT_EG_SOC_DEV_HART]);
    cs->disabled = true;
}

/* Generated-to-generated boot bridge: co-step the generated rom_ctrl and
 * kmac models at signal level until the checker finishes (host cosim
 * verified byte-exact against a python cSHAKE-256 reference).  App
 * channel 2 is the RomCtrl cSHAKE channel by kmac_pkg AppCfg order. */
/* keymgr <-> kmac app channel 0: a QEMU timer LOCK-STEPS the two
 * generated models with the exact cadence the host cosim proved
 * (wire, update+update, tick+tick, update+update).  A coarse
 * step()+step() interleave skews the handshake phases; the per-tick
 * hook could not lock-step without re-entering keymgr.  The pump only
 * burns cycles while an operation is in flight. */
/* lc_ctrl: inject the PROD life-cycle OTP image + interface idles.
 * State/count words from lc_ctrl_state_pkg (RTL default key/nonce);
 * the broadcasts (lc_dft_en=Off, lc_cpu_en=On, lc_keymgr_en=On, ...)
 * and both CSR ports then decode from real logic instead of ties. */
static void ot_eg_lc_ctrl_qp_wire(DeviceState *lc_dev)
{
    lc_ctrl_state *l = ot_lc_ctrl_qp_core(lc_dev);
    static const uint64_t prod_state[5] = {
        0x40ff3ddbfbd1b35aULL, 0x7df5f6fffb9efcf2ULL, 0xbfedc7fdb5a9fdd4ULL,
        0xaab0eefcfe9db763ULL, 0xc949db21561875efULL };
    static const uint64_t cnt8[6] = {
        0xaf6f73fbULL | (0x387fb1fdULL << 32),
        0xe6bdfef7ULL | (0xe97f7732ULL << 32),
        0x418c3230ULL | (0x11d87bb0ULL << 32),
        0xe92a3c40ULL | (0x562214ecULL << 32),
        0x131ba506ULL | (0x39386883ULL << 32),
        0x07c828d2ULL | (0x13e94c44ULL << 32) };

    l->esc_scrap_state0_tx_i_resp_p = 0;
    l->esc_scrap_state0_tx_i_resp_n = 1;
    l->esc_scrap_state1_tx_i_resp_p = 0;
    l->esc_scrap_state1_tx_i_resp_n = 1;
    l->u_prim_esc_receiver0_esc_tx_i_esc_p = 0;
    l->u_prim_esc_receiver0_esc_tx_i_esc_n = 1;
    l->u_prim_esc_receiver1_esc_tx_i_esc_p = 0;
    l->u_prim_esc_receiver1_esc_tx_i_esc_n = 1;
    l->lc_clk_byp_ack_i = 0xA;
    l->lc_nvm_rma_ack_i_0_ = 0xA;
    l->lc_nvm_rma_ack_i_1_ = 0xA;
    l->otp_lc_data_i_valid = 1;
    l->otp_lc_data_i_secrets_valid = 0xA;
    l->otp_lc_data_i_test_tokens_valid = 0xA;
    l->otp_lc_data_i_rma_token_valid = 0xA;
    memcpy(l->otp_lc_data_i_state, prod_state, sizeof(prod_state));
    memcpy(l->otp_lc_data_i_count, cnt8, sizeof(cnt8));
    /* pwrmgr init go, then let it decode to completion */
    lc_ctrl_step_many(l, 4);
    l->pwr_lc_i_lc_init = 1;
    lc_ctrl_step(l);
    l->pwr_lc_i_lc_init = 0;
    lc_ctrl_step_many(l, 200);
}

static kmac_state *ot_eg_keymgr_qp_kmac;
static keymgr_state *ot_eg_keymgr_qp_g;
static QEMUTimer *ot_eg_keymgr_qp_timer;

static uint64_t ot_eg_keymgr_dig0[6], ot_eg_keymgr_dig1[6];
static int ot_eg_keymgr_have_dig;

static void ot_eg_keymgr_qp_pump(void *opaque)
{
    keymgr_state *g = ot_eg_keymgr_qp_g;
    kmac_state *k = ot_eg_keymgr_qp_kmac;

    (void)opaque;
    if (g && k &&
        (g->u_reg_op_status_qs == 1u || g->u_kmac_if_state_q != 930u ||
         g->kmac_data_o_valid)) {
        for (unsigned t = 0; t < 4096u; t++) {
            k->app_i_0__valid = g->kmac_data_o_valid;
            k->app_i_0__data = g->kmac_data_o_data;
            k->app_i_0__strb = g->kmac_data_o_strb;
            k->app_i_0__last = g->kmac_data_o_last;
            k->keymgr_key_i_valid = g->kmac_key_o_valid;
            for (unsigned w = 0; w < 4u; w++) {
                k->keymgr_key_i_key_0_[w] = g->kmac_key_o_key_0_[w];
                k->keymgr_key_i_key_1_[w] = g->kmac_key_o_key_1_[w];
            }
            g->kmac_data_i_ready = k->app_o_0__ready;
            g->kmac_data_i_error = k->app_o_0__error;
            /* bridge-owned completion beat: capture the REAL kmac digest
             * at its done pulse; present done only while keymgr sits in
             * StOpWait (553) with the digest held stable */
            if (k->app_o_0__done) {
                for (unsigned w = 0; w < 6u; w++) {
                    ot_eg_keymgr_dig0[w] = k->app_o_0__digest_share0[w];
                    ot_eg_keymgr_dig1[w] = k->app_o_0__digest_share1[w];
                }
                ot_eg_keymgr_have_dig = 1;
            }
            if (g->u_kmac_if_state_q == 553u && ot_eg_keymgr_have_dig) {
                g->kmac_data_i_done = 1;
                for (unsigned w = 0; w < 6u; w++) {
                    g->kmac_data_i_digest_share0[w] = ot_eg_keymgr_dig0[w];
                    g->kmac_data_i_digest_share1[w] = ot_eg_keymgr_dig1[w];
                }
            } else {
                g->kmac_data_i_done = 0;
            }
            keymgr_update(g);
            kmac_update(k);
            keymgr_tick(g);
            kmac_tick(k);
            keymgr_update(g);
            kmac_update(k);
            if (g->u_reg_op_status_qs != 1u && g->u_kmac_if_state_q == 930u &&
                !g->kmac_data_o_valid) {
                ot_eg_keymgr_have_dig = 0;
                break;
            }
        }
    }
    timer_mod(ot_eg_keymgr_qp_timer,
              qemu_clock_get_us(QEMU_CLOCK_VIRTUAL) + 100);
}

static void ot_eg_keymgr_qp_wire(DeviceState *km_dev, DeviceState *kmac_dev,
                                 DeviceState *rc_dev)
{
    keymgr_state *g = ot_keymgr_qp_core(km_dev);

    g->lc_keymgr_en_i = 5;
    g->otp_key_i_creator_root_key_share0_valid = 1;
    g->otp_key_i_creator_root_key_share1_valid = 1;
    g->otp_key_i_creator_seed_valid = 1;
    g->otp_key_i_owner_seed_valid = 1;
    g->rom_digest_i_valid = 1;
    for (unsigned i = 0; i < 4u; i++) {
        g->otp_key_i_creator_root_key_share0[i] = 0x1111111122222201ULL + i * 0x101;
        g->otp_key_i_creator_root_key_share1[i] = 0x2222222244444402ULL + i * 0x101;
        g->otp_key_i_creator_seed[i] = 0x3333333366666603ULL + i * 0x101;
        g->otp_key_i_owner_seed[i] = 0x4444444488888804ULL + i * 0x101;
        g->otp_device_id_i[i] = 0x55555555AAAAAA05ULL + i * 0x101;
        g->flash_i_seeds_0_[i] = 0x9999999911111109ULL + i * 0x101;
        g->flash_i_seeds_1_[i] = 0xAAAAAAAA2222220AULL + i * 0x101;
    }
    /* the FIRST generated-to-generated data value in the machine: the
     * boot digest rom_ctrl computed (via kmac ch 2) seeds keymgr's
     * CreatorRootKey derivation */
    if (rc_dev) {
        rom_ctrl_state *r = ot_rom_ctrl_qp_core(rc_dev);
        for (unsigned i = 0; i < 4u; i++) {
            g->rom_digest_i_data[i] = r->keymgr_data_o_data[i];
        }
        g->rom_digest_i_valid = r->keymgr_data_o_valid;
    }
    ot_eg_keymgr_qp_kmac = ot_kmac_qp_core(kmac_dev);
    ot_eg_keymgr_qp_g = g;
    if (!ot_eg_keymgr_qp_timer) {
        ot_eg_keymgr_qp_timer = timer_new_us(QEMU_CLOCK_VIRTUAL,
                                             ot_eg_keymgr_qp_pump, NULL);
    }
    timer_mod(ot_eg_keymgr_qp_timer,
              qemu_clock_get_us(QEMU_CLOCK_VIRTUAL) + 100);
}

/* Entropy ring: lock-step the generated entropy_src, csrng and edn at
 * signal level (the host-cosim-proven cadence: wire, update x3,
 * tick x3, update x3), then FREEZE every cross-model wire input so no
 * handshake can complete inside a later single-model MMIO settle
 * (the beat would evaporate with the partner not stepping). */
static entropy_src_state *ot_eg_ring_es;
static aes_state *ot_eg_ring_ae;
static unsigned ot_eg_ring_aes_cool;
static csrng_state *ot_eg_ring_cs;
static edn_state *ot_eg_ring_ed;
static QEMUTimer *ot_eg_ring_timer;

static void ot_eg_ring_freeze(void)
{
    csrng_state *cs = ot_eg_ring_cs;
    edn_state *ed = ot_eg_ring_ed;
    entropy_src_state *es = ot_eg_ring_es;

    cs->csrng_cmd_i_0__csrng_req_valid = 0;
    cs->csrng_cmd_i_0__genbits_ready = 0;
    cs->entropy_src_hw_if_i_es_ack = 0;
    ed->csrng_cmd_i_csrng_req_ready = 0;
    ed->csrng_cmd_i_csrng_rsp_ack = 0;
    ed->csrng_cmd_i_genbits_valid = 0;
    es->entropy_src_hw_if_i_es_req = 0;
    es->entropy_src_rng_valid_i = 0;
    ed->edn_i_0__edn_req = 0;
    if (ot_eg_ring_ae) {
        /* split entropy supply: TRIGGER.prng_reseed transactions are
         * served by the REAL ring (pump costeps; the aes-edn gate's
         * assertion) — everything else (the clearing PRNG's demands
         * that arise INSIDE an MMIO settle, invisible to any pump)
         * gets a standing larder word parked on the input, restoring
         * the tie-instant semantics the settle path requires. */
        static int no_larder = -1;
        if (no_larder < 0) {
            no_larder = getenv("OT_AES_NO_LARDER") != NULL;
        }
        if (no_larder ||
            ot_eg_ring_ae->u_aes_core_u_aes_prng_clearing_reseed_req_i) {
            ot_eg_ring_ae->edn_i_edn_ack = 0;
        } else {
            ot_eg_ring_ae->edn_i_edn_ack = 1;
            ot_eg_ring_ae->edn_i_edn_bus = 0xAAAAAAAAu;
            ot_eg_ring_ae->edn_i_edn_fips = 1;
        }
    }
}

static uint32_t ot_eg_noise_lfsr = 0x5EEDBA5Eu;

static void ot_eg_ring_costep(void)
{
    csrng_state *cs = ot_eg_ring_cs;
    edn_state *ed = ot_eg_ring_ed;
    entropy_src_state *es = ot_eg_ring_es;

    cs->csrng_cmd_i_0__csrng_req_valid = ed->csrng_cmd_o_csrng_req_valid;
    cs->csrng_cmd_i_0__csrng_req_bus = ed->csrng_cmd_o_csrng_req_bus;
    cs->csrng_cmd_i_0__genbits_ready = ed->csrng_cmd_o_genbits_ready;
    ed->csrng_cmd_i_csrng_req_ready = cs->csrng_cmd_o_0__csrng_req_ready;
    ed->csrng_cmd_i_csrng_rsp_ack = cs->csrng_cmd_o_0__csrng_rsp_ack;
    ed->csrng_cmd_i_csrng_rsp_sts = cs->csrng_cmd_o_0__csrng_rsp_sts;
    ed->csrng_cmd_i_genbits_valid = cs->csrng_cmd_o_0__genbits_valid;
    ed->csrng_cmd_i_genbits_fips = cs->csrng_cmd_o_0__genbits_fips;
    ed->csrng_cmd_i_genbits_bus = cs->csrng_cmd_o_0__genbits_bus;
    es->entropy_src_hw_if_i_es_req = cs->entropy_src_hw_if_o_es_req;
    cs->entropy_src_hw_if_i_es_ack = es->entropy_src_hw_if_o_es_ack;
    for (unsigned w = 0; w < 6u; w++) {
        cs->entropy_src_hw_if_i_es_bits[w] =
            es->entropy_src_hw_if_o_es_bits[w];
    }
    cs->entropy_src_hw_if_i_es_fips = es->entropy_src_hw_if_o_es_fips;

    /* noise-pump organ: when the generated entropy_src is collecting
     * from its REAL rng port (module wants noise, fw_ov insert not
     * active), feed one deterministic xorshift32 nibble per costep —
     * the same stream the host KAT and the firmware expect header are
     * derived from.  rng_valid is a wire input: it is cleared by
     * ot_eg_ring_freeze before any MMIO settle can run. */
    if (es->entropy_src_rng_enable_o &&
        ((es->u_entropy_src_core_es_enable_fo >> 5) & 1u) &&
        es->u_entropy_src_core_es_delayed_enable &&
        es->u_entropy_src_core_u_prim_fifo_sync_esrng_wready_o &&
        /* whole-pipeline-idle throttle: a nibble is fed only when the
         * previous one has fully drained into the conditioner — the
         * esbit/postht/distr packers DROP data when they stall against
         * a busy SHA3, and a dropped-but-counted nibble desyncs the
         * stream from the python reference */
        !es->u_entropy_src_core_sfifo_esrng_not_empty &&
        !es->u_entropy_src_core_pfifo_esbit_not_empty &&
        !es->u_entropy_src_core_u_prim_packer_fifo_postht_rvalid_o &&
        !es->u_entropy_src_core_sfifo_distr_not_empty &&
        !es->u_entropy_src_core_u_enable_delay_sha3_block_busy_i &&
        !es->u_entropy_src_core_fw_ov_mode_entropy_insert) {
        ot_eg_noise_lfsr ^= ot_eg_noise_lfsr << 13;
        ot_eg_noise_lfsr ^= ot_eg_noise_lfsr >> 17;
        ot_eg_noise_lfsr ^= ot_eg_noise_lfsr << 5;
        es->entropy_src_rng_valid_i = 1;
        es->entropy_src_rng_bits_i = ot_eg_noise_lfsr & 0xFu;
    } else {
        es->entropy_src_rng_valid_i = 0;
    }

    /* aes leg (endpoint 0): the generated aes rides the ring for REAL
     * EDN entropy — its shim tie (always-ack) is overridden at wire
     * time.  The leg only steps while aes wants entropy, an ack is in
     * flight, or within a short cooldown (the multi-word request
     * sequence needs a few ticks between words); an idle aes is never
     * touched, so MMIO-driven aes targets are unaffected. */
    {
        aes_state *ae = ot_eg_ring_ae;
        int aes_leg = 0;
        if (ae) {
                    ed->edn_i_0__edn_req = ae->edn_o_edn_req;
            ae->edn_i_edn_ack = ed->edn_o_0__edn_ack;
            ae->edn_i_edn_bus = ed->edn_o_0__edn_bus;
            ae->edn_i_edn_fips = ed->edn_o_0__edn_fips;
            /* the leg runs for the WHOLE prng-reseed transaction
             * (reseed_req_i spans the multi-word burst including the
             * inter-word gaps where edn_req is momentarily low) plus a
             * short tail so the transaction retires; an aes running
             * ordinary cipher ops is never ticked. */
            {
                static int prev_rr;
                int rr = ae->u_aes_core_u_aes_prng_clearing_reseed_req_i;
                if (prev_rr && !rr) {
                    ot_eg_ring_aes_cool = 16u;   /* transaction tail */
                }
                prev_rr = rr;
                if (rr || ed->edn_o_0__edn_ack) {
                    ot_eg_ring_aes_cool =
                        ot_eg_ring_aes_cool > 1u ? ot_eg_ring_aes_cool : 1u;
                }
            }
            aes_leg = ot_eg_ring_aes_cool > 0u;
            if (ot_eg_ring_aes_cool) {
                ot_eg_ring_aes_cool--;
            }
        }
        edn_update(ed); csrng_update(cs); entropy_src_update(es);
        if (aes_leg) aes_update(ae);
        edn_tick(ed); csrng_tick(cs); entropy_src_tick(es);
        if (aes_leg) aes_tick(ae);
        edn_update(ed); csrng_update(cs); entropy_src_update(es);
        if (aes_leg) aes_update(ae);
    }
}

static void ot_eg_ring_pump(void *opaque)
{
    csrng_state *cs = ot_eg_ring_cs;
    edn_state *ed = ot_eg_ring_ed;
    entropy_src_state *es = ot_eg_ring_ed ? ot_eg_ring_es : NULL;
    bool quiesced = false;
    (void)opaque;
    cs->_qp_pump = 1;
    ed->_qp_pump = 1;
    es->_qp_pump = 1;

    for (unsigned t = 0; t < 4096u; t++) {
        ot_eg_ring_costep();

        /* quiescent: nothing in flight anywhere on the ring, and the
         * noise organ is not mid-collection toward an unread SW seed
         * (es wants rng noise, insert off, es_entropy_valid intr not
         * yet pending -> keep the fast cadence until the seed lands) */
        if (!ed->csrng_cmd_o_csrng_req_valid &&
            !cs->csrng_cmd_o_0__genbits_valid &&
            !cs->entropy_src_hw_if_o_es_req &&
            !(es->entropy_src_rng_enable_o &&
              !es->u_entropy_src_core_fw_ov_mode_entropy_insert &&
              !es->u_reg_u_intr_state_es_entropy_valid_q) &&
            ot_eg_ring_aes_cool == 0u &&
            cs->u_csrng_core_u_csrng_main_sm_state_q == 0x37u /* Idle */) {
            quiesced = true;
            break;
        }
    }
    ot_eg_ring_freeze();
    /* adaptive cadence: tight while beats are in flight (a wire beat
     * costs one fire), relaxed when the ring is quiet — csrng's
     * update_state is enormous and an always-hot 1us pump starves the
     * vCPU. */
    timer_mod(ot_eg_ring_timer,
              qemu_clock_get_us(QEMU_CLOCK_VIRTUAL) + (quiesced ? 200 : 2));
}

static void ot_eg_entropy_ring_wire(DeviceState *es_dev, DeviceState *cs_dev,
                                    DeviceState *ed_dev, DeviceState *ae_dev)
{
    entropy_src_state *es = ot_entropy_src_qp_core(es_dev);
    csrng_state *cs = ot_csrng_qp_core(cs_dev);
    edn_state *ed = ot_edn_qp_core(ed_dev);
    aes_state *ae = ot_aes_qp_core(ae_dev);

    /* replace the aes shim's always-ack EDN binding tie with the REAL
     * endpoint-0 wiring: from here on, aes entropy comes from the ring
     * (boot genbits serve its parked initial PRNG request) */
    ae->edn_i_edn_ack = 0;
    ae->edn_i_edn_bus = 0;
    ae->edn_i_edn_fips = 0;
    ot_eg_ring_ae = getenv("OT_NO_AES_LEG") ? NULL : ae;

    /* the host-harness tie set: alert receivers idle, OTP mubi8 gates
     * True, lc debug Off, rng raw inputs silent, pumps held high so
     * ACCUMULATE counters (v_ctr, sha3 pad counts) run per co-step */
    cs->alert_rx_i_0__ack_p = 0; cs->alert_rx_i_0__ack_n = 1;
    cs->alert_rx_i_0__ping_p = 0; cs->alert_rx_i_0__ping_n = 1;
    cs->alert_rx_i_1__ack_p = 0; cs->alert_rx_i_1__ack_n = 1;
    cs->alert_rx_i_1__ping_p = 0; cs->alert_rx_i_1__ping_n = 1;
    cs->lc_hw_debug_en_i = 0xA;
    cs->otp_en_csrng_sw_app_read_i = 0x96;
    cs->_qp_pump = 1;

    ed->alert_rx_i_0__ack_p = 0; ed->alert_rx_i_0__ack_n = 1;
    ed->alert_rx_i_0__ping_p = 0; ed->alert_rx_i_0__ping_n = 1;
    ed->alert_rx_i_1__ack_p = 0; ed->alert_rx_i_1__ack_n = 1;
    ed->alert_rx_i_1__ping_p = 0; ed->alert_rx_i_1__ping_n = 1;
    ed->_qp_pump = 1;

    es->alert_rx_i_0__ack_p = 0; es->alert_rx_i_0__ack_n = 1;
    es->alert_rx_i_0__ping_p = 0; es->alert_rx_i_0__ping_n = 1;
    es->alert_rx_i_1__ack_p = 0; es->alert_rx_i_1__ack_n = 1;
    es->alert_rx_i_1__ping_p = 0; es->alert_rx_i_1__ping_n = 1;
    es->otp_en_entropy_src_fw_over_i = 0x96;
    es->otp_en_entropy_src_fw_read_i = 0x96;
    es->entropy_src_rng_valid_i = 0;
    es->entropy_src_rng_bits_i = 0;
    es->_qp_pump = 1;

    ot_eg_ring_es = es;
    ot_eg_ring_cs = cs;
    ot_eg_ring_ed = ed;

    /* THE BOOT RING runs right here, uninterrupted (the rom_ctrl boot
     * bridge precedent): fw_ov window 1 -> SHA3-384 seed, then EDN
     * boot mode drives INS (REAL entropy over the es wire) + GEN
     * through the generated csrng to BootDone.  No firmware MMIO can
     * interleave, so no freeze semantics apply — this is exactly the
     * host-proven ring harness cadence. */
    ot_eg_ring_freeze();
    entropy_src_write(es, 0x20u, 0x00699996u, 4);   /* CONF */
    ot_eg_ring_freeze();
    entropy_src_write(es, 0x94u, 0x66u, 4);         /* FW_OV_CONTROL */
    ot_eg_ring_freeze();
    entropy_src_write(es, 0x1cu, 0x6u, 4);          /* MODULE_ENABLE */
    ot_eg_ring_freeze();
    entropy_src_write(es, 0x98u, 0x6u, 4);          /* SHA3 window open */
    for (unsigned i = 0; i < 64u; i++) {
        unsigned guard = 0;
        for (;;) {
            ot_eg_ring_freeze();
            if (!(entropy_src_read(es, 0x9cu, 4) & 1u) || guard++ >= 1000u) {
                break;
            }
            ot_eg_ring_costep();
        }
        ot_eg_ring_freeze();
        entropy_src_write(es, 0xa8u, 0x5EED0000u + 0x01010101u * i, 4);
    }
    ot_eg_ring_freeze();
    entropy_src_write(es, 0x98u, 0x9u, 4);          /* close: pad+squeeze */
    ot_eg_ring_freeze();
    csrng_write(cs, 0x14u, 0x9666u, 4);             /* csrng CTRL */
    ot_eg_ring_freeze();
    edn_write(ed, 0x18u, 0x00000901u, 4);           /* BOOT_INS: real entropy */
    ot_eg_ring_freeze();
    edn_write(ed, 0x1cu, 0x00001903u, 4);           /* BOOT_GEN glen=1 */
    ot_eg_ring_freeze();
    edn_write(ed, 0x14u, 0x9966u, 4);               /* enable + boot mode */
    for (unsigned t = 0; t < 40000u; t++) {
        ot_eg_ring_costep();
        /* BootDone poll on the INTERNAL state: an edn MMIO settle here
         * would replay live-wire beats and evaporate the boot genbits */
        if (ed->u_edn_core_u_edn_main_sm_state_q == 0xF0u) {
            break;
        }
    }
    /* tail flush: the boot handshake's last beats (csrng ack path,
     * fifo drains) complete a few ticks after BootDone shows */
    for (unsigned t = 0; t < 200u; t++) {
        ot_eg_ring_costep();
    }
    /* lost-arm workaround: RTL re-initializes the ctr_drbg gen_subcmd
     * register at every GEN accept (csrng_ctr_drbg.sv Idle arm); the
     * generated model dropped that arm, so boot residue (UPD_FINAL)
     * silently kills the next GEN.  Reset to the value every proven
     * KAT ran with (emitter fix pending). */
    cs->u_csrng_core_u_csrng_ctr_drbg_gen_subcmd_q = 0;
    cs->u_csrng_core_u_csrng_ctr_drbg_gen_subcmd_d = 0;
    if (ed->u_edn_core_u_edn_main_sm_state_q != 0xF0u) {
        warn_report("entropy ring boot bridge: BootDone not reached (sm=%x)",
                    (unsigned)ed->u_edn_core_u_edn_main_sm_state_q);
    }

    ot_eg_ring_freeze();
    if (!ot_eg_ring_timer) {
        ot_eg_ring_timer = timer_new_us(QEMU_CLOCK_VIRTUAL,
                                        ot_eg_ring_pump, NULL);
    }
    timer_mod(ot_eg_ring_timer,
              qemu_clock_get_us(QEMU_CLOCK_VIRTUAL) + 1);
}

static void ot_eg_romctrl_qp_boot_bridge(DeviceState *rc_dev,
                                         DeviceState *kmac_dev)
{
    rom_ctrl_state *r = ot_rom_ctrl_qp_core(rc_dev);
    kmac_state *k = ot_kmac_qp_core(kmac_dev);

    /* ACCUMULATE counters (FSM address counter, msgfifo pointers, sha3pad
     * sent count) only advance on pump ticks; every bridge co-step IS a
     * real clock, so hold the pump high for the duration. */
    uint8_t rp = r->_qp_pump, kp = k->_qp_pump;
    r->_qp_pump = 1;
    k->_qp_pump = 1;

    for (unsigned t = 0; t < 200000u && r->pwrmgr_data_o_done != 0x6u; t++) {
        r->kmac_data_i_ready = k->app_o_2__ready;
        r->kmac_data_i_done = k->app_o_2__done;
        r->kmac_data_i_error = k->app_o_2__error;
        for (unsigned w = 0; w < 6u; w++) {
            r->kmac_data_i_digest_share0[w] = k->app_o_2__digest_share0[w];
            r->kmac_data_i_digest_share1[w] = k->app_o_2__digest_share1[w];
        }
        k->app_i_2__valid = r->kmac_data_o_valid;
        k->app_i_2__data = r->kmac_data_o_data;
        k->app_i_2__strb = r->kmac_data_o_strb;
        k->app_i_2__last = r->kmac_data_o_last;
        rom_ctrl_step(r);
        kmac_step(k);
    }
    r->_qp_pump = rp;
    k->_qp_pump = kp;
    if (r->pwrmgr_data_o_good != 0x6u) {
        warn_report("rom_ctrl_qp boot bridge: done=%x good=%x",
                    r->pwrmgr_data_o_done, r->pwrmgr_data_o_good);
    }
}

static void ot_eg_soc_reset_exit(Object *obj, ResetType type)
{
    OtEGSoCClass *c = RISCV_OT_EG_SOC_GET_CLASS(obj);
    OtEGSoCState *s = RISCV_OT_EG_SOC(obj);

    if (c->parent_phases.exit) {
        c->parent_phases.exit(obj, type);
    }

    if (s->devices[OT_EG_SOC_DEV_ROM_CTRL_QP] &&
        s->devices[OT_EG_SOC_DEV_KMAC]) {
        ot_eg_romctrl_qp_boot_bridge(s->devices[OT_EG_SOC_DEV_ROM_CTRL_QP],
                                     s->devices[OT_EG_SOC_DEV_KMAC]);
    }

    /* HEART SWAP: the generated rom_ctrl checks the boot image and its
     * verdict raises the pwrmgr done/good lines — this call replaces
     * the native rom_ctrl "load" trigger. */
    if (s->devices[OT_EG_SOC_DEV_ROM_CTRL_QP_BOOT]) {
        ot_rom_ctrl_qp_boot_run(s->devices[OT_EG_SOC_DEV_ROM_CTRL_QP_BOOT]);
    }

    if (s->devices[OT_EG_SOC_DEV_KEYMGR_QP] &&
        s->devices[OT_EG_SOC_DEV_KMAC]) {
        /* keymgr eats the digest of the ACTUAL boot image (the heart-
         * swap primary), not the parallel canary's KAT image */
        ot_eg_keymgr_qp_wire(s->devices[OT_EG_SOC_DEV_KEYMGR_QP],
                             s->devices[OT_EG_SOC_DEV_KMAC],
                             s->devices[OT_EG_SOC_DEV_ROM_CTRL_QPP]);
    }

    if (s->devices[OT_EG_SOC_DEV_LC_CTRL_QP]) {
        ot_eg_lc_ctrl_qp_wire(s->devices[OT_EG_SOC_DEV_LC_CTRL_QP]);
    }

    if (s->devices[OT_EG_SOC_DEV_ENTROPY_SRC_QP] &&
        s->devices[OT_EG_SOC_DEV_CSRNG_QP] &&
        s->devices[OT_EG_SOC_DEV_EDN_QP]) {
        ot_eg_entropy_ring_wire(s->devices[OT_EG_SOC_DEV_ENTROPY_SRC_QP],
                                s->devices[OT_EG_SOC_DEV_CSRNG_QP],
                                s->devices[OT_EG_SOC_DEV_EDN_QP],
                                s->devices[OT_EG_SOC_DEV_AES]);
    }
}

static void ot_eg_soc_realize(DeviceState *dev, Error **errp)
{
    OtEGSoCState *s = RISCV_OT_EG_SOC(dev);
    (void)errp;

    CPUState *cpu = CPU(s->devices[OT_EG_SOC_DEV_HART]);
    cpu->memory = get_system_memory();
    cpu->cpu_index = 0;

    /* Create the private bus on which all OpenTitan IPs are connected */
    s->ot_bus = BUS(object_new(TYPE_SYSTEM_BUS));
    qbus_init(s->ot_bus, sizeof(*s->ot_bus), TYPE_SYSTEM_BUS, DEVICE(s),
              "ot.bus");

    /* Link, define properties and realize devices, then connect GPIOs */
    ot_common_configure_devices_with_id(s->devices, s->ot_bus, "soc", false,
                                        ot_eg_soc_devices,
                                        ARRAY_SIZE(ot_eg_soc_devices));

    MemoryRegion *lc_ctrl_tap_mr = g_new0(MemoryRegion, 1u);
    memory_region_init(lc_ctrl_tap_mr, OBJECT(dev), OT_EG_LC_CTRL_TAP_XBAR,
                       OT_EG_LC_CTRL_TAP_TL_SIZE);

    MemoryRegion *mrs[IBEX_MEMMAP_REGIDX_COUNT] = {
        [OT_EG_DEFAULT_MEMORY_REGION] = get_system_memory(),
        [OT_EG_LC_CTRL_TAP_MEMORY_REGION] = lc_ctrl_tap_mr,
    };
    ibex_map_devices_mask(s->devices, mrs, ot_eg_soc_devices,
                          ARRAY_SIZE(ot_eg_soc_devices),
                          IBEX_MEMMAP_MAKE_REG_MASK(
                              OT_EG_DEFAULT_MEMORY_REGION) |
                              IBEX_MEMMAP_MAKE_REG_MASK(
                                  OT_EG_LC_CTRL_TAP_MEMORY_REGION));
    Object *oas;
    AddressSpace *as = g_new0(AddressSpace, 1u);
    address_space_init(as, lc_ctrl_tap_mr, OT_EG_LC_CTRL_TAP_AS);
    oas = object_new(TYPE_OT_ADDRESS_SPACE);
    object_property_add_child(OBJECT(dev), as->name, oas);
    ot_address_space_set(OT_ADDRESS_SPACE(oas), as);

    qdev_connect_gpio_out_named(DEVICE(s->devices[OT_EG_SOC_DEV_RSTMGR]),
                                OT_RSTMGR_SOC_RST, 0,
                                qdev_get_gpio_in_named(DEVICE(s),
                                                       OT_EG_SOC_RST_REQ, 0));

    /* TEST SCAFFOLDING (opt-in via OT_GPIO_LOOPBACK env var, off by default so
     * other GPIO tests are unaffected): loop each GPIO data-output pin back to
     * the matching data-input pin.  Lets firmware verify the generic pin-I/O
     * path (write DIRECT_OUT -> read DATA_IN, real-edge interrupts) without any
     * external pin driver.  Not part of normal SoC behavior. */
    if (getenv("OT_GPIO_LOOPBACK")) {
        DeviceState *gpio = DEVICE(s->devices[OT_EG_SOC_DEV_GPIO]);
        for (int i = 0; i < 32; i++) {
            qdev_connect_gpio_out(gpio, i, qdev_get_gpio_in(gpio, i));
        }
    }

    /* TEST SCAFFOLDING (opt-in via OT_PIN_GROUP env var): the pin-group
     * closed loop.  Wires the generated pwm/pattgen data outputs into the
     * generated pinmux's PERIPHERAL side, loops every MIO pad output back
     * to the matching pad input, and feeds pinmux's peripheral outputs into
     * the generated gpio's pin inputs.  Firmware then routes signals with
     * ordinary pinmux OUTSEL/INSEL programming and observes them in GPIO
     * DATA_IN — pwm -> pinmux -> pad -> pinmux -> gpio, five generated
     * models in one ring, no CPU pin driver. */
    if (getenv("OT_PIN_GROUP")) {
        DeviceState *pinmux = DEVICE(s->devices[OT_EG_SOC_DEV_PINMUX]);
        DeviceState *gpio = DEVICE(s->devices[OT_EG_SOC_DEV_GPIO]);
        DeviceState *pwm = DEVICE(s->devices[OT_EG_SOC_DEV_PWM]);
        DeviceState *pattgen = DEVICE(s->devices[OT_EG_SOC_DEV_PATTGEN]);
        /* Peripheral-output indices (top_earlgrey.h OUTSEL constants - 3):
         * PWM0..5 = 62..67, Pattgen pda0 = 46. */
        for (int i = 0; i < 6; i++) {
            qdev_connect_gpio_out(pwm, i,
                qdev_get_gpio_in_named(pinmux, "periph-in", 62 + i));
        }
        qdev_connect_gpio_out(pattgen, 0,
            qdev_get_gpio_in_named(pinmux, "periph-in", 46));
        /* MIO pad loopback: pad output k -> pad input k (47 pads). */
        for (int i = 0; i < 47; i++) {
            qdev_connect_gpio_out(pinmux, i, qdev_get_gpio_in(pinmux, i));
        }
        /* Peripheral inputs: mio_to_periph 0..31 are GPIO0..31. */
        for (int i = 0; i < 32; i++) {
            qdev_connect_gpio_out_named(pinmux, "periph-out", i,
                                        qdev_get_gpio_in(gpio, i));
        }
    }

    /* TEST SCAFFOLDING (opt-in via OT_SPI_LOOP env var): the SPI self-loop.
     * Hangs an SSI-to-pin bridge on the generated spi_host0's "spi0" bus
     * (the m25p80 flash is only created when a -device flash is given on
     * the command line, so the loop test owns the bus) and wires the
     * bridge's raw SPI pins to the generated spi_device's aux pin lines:
     * two generated models talking SPI to each other, no external client. */
    if (getenv("OT_SPI_LOOP")) {
        DeviceState *spihost = DEVICE(s->devices[OT_EG_SOC_DEV_SPI_HOST0]);
        DeviceState *spidev = DEVICE(s->devices[OT_EG_SOC_DEV_SPI_DEVICE]);
        BusState *spibus = qdev_get_child_bus(spihost, "spi0");
        if (spibus) {
            DeviceState *br = qdev_new("ot-ssi-pin-bridge");
            qdev_realize_and_unref(br, spibus, &error_fatal);
            qdev_connect_gpio_out_named(spihost, SSI_GPIO_CS, 0,
                qdev_get_gpio_in_named(br, SSI_GPIO_CS, 0));
            qdev_connect_gpio_out_named(br, "sck", 0,
                qdev_get_gpio_in_named(spidev, "sck-in", 0));
            qdev_connect_gpio_out_named(br, "csb", 0,
                qdev_get_gpio_in_named(spidev, "csb-in", 0));
            qdev_connect_gpio_out_named(br, "mosi", 0,
                qdev_get_gpio_in_named(spidev, "sd-in", 0));
            qdev_connect_gpio_out_named(spidev, "sd-out", 1,
                qdev_get_gpio_in_named(br, "miso", 0));
        }
    }

    ot_common_check_rom_configuration();

    /* load kernel if provided */
    ibex_load_kernel(cpu);
}

static void ot_eg_soc_init(Object *obj)
{
    OtEGSoCState *s = RISCV_OT_EG_SOC(obj);

    s->devices = ibex_create_devices(ot_eg_soc_devices,
                                     ARRAY_SIZE(ot_eg_soc_devices), DEVICE(s));

    qdev_init_gpio_in_named(DEVICE(obj), &ot_eg_soc_hw_reset, OT_EG_SOC_RST_REQ,
                            1);
}

static void ot_eg_soc_class_init(ObjectClass *oc, const void *data)
{
    OtEGSoCClass *sc = RISCV_OT_EG_SOC_CLASS(oc);
    DeviceClass *dc = DEVICE_CLASS(oc);
    ResettableClass *rc = RESETTABLE_CLASS(dc);
    (void)data;

    resettable_class_set_parent_phases(rc, NULL, &ot_eg_soc_reset_hold,
                                       &ot_eg_soc_reset_exit,
                                       &sc->parent_phases);
    rc->child_foreach = &ot_eg_soc_reset_child_foreach;
    dc->realize = &ot_eg_soc_realize;
    dc->user_creatable = false;
}

static const TypeInfo ot_eg_soc_type_info = {
    .name = TYPE_RISCV_OT_EG_SOC,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(OtEGSoCState),
    .instance_init = &ot_eg_soc_init,
    .class_init = &ot_eg_soc_class_init,
    .class_size = sizeof(OtEGSoCClass),
};

static void ot_eg_soc_register_types(void)
{
    type_register_static(&ot_eg_soc_type_info);
}

type_init(ot_eg_soc_register_types);

/* ------------------------------------------------------------------------ */
/* Board */
/* ------------------------------------------------------------------------ */

static void ot_eg_board_set_spiflash0(Object *obj, const char *value,
                                      Error **errp)
{
    OtEGBoardState *board = RISCV_OT_EG_BOARD(obj);
    (void)errp;

    g_free(board->spiflash[OT_EG_MTD_SPI0]);
    board->spiflash[OT_EG_MTD_SPI0] = g_strdup(value);
}

static void ot_eg_board_set_spiflash1(Object *obj, const char *value,
                                      Error **errp)
{
    OtEGBoardState *board = RISCV_OT_EG_BOARD(obj);
    (void)errp;

    g_free(board->spiflash[OT_EG_MTD_SPI1]);
    board->spiflash[OT_EG_MTD_SPI1] = g_strdup(value);
}

static void ot_eg_board_child_foreach(Object *obj, ResettableChildCallback cb,
                                      void *opaque, ResetType type)
{
    OtEGBoardState *s = RISCV_OT_EG_BOARD(obj);

    for (unsigned ix = 0; ix < OT_EG_BOARD_DEV_COUNT; ix++) {
        /* flash devices are optional */
        if (s->devices[ix]) {
            cb(OBJECT(s->devices[ix]), opaque, type);
        }
    }
}

static void ot_eg_board_realize(DeviceState *dev, Error **errp)
{
    OtEGBoardState *board = RISCV_OT_EG_BOARD(dev);

    DeviceState *soc = board->devices[OT_EG_BOARD_DEV_SOC];
    object_property_add_child(OBJECT(board), "soc", OBJECT(soc));

    BusState *bus = sysbus_get_default();
    qdev_realize_and_unref(soc, bus, &error_fatal);

    for (unsigned fix = 0; fix < OT_EG_MTD_SPI_COUNT; fix++) {
        const char *flash_type = board->spiflash[OT_EG_MTD_SPI0 + fix];
        /*
         * skip this flash slot if no device type has been defined on the QEMU
         * command line
         */
        if (!flash_type) {
            continue;
        }

        /* qdev_new aborts if the specified device is not supported */
        DeviceState *flash = qdev_new(flash_type);

        if (!object_dynamic_cast(OBJECT(flash), TYPE_M25P80)) {
            error_setg(errp, "%s is not a SPI dataflash device", flash_type);
        }

        /*
         * retrieve the SPI host controller bus. Although each SPI host only
         * has one SPI bus, each bus name in QEMU needs to be unique. The SPI
         * host controller uses its bus-num property as a suffix for naming its
         * bus
         */
        DeviceState *spihost =
            RISCV_OT_EG_SOC(soc)->devices[OT_EG_SOC_DEV_SPI_HOST0 + fix];
        char *busname = g_strdup_printf("spi%u", fix);
        BusState *spibus = qdev_get_child_bus(spihost, busname);
        g_assert(spibus);

        /*
         * if a "drive" property for this bus/unit pair is defined on the QEMU
         * command line, assigned it to the flash device
         */
        DriveInfo *dinfo = drive_get(IF_MTD, (int)fix, 0);
        if (dinfo) {
            qdev_prop_set_drive_err(DEVICE(flash), "drive",
                                    blk_by_legacy_dinfo(dinfo), &error_fatal);
        }

        /* the flash device is a child of the board */
        char *flashname = g_strdup_printf("dataflash%u", fix);
        object_property_add_child(OBJECT(board), flashname, OBJECT(flash));
        /* connect it as a peripheral of the SPI host controller bus */
        ssi_realize_and_unref(flash, SSI_BUS(spibus), errp);

        board->devices[OT_EG_BOARD_DEV_FLASH0 + fix] = flash;

        /*
         * finally, connect the first CS line of the SPI controller to control
         * to select this SPI flash device
         */
        qemu_irq cs = qdev_get_gpio_in_named(flash, SSI_GPIO_CS, 0);
        qdev_connect_gpio_out_named(spihost, SSI_GPIO_CS, 0, cs);

        g_free(flashname);
        g_free(busname);
    }
}

static void ot_eg_board_init(Object *obj)
{
    object_property_add_str(obj, "spiflash0", NULL, &ot_eg_board_set_spiflash0);
    object_property_set_description(obj, "spiflash0",
                                    "SPI dataflash on SPI0 bus");
    object_property_add_str(obj, "spiflash1", NULL, &ot_eg_board_set_spiflash1);
    object_property_set_description(obj, "spiflash1",
                                    "SPI dataflash on SPI1 bus");

    OtEGBoardState *s = RISCV_OT_EG_BOARD(obj);

    s->devices = g_new0(DeviceState *, OT_EG_BOARD_DEV_COUNT);
    s->devices[OT_EG_BOARD_DEV_SOC] = qdev_new(TYPE_RISCV_OT_EG_SOC);
}

static void ot_eg_board_class_init(ObjectClass *oc, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    (void)data;

    dc->realize = &ot_eg_board_realize;

    ResettableClass *rc = RESETTABLE_CLASS(oc);
    rc->child_foreach = &ot_eg_board_child_foreach;
}

static const TypeInfo ot_eg_board_type_info = {
    .name = TYPE_RISCV_OT_EG_BOARD,
    .parent = TYPE_DEVICE,
    .instance_size = sizeof(OtEGBoardState),
    .instance_init = &ot_eg_board_init,
    .class_init = &ot_eg_board_class_init,
};

static void ot_eg_board_register_types(void)
{
    type_register_static(&ot_eg_board_type_info);
}

type_init(ot_eg_board_register_types);

/* ------------------------------------------------------------------------ */
/* Machine */
/* ------------------------------------------------------------------------ */

static bool ot_eg_machine_get_no_epmp_cfg(Object *obj, Error **errp)
{
    OtEGMachineState *s = RISCV_OT_EG_MACHINE(obj);
    (void)errp;

    return s->no_epmp_cfg;
}

static void ot_eg_machine_set_no_epmp_cfg(Object *obj, bool value, Error **errp)
{
    OtEGMachineState *s = RISCV_OT_EG_MACHINE(obj);
    (void)errp;

    s->no_epmp_cfg = value;
}

static bool ot_eg_machine_get_ignore_elf_entry(Object *obj, Error **errp)
{
    OtEGMachineState *s = RISCV_OT_EG_MACHINE(obj);
    (void)errp;

    return s->ignore_elf_entry;
}

static void
ot_eg_machine_set_ignore_elf_entry(Object *obj, bool value, Error **errp)
{
    OtEGMachineState *s = RISCV_OT_EG_MACHINE(obj);
    (void)errp;

    s->ignore_elf_entry = value;
}

static bool ot_eg_machine_get_verilator(Object *obj, Error **errp)
{
    OtEGMachineState *s = RISCV_OT_EG_MACHINE(obj);
    (void)errp;

    return s->verilator;
}

static void ot_eg_machine_set_verilator(Object *obj, bool value, Error **errp)
{
    OtEGMachineState *s = RISCV_OT_EG_MACHINE(obj);
    (void)errp;

    s->verilator = value;
}

static ResettableState *ot_eg_machine_get_reset_state(Object *obj)
{
    OtEGMachineState *s = RISCV_OT_EG_MACHINE(obj);

    return &s->reset;
}

static void ot_eg_machine_child_foreach(Object *obj, ResettableChildCallback cb,
                                        void *opaque, ResetType type)
{
    Object *board = object_property_get_link(obj, "board", &error_fatal);

    cb(board, opaque, type);
}

static void ot_eg_machine_reset(MachineState *ms, ResetType reason)
{
    OtEGMachineState *s = RISCV_OT_EG_MACHINE(ms);

    g_assert(reason == RESET_TYPE_COLD);

    resettable_reset(OBJECT(s), reason);
}

static void ot_eg_machine_instance_init(Object *obj)
{
    OtEGMachineState *s = RISCV_OT_EG_MACHINE(obj);

    s->no_epmp_cfg = false;
    object_property_add_bool(obj, "no-epmp-cfg", &ot_eg_machine_get_no_epmp_cfg,
                             &ot_eg_machine_set_no_epmp_cfg);
    object_property_set_description(obj, "no-epmp-cfg",
                                    "Skip default ePMP configuration");
    object_property_add_bool(obj, "ignore-elf-entry",
                             &ot_eg_machine_get_ignore_elf_entry,
                             &ot_eg_machine_set_ignore_elf_entry);
    object_property_set_description(obj, "ignore-elf-entry",
                                    "Do not set vCPU PC with ELF entry point");
    object_property_add_bool(obj, "verilator", &ot_eg_machine_get_verilator,
                             &ot_eg_machine_set_verilator);
    object_property_set_description(obj, "verilator", "Use Verilator clocks");
}

static void ot_eg_machine_init(MachineState *state)
{
    DeviceState *dev = qdev_new(TYPE_RISCV_OT_EG_BOARD);

    object_property_add_child(OBJECT(state), "board", OBJECT(dev));

    qdev_realize(dev, NULL, &error_fatal);
}

static void ot_eg_machine_class_init(ObjectClass *oc, const void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);
    (void)data;

    mc->desc = "RISC-V Board compatible with OpenTitan EarlGrey FPGA platform";
    mc->init = ot_eg_machine_init;
    mc->reset = &ot_eg_machine_reset;
    mc->max_cpus = 1u;
    mc->default_cpus = 1u;

    /*
     * Implement the resettable interface to ensure the proper initialization
     * sequence.
     */
    ResettableClass *rc = RESETTABLE_CLASS(oc);

    rc->get_state = &ot_eg_machine_get_reset_state;
    rc->child_foreach = &ot_eg_machine_child_foreach;
}

static const TypeInfo ot_eg_machine_type_info = {
    .name = TYPE_RISCV_OT_EG_MACHINE,
    .parent = TYPE_MACHINE,
    .instance_size = sizeof(OtEGMachineState),
    .instance_init = &ot_eg_machine_instance_init,
    .class_init = &ot_eg_machine_class_init,
    .interfaces = (InterfaceInfo[]){ { TYPE_RESETTABLE_INTERFACE }, {} },
};

static void ot_eg_machine_register_types(void)
{
    type_register_static(&ot_eg_machine_type_info);
}

type_init(ot_eg_machine_register_types);
