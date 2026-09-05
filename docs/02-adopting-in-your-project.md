# Adopting this in an existing TMS570 project

The sandbox is a template. This page is the checklist for moving the pattern into a
real HALCoGen / CCS / Embedded Coder project.

## 1. Decide where the mock boundary is

Draw the line **one layer above the register-poking code**:

```
application logic  --->  hand-written HAL / HALCoGen driver API  --->  registers
  (mock the HAL)             (overlay-redirect the registers)
```

- Application modules call driver functions (`adcGetData()`, `gioSetBit()`,
  `sciSend()`, or your own thin wrappers). **Mock those headers with CMock.**
- Driver modules dereference `adcREG1->…`. **Redirect the overlay** to RAM and test the
  real driver code.

If your application calls HALCoGen APIs directly you can mock HALCoGen's own headers
(`adc.h`, `gio.h`, `sci.h`, …) - they are plain prototypes and CMock parses them. A
thin project-owned HAL (as in `src/hal/adc_hal.h` here) is still recommended: it is
smaller, stable across HALCoGen regenerations, and documents exactly which hardware
services the application depends on.

## 2. Copy the build glue

Copy `cmake/`, `CMakePresets.json`, `test/support/cmock_config.yml`, `target/`,
`tools/` and the relevant lines from the top-level `CMakeLists.txt`. Keep the
firmware's own target build (CCS project) as-is; CMake here builds the host tests and,
with the `target` preset, the on-target *test* binaries - never the firmware. The CCS
project and the CMake project simply share the `src/` tree (and, for the on-target
run, the HALCoGen directory).

Add include paths for whatever the code under test needs (HALCoGen `include/`, Embedded
Coder output, etc.) via the `INCLUDES` argument of `add_unity_test()`.

## 3. Mocking HALCoGen headers

```cmake
add_cmock_mock(${HALCOGEN}/include/adc.h)
add_unity_test(test_my_module
    SOURCES  ${SRC}/my_module.c
    MOCKS    mock_adc
    INCLUDES ${HALCOGEN}/include ${SRC})
```

HALCoGen headers pull in `sys_common.h` and `reg_*.h`, which compile fine on gcc. The
things that do **not** are in `sys_core.h` / `sys_vim.h` (assembly intrinsics) and any
`#pragma` that TI's compiler understands and gcc rejects. Handle them with a
`test/support/` directory that is first on the include path and contains stub
versions of those headers - e.g. a `sys_core.h` whose `_disable_IRQ_interrupt_()` is
an empty inline function. `add_unity_test()` already puts `test/support/` on the
include path.

## 4. Redirecting a HALCoGen register overlay without editing generated files

`reg_adc.h` ends with:

```c
#define adcREG1 ((adcBASE_t *)0xFFF7C000U)
```

Regenerating HALCoGen would wipe any `#ifdef UNIT_TEST` you add. Two clean options:

- **Include-guard jamming.** HALCoGen headers are guarded (`#ifndef __REG_ADC_H__`).
  Put a test-only `reg_adc.h` in `test/support/` that defines the same guard macro,
  copies the `adcBASE_t` typedef, and points `adcREG1` at `extern adcBASE_t adcREG1_fake;`.
  Because `test/support/` comes first on the include path, the generated file is never
  seen by the test build. No generated file is touched.
- **Macro override on the command line.** `target_compile_definitions(test_x PRIVATE
  adcREG1=&adcREG1_fake)` works only if the generated header does not `#define` it
  again; HALCoGen's does, so prefer guard jamming.

This sandbox uses the simpler `#ifdef UNIT_TEST` form in its own (non-generated)
`tms570_adc_regs.h` because the point is to show the mechanism.

## 5. Simulink Embedded Coder modules

Worked example in this repo: model `heater_ctrl` under `src/gen/heater_ctrl_ert_rtw/`,
its glue `src/app/heater_task.c`, and the tests `test_heater_ctrl.c` (model driven
directly) and `test_heater_task.c` (model mocked). The generated files there are a
labelled hand-written stand-in with the exact layout of ERT output, so the mechanics
can be exercised without MATLAB; swap the directory for real output and nothing else
changes.

### 5.1 Folder convention

- Copy Embedded Coder's `<model>_ert_rtw/` directory to `src/gen/<model>_ert_rtw/`
  verbatim (or point *Code Generation → Code generation folder* there). If the model
  uses shared utilities, `slprj/ert/_sharedutils/` comes too.
- Add one hand-maintained `CMakeLists.txt` in that directory (the only file you own
  there) that builds the model as a static library:

  ```cmake
  add_library(heater_ctrl_model STATIC heater_ctrl.c heater_ctrl_data.c)
  target_include_directories(heater_ctrl_model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
  target_compile_options(heater_ctrl_model PRIVATE $<$<C_COMPILER_ID:GNU,Clang>:-Wall;-Wextra>)  # no -Werror
  ```

  Generated code gets its own warning policy: report, don't fail. You cannot fix
  warnings in generated code; you change model settings and regenerate.
