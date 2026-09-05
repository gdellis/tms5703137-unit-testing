# TMS570LS3137 unit-testing sandbox

[![CI](https://github.com/gdellis/tms5703137-unit-testing/actions/workflows/ci.yml/badge.svg)](https://github.com/gdellis/tms5703137-unit-testing/actions/workflows/ci.yml)

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

Prerequisites: CMake ≥ 3.21, Ninja, a C11 compiler (gcc or clang), Ruby (≥ 2.7; used
only to *generate* mocks and test runners), git (for FetchContent).

```sh
cmake --preset host            # fetches Unity/CMock, generates build files
cmake --build --preset host    # generates mocks + runners, compiles
ctest --preset host            # runs every test executable
```

Swap `host` for `host-clang` to build with clang, or `host-m32` for a 32-bit data
model like the target's (needs `gcc-multilib`). All run in CI
(`.github/workflows/ci.yml`).

Coverage of the code under test (gcc + gcovr, HTML and Cobertura in
`build/host-coverage/coverage/`):

```sh
cmake --preset host-coverage && cmake --build --preset host-coverage
cmake --build --preset host-coverage-report     # runs the tests, writes the report
```

The same tests also run **on the board**: `cmake --preset target` cross-compiles them
with TI `armcl` (HALCoGen supplies start-up and SCI), `ctest --preset target` flashes
each binary and reads Unity's verdict back over the UART. See
[docs/03-on-target.md](docs/03-on-target.md). CI compiles and links all of it with
the real TI compiler on every commit (`target-ci` preset; the job downloads and caches
the compiler) against a stub board-support package, and `target-dryrun` checks the
plumbing on machines without the TI tools.

## What is in the box

```
src/app/temp_monitor.[ch]        application logic under test (calls the ADC HAL)
src/app/heater_task.[ch]         glue: sensor -> model inputs, model outputs -> GIO pin
src/hal/adc_hal.[ch]             ADC HAL: mocked in app tests, overlay-tested itself
src/hal/gio_hal.[ch]             GIO HAL: same treatment
src/hal/tms570_*_regs.h          HALCoGen-style register structs; #ifdef UNIT_TEST redirects them
src/gen/heater_ctrl_ert_rtw/     Embedded Coder-style model output (a labelled stand-in,
                                 laid out exactly like ERT output - see docs/02 section 5)
test/test_temp_monitor.c         app tests: pure maths + mock-driven interaction tests
test/test_adc_hal.c              driver tests: pre-load "registers", inspect what was written
test/test_gio_hal.c              driver tests, same pattern
test/test_heater_ctrl.c          model tests: set _U, call _step(), assert _Y; tune _P
test/test_heater_task.c          glue tests: sensor, GIO *and the model* are mocked
test/support/cmock_config.yml    CMock plugins/options
cmake/FetchUnityCMock.cmake      pins Unity v2.7.0 + CMock v2.7.0 via FetchContent
cmake/UnityTest.cmake            add_cmock_mock() / add_unity_test() helpers
cmake/Coverage.cmake             COVERAGE option, coverage_instrument(), 'coverage' target (gcovr)
cmake/toolchain-ti-armcl.cmake   TI ARM compiler toolchain file (Cortex-R4F, big-endian)
target/                          on-target runtime: unity_config.h, Unity-over-SCI hooks,
                                 CMake glue for the HALCoGen project you generate there
tools/run_on_target.sh           CTest launcher: flash, capture serial, return verdict
tools/unity_serial_capture.py    the capture half of that, usable standalone
tools/halcogen-stub/             stand-in HALCoGen project: compiles and links, never runs
tools/dryrun/                    stand-in armcl/armar scripts for the dry run
tools/ci/install-ti-cgt.sh       unattended download + install of TI ARM CGT (used by CI)
```

Four testing patterns are demonstrated, because a TMS570 project needs all of them:

1. **Mock the HAL boundary** (`test_temp_monitor.c`). Application code is compiled
   against a CMock-generated `mock_adc_hal.c`. Tests state which HAL calls are
   expected, with what arguments, and what they return - including out-parameters via
   `_ReturnThruPtr_`. Unexpected or missing calls fail the test.

2. **Redirect the register overlay** (`test_adc_hal.c`, `test_gio_hal.c`). Driver code
   that pokes memory-mapped registers is compiled unchanged with `-DUNIT_TEST`, which
   makes `adcREG1` point at a plain `adcBASE_t` struct owned by the test instead of
   `0xFFF7C000`. The test pre-loads status bits / FIFO words and asserts on what the
   driver wrote. This is how you test HALCoGen-level code without hardware.

3. **Drive generated code as shipped** (`test_heater_ctrl.c`). The Embedded Coder
   output is built once as a library (no `UNIT_TEST`, its own warning policy) and the
   test links that library. Set `heater_ctrl_U`, call `heater_ctrl_step()`, assert on
   `heater_ctrl_Y`; twist `heater_ctrl_P` to test tunable parameters. No mocks.

4. **Mock generated code from its caller** (`test_heater_task.c`). The hand-written
   glue that feeds the model is tested with the model replaced by a CMock mock, so a
   model change cannot break a glue test and vice versa. Shows the two wrinkles this
   involves: CMock must be told to mock `extern` prototypes, and the model's global
   `_U`/`_Y` structs have to be defined by the test.

## Adding a test

1. Write `test/test_<module>.c` with `setUp`, `tearDown` and `void test_*(void)`
   functions. `#include "mock_<header>.h"` for every header you want mocked.
2. In `test/CMakeLists.txt`: `add_cmock_mock(<header>)` once per header, then
   `add_unity_test(test_<module> SOURCES <files under test> MOCKS <mock targets>
   [LIBS <prebuilt libraries>])`.
3. `cmake --build --preset host && ctest --preset host`.

Runners are generated - test files never contain `main()`.

## Where to go from here

The roadmap from the first pass is done: host tests, Embedded Coder patterns,
on-target execution, coverage. Things this template deliberately leaves to the real
project:

- **Hardware-in-the-loop tests** (does the ADC actually convert?) - a separate suite
  with board fixtures, not the overlay tests re-run on silicon.
- **Simulink Test SIL/PIL** for model-vs-code equivalence, on the Simulink side;
  complementary to the model/glue tests here (docs/02 section 5). An unverified
  sketch of what that track looks like - Test Manager scripting, MIL/SIL/PIL,
  CI on a self-hosted runner - is in
  [docs/04-simulink-test.md](docs/04-simulink-test.md).
- **A coverage gate** once there is real code: `-DCOVERAGE_FAIL_UNDER_LINE=<pct>`
  (docs/02 section 8).

See [docs/01-approach-and-options.md](docs/01-approach-and-options.md) for details.

## License

[MIT](LICENSE) - copy it into your projects freely.

Nothing third-party is vendored here, but a build pulls in code under other terms:

| What | Where it comes from | Terms |
|---|---|---|
| Unity, CMock | fetched at configure time by `cmake/FetchUnityCMock.cmake` | MIT |
| TI ARM compiler and its run-time libraries | installed by `tools/ci/install-ti-cgt.sh`, or with CCS | TI's own licence; not redistributed by this repo |
| HALCoGen output | generated by you into `target/halcogen/`, which is git-ignored | TI's own licence; never committed here |

`tools/halcogen-stub/` is hand-written to stand in for that generated code, so it
carries this repository's licence rather than TI's.
