# TMS570LS3137 unit-testing sandbox

A small, runnable template showing how to unit-test TMS570LS3137 firmware **on the
host PC** with [Unity](https://www.throwtheswitch.org/unity) +
[CMock](https://www.throwtheswitch.org/cmock) driven by CMake/CTest. No board, no
Code Composer Studio, no TI compiler needed for the tests.

The repo exists to settle the approach and to be copied into real TMS570 projects.
The reasoning behind the choices (including why TI's SPNU615 "Test Automation Unit"
and Simulink SIL are *not* the first-pass tools) is in
[docs/01-approach-and-options.md](docs/01-approach-and-options.md). The step-by-step
for applying this to an existing HALCoGen / Embedded Coder project is in
[docs/02-adopting-in-your-project.md](docs/02-adopting-in-your-project.md).

## Quick start

Prerequisites: CMake ≥ 3.21, Ninja, a C compiler (gcc or clang), Ruby (≥ 2.7; used
only to *generate* mocks and test runners), git (for FetchContent).

```sh
cmake --preset host            # fetches Unity/CMock, generates build files
cmake --build --preset host    # generates mocks + runners, compiles
ctest --preset host            # runs every test executable
```

Swap `host` for `host-clang` to build with clang. Both run in CI
(`.github/workflows/ci.yml`).

## What is in the box

```
src/app/temp_monitor.[ch]     application logic under test (calls the HAL)
src/hal/adc_hal.h             the HAL interface  -> mocked by CMock in app tests
src/hal/adc_hal.c             the HAL driver     -> tested against a RAM register overlay
src/hal/tms570_adc_regs.h     HALCoGen-style register struct; #ifdef UNIT_TEST redirects it
test/test_temp_monitor.c      app tests: pure maths + mock-driven interaction tests
test/test_adc_hal.c           driver tests: pre-load "registers", inspect what was written
test/support/cmock_config.yml CMock plugins/options
cmake/FetchUnityCMock.cmake   pins Unity v2.7.0 + CMock v2.7.0 via FetchContent
cmake/UnityTest.cmake         add_cmock_mock() / add_unity_test() helpers
```

Two testing patterns are demonstrated, because a TMS570 project needs both:

1. **Mock the HAL boundary** (`test_temp_monitor.c`). Application code is compiled
   against a CMock-generated `mock_adc_hal.c`. Tests state which HAL calls are
   expected, with what arguments, and what they return - including out-parameters via
   `_ReturnThruPtr_`. Unexpected or missing calls fail the test.

2. **Redirect the register overlay** (`test_adc_hal.c`). Driver code that pokes
   memory-mapped registers is compiled unchanged with `-DUNIT_TEST`, which makes
   `adcREG1` point at a plain `adcBASE_t` struct owned by the test instead of
   `0xFFF7C000`. The test pre-loads status bits / FIFO words and asserts on what the
   driver wrote. This is how you test HALCoGen-level code without hardware.

## Adding a test

1. Write `test/test_<module>.c` with `setUp`, `tearDown` and `void test_*(void)`
   functions. `#include "mock_<header>.h"` for every HAL header you want mocked.
2. In `test/CMakeLists.txt`: `add_cmock_mock(<header>)` once per header, then
   `add_unity_test(test_<module> SOURCES <files under test> MOCKS <mock targets>)`.
3. `cmake --build --preset host && ctest --preset host`.

Runners are generated - test files never contain `main()`.

## Roadmap (not in this pass)

- **On-target execution** of the same Unity tests with the TI ARM compiler (CCS,
  simulator or board) to catch big-endian / ILP32 / compiler-specific behaviour.
- **Simulink Embedded Coder** modules: SIL/PIL in Simulink Test for model
  equivalence, plus host Unity tests calling `Model_step()` for integration.
- **Coverage** (gcov/lcov on host) once there is real code to measure.

See [docs/01-approach-and-options.md](docs/01-approach-and-options.md) for details.
