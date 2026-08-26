# Settle Cycle Detection Test Outputs

This directory contains generated QEMU C models used to validate dependency-
ordered combinational evaluation, two-phase sequential commits, runtime
fixed-point detection, and repeated-state cycle detection.

## Devices

- `uart`
- `gpio`
- `i2c`
- `spi_device`
- `spi_host`
- `pattgen`

Each device directory contains:

- `<device>.c`
- `<device>.h`
- `<device>_qp_shim.c`
- `<device>_qp_shim.h`

Temporary runtime harnesses, binaries, compiler stubs, and pass logs are not
included.

## Regeneration

Run from the `zhx` repository root:

```sh
./build/qemu-passes opentitan_examples/<device>_llhd.mlir \
  --all-passes \
  --qemu-emit-c \
  --qemu-output-dir=generated_settle_cycle_test \
  --no-reports
```

## Validation

The six core model C files passed:

- strict C11 syntax checking with `-Werror` and lightweight QEMU API stubs;
- one runtime `reset -> write -> read` smoke test;
- generated-code scans confirming repeated-state settle detection is present;
- generated-code scans confirming no emitter `TODO` markers remain.

These checks validate the generated core logic outside a full QEMU build. They
do not replace linking the models into QEMU and running guest firmware tests.

Full QEMU integration and firmware results are recorded in
`FIRMWARE_TEST_RESULTS.md`.