- Never edit generated files. After a regeneration the diff should be confined to
  `src/gen/`; if tests then fail, the model's interface or behaviour changed, which is
  exactly what you want to know.

### 5.2 Model configuration that matters for host testing

| Configuration parameter | Set to | Why |
|---|---|---|
| System target file | `ert.tlc` | Bare-metal output, no `rtModel` scheduler baggage |
| Code interface packaging | *Nonreusable function* | Globals `<m>_U`, `<m>_Y`, `<m>_P`, `<m>_DW`, entry points `<m>_step()`; what this repo assumes. *Reusable function* passes an `RT_MODEL_<m>_T*` and the structs as arguments instead - tests then own an instance of each |
| Generate an example main program | off | `ert_main.c` would define a second `main()` next to Unity's runner |
| Default parameter behavior | *Tunable* | Emits `<m>_P` so tests can move set-points, thresholds, debounce counts. *Inlined* bakes them into the code as constants |
| Remove internal data zero initialization | (default on) | `<m>_initialize()` then does **not** clear states/IO; it relies on C start-up. Tests must `memset` `<m>_DW`, `<m>_U`, `<m>_Y` in `setUp()` (see `test_heater_ctrl.c`), or turn this off |
| Hardware Implementation → Device | ARM Compatible / ARM Cortex-R | Fixes `rtwtypes.h`: `int32_T` = `int`, big-endian, no `long long` unless the TI compiler is told to support it |

### 5.3 `rtwtypes.h` and host compilers

- `int32_T`/`uint32_T` map to `int`/`unsigned int`, 32-bit on both target and host.
  `ulong_T` (`unsigned long`) is 64-bit on x86-64; if generated code uses it, build
  the tests with `-m32`.
- `rtwtypes.h` defines `true`/`false` as `(1U)`/`(0U)` **only if not already defined**.
  Including it *before* `<stdbool.h>` makes `<stdbool.h>` redefine them (`1U` → `1`).
  gcc and clang stay silent only because the second definition is inside a system
  header (`-Wsystem-headers` shows it); a HALCoGen or project header that defines
  `true`/`false` itself, MISRA checkers, and other front ends report it, and under
  `-Werror` that is a build break. Rule: in any file that mixes them, include the
  hand-written headers (which pull in `<stdbool.h>`) before the generated header.
  `heater_task.c` and `test_heater_task.c` show the order.
- Tell CMock about the generated types once, in `cmock_config.yml`:
  `boolean_T: UINT8`, `real32_T: FLOAT`, `real_T: DOUBLE`.

### 5.4 Testing the model directly (`test_heater_ctrl.c`)

```cmake
add_unity_test(test_heater_ctrl LIBS heater_ctrl_model)   # no SOURCES, no mocks
```

The test links the same library object code the firmware uses. Pattern:

```c
static boolean_T step(real32_T temp_degC, boolean_T enable) {
    heater_ctrl_U.temp_degC = temp_degC;
    heater_ctrl_U.enable    = enable;
    heater_ctrl_step();
    return heater_ctrl_Y.heater_cmd;
}
```

- Time is step count: an expectation "faults after 300 ms" at Ts = 0.1 s is "on the
  third step". Write a `step_n()` helper and name the constants after the requirement.
- Float outputs: use `TEST_ASSERT_FLOAT_WITHIN`, not `_EQUAL_FLOAT`, for anything that
  went through arithmetic.
- Tunable parameters are one global; save a copy in the first `setUp()` and restore it
  in `tearDown()` or later tests inherit the change.
- Multi-rate models emit `<m>_step0()`, `<m>_step1()`, ...; call them in the ratio the
  scheduler would.
- Generated code calls no hardware, so no mocks - unless the model has custom-code /
  S-function blocks that call your HAL; then mock that HAL header as usual.

### 5.5 Mocking the model from its caller (`test_heater_task.c`)

The glue that feeds the model (sensor scaling, enable logic, output plumbing) should
be tested with the model *mocked*, so the two test files fail for different reasons.
Two things bite:

1. **`extern` prototypes.** CMock's `:treat_externs` defaults to `:exclude`, and every
   Embedded Coder entry point is declared `extern void <m>_step(void);`. The mock is
   generated **empty** and the link fails with undefined `<m>_step_Expect`. Set
   `:treat_externs: :include` in `cmock_config.yml`.
