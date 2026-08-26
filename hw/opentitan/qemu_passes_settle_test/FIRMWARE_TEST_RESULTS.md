# Firmware Test Results

## Test Environment

- Generated models: `generated_settle_cycle_test/`
- QEMU binary: `/home/user/workspace/ot-qemu/build/qemu-system-riscv32`
- Machine: `ot-earlgrey`
- Firmware directory: `firmware_test/`
- QP-routed instances:
  - UART0
  - GPIO
  - I2C0
  - SPI device
  - SPI host 0
  - Pattgen

The six generated core models and shims were copied into the QEMU tree. The
Earl Grey device table was generated from the six-device subset of
`soc/earlgrey.soc.json` and `soc/earlgrey.glue.json`, then QEMU was rebuilt.

## Passing Tests

### CSR And Sequential Behavior

| Test | Result | Coverage |
|---|---|---|
| `run-uart-full` | PASS | Four RW registers and multi-stage readback |
| `run-gpio-full` | PASS | RW registers and INTR_STATE W1C |
| `run-i2c-full` | PASS | Six RW registers and event W1C |
| `run-spi-device-full` | PASS | IRQ register storage and W1C |
| `run-spi-host-full` | PASS | Six RW registers and W1C |
| `run-pattgen-full` | PASS | Pattgen register storage |

### External And Interrupt Behavior

| Test | Result | Coverage |
|---|---|---|
| `run-uart-chardev-loopback` | PASS | Firmware TX bytes reach a QEMU chardev |
| `run-uart-chardev-rx-echo` | PASS | Chardev RX reaches firmware and echoes to TX |
| `run-uart-dif-echo` | PASS | Unmodified OpenTitan `dif_uart.c` interrupt echo |
| `run-uart-irq` | PASS | Generated UART IRQ to PLIC and CPU |
| `run-gpio-loopback` | PASS | Output pin to input pin round trip |
| `run-gpio-pin-irq` | PASS | Real GPIO rising edge to PLIC and CPU |
| `run-i2c-irq` | PASS | Generated I2C interrupt delivery |
| `run-spi-device-irq` | PASS | Generated SPI-device interrupt delivery |
| `run-spi-host-irq` | PASS | Generated SPI-host interrupt delivery |
| `run-pattgen-irq` | PASS | Generated Pattgen interrupt delivery |

The GPIO tests require test-only Earl Grey wiring enabled by
`OT_GPIO_LOOPBACK=1`. The board connects each generated GPIO output line to the
matching input line only while that environment variable is set.

## Known Boundaries

| Test | Result | Reason |
|---|---|---|
| `run-uart-smoketest` | INCONCLUSIVE | Firmware remains in `poll_status()` waiting for `STATUS.TXIDLE`; the internal cycle-level serializer is not advanced to completion by the current functional timing model. |
| `run-i2c-backend` | FAIL `0x320` | An FDATA write does not change FMT FIFO level/empty state. The SRAM-backed FIFO pointer increment is absent from the active tick/backend path, so the host FSM cannot start a real I2C transaction. |

## Conclusion

The generated models currently simulate these hardware-visible behaviors in
real QEMU:

- MMIO decode and register storage;
- dependency-ordered combinational propagation;
- simultaneous sequential register updates;
- W1C interrupt state;
- UART character TX/RX through a QEMU chardev;
- GPIO pin input/output and real edge detection;
- IRQ delivery through the PLIC to firmware.

They do not yet provide universally cycle-accurate behavior for autonomous,
multi-cycle serial engines and SRAM-backed protocol FIFOs. Those paths need
dedicated backend passes and QEMU scheduling/API integration rather than more
MMIO settle iterations.