2. **Data objects.** The mock supplies functions only. `<m>_U` and `<m>_Y` are extern
   globals declared in the model header, so the test defines them. Use the `:callback`
   plugin to install a fake `<m>_step()` that snapshots `_U` and writes the `_Y` the
   scenario needs:

   ```c
   ExtU_heater_ctrl_T heater_ctrl_U;            /* normally in heater_ctrl.c */
   ExtY_heater_ctrl_T heater_ctrl_Y;

   static void fake_step(int num_calls) { seen_U = heater_ctrl_U; heater_ctrl_Y = next_Y; }
   void setUp(void) { heater_ctrl_step_Stub(fake_step); }
   ```

### 5.6 Where Simulink Test still fits

Keep Simulink Test (SIL/PIL, back-to-back equivalence, model coverage) for proving
the generated code matches the model and meets model-level requirements. Use Unity
for everything the model cannot see: unit conversion into `_U`, what drives `enable`,
what the outputs are wired to, and as the per-commit gate on a CI runner without a
MATLAB licence.

## 6. Running the same tests on the target

Done in [03-on-target.md](03-on-target.md): `cmake --preset target` cross-compiles
every test binary with TI `armcl`, HALCoGen supplies start-up code and the SCI
driver, Unity prints over the UART, and `ctest --preset target` flashes and captures.
Keep `UNIT_TEST` defined there too - the overlay tests are driver-logic tests, not
hardware tests. Run it per release, or per commit if a board sits in CI; the host
suite stays the everyday gate.

## 7. CI

`.github/workflows/ci.yml` is generic: install cmake/ninja/ruby, then
`cmake --preset` / `cmake --build --preset` / `ctest --preset`. Jobs:

| Job | Preset | Catches |
|---|---|---|
| Host unit tests (gcc, clang) | `host`, `host-clang` | the per-commit gate; two compilers keep the code portable |
| Host unit tests (gcc -m32) | `host-m32` | ILP32 assumptions (`long`, pointer width) that hide on LP64 |
| Coverage | `host-coverage` | what the suite does not exercise; HTML report as a build artifact, summary on the job page |
| Target build | `target-ci` | the whole tree compiled and linked by the real TI compiler (downloaded and cached by the job) against a stub board-support package |
| Target build dry run | `target-dryrun` | the cross-build plumbing, without the TI tools |

Copy it verbatim. The on-target run (docs/03) is not in CI unless a board is.

## 8. Coverage

`cmake/Coverage.cmake` adds the `COVERAGE` option and a `coverage` target; the
`host-coverage` preset turns it on.

```sh
cmake --preset host-coverage && cmake --build --preset host-coverage
cmake --build --preset host-coverage-report     # ctest, then gcovr
```

- **What is instrumented:** only the code under test - the firmware sources each
  test compiles (`add_unity_test(... SOURCES ...)`) and libraries you mark with
  `coverage_instrument(<target>)`, as the Embedded Coder model library is. The report
  is filtered to `src/`, so Unity, CMock, mocks, runners and the test files never
  dilute or inflate the numbers. The smoke library `tms570_app` is not instrumented;
  it is never executed.
- **Output:** `build/host-coverage/coverage/index.html` (per-file, per-line, with
  branch markers), `coverage.xml` (Cobertura, for CI dashboards) and `summary.txt`.
  `--delete` clears the counters after each report so runs do not accumulate.
- **Gate:** `-DCOVERAGE_FAIL_UNDER_LINE=90` makes the `coverage` target fail below
  that line coverage. Keep it at 0 until there is real code, then set it to what
  the suite actually achieves and ratchet up.
- **Reading branch coverage:** gcov counts *both* outcomes of every condition. The
  first report of this sandbox showed 100% lines but two missed branches, both the
  "already at maximum" side of a saturating counter (`if (count < MAX) count++`), in
  `temp_monitor.c` and in the model. Those are real gaps (a wrapping counter would
  clear a fault), so each got a test - `test_error_counter_saturates_without_wrapping`
  and `test_overtemp_counter_saturates_without_wrapping` - and the suite is now at
  100% branches. Expect the branch view to point at exactly this kind of thing.
  Gate on line coverage; read branch coverage.
- **gcc only.** clang emits a different profile format; the preset refuses other
  compilers. gcovr runs anywhere Python does (`pip install gcovr`), including
  Windows, which is why it was chosen over lcov/genhtml.
- **Generated code** (Embedded Coder) is in the report on purpose: it tells you which
  model paths the *integration* tests reach. Model-level coverage belongs to
  Simulink Test; exclude `src/gen/` with `--exclude` in `Coverage.cmake` if you gate
  on the number.

## 9. ILP32 on the host

`cmake --preset host-m32` builds and runs the suite with `-m32` (needs
`gcc-multilib`): `long` and pointers are 32-bit, `size_t` is 32-bit, as on the
Cortex-R4F. It is a cheap approximation of the target's data model; the on-target run
(docs/03) is the real one. Everything else (endianness, packed enums, the compiler)
stays host-like.
