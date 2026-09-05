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

Copy `cmake/`, `CMakePresets.json`, `test/support/cmock_config.yml` and the relevant
lines from the top-level `CMakeLists.txt`. Keep the target build (CCS project) as-is;
CMake here only builds host tests. The CCS project and the CMake project simply share
the `src/` tree.

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

- Generate with `ert.tlc`, "Reusable function" or "Nonreusable function" interface;
  put the output under `src/gen/<ModelName>/` and never edit it.
- Set *Hardware Implementation → Device vendor/type* to ARM Cortex-R and enable
  *Support: long long* only if the target compiler does; keep *Test hardware* matching
  the host for SIL runs.
- Host Unity tests can drive the model directly:

  ```c
  #include "MyModel.h"
  void test_step_raises_output_when_input_high(void) {
      MyModel_initialize();
      MyModel_U.temperature = 105.0f;
      MyModel_step();
      TEST_ASSERT_TRUE(MyModel_Y.fault);
  }
  ```

  Generated code calls no hardware itself, so no mocks are needed unless your model has
  custom-code blocks that call the HAL - then mock that HAL header as usual.
- Keep Simulink Test (SIL/PIL, equivalence, coverage) for model-level verification;
  use Unity for the integration seams between generated and hand-written code.

## 6. Roadmap: running the same tests on the target

1. Add a CMake toolchain file for `armcl` (or a second CCS project) that compiles
   Unity's `unity.c`, the generated runners and the test sources.
2. Provide `unity_config.h` with `UNITY_OUTPUT_CHAR(c)` mapped to `sciSendByte()` (or
   CCS CIO) and `UNITY_EXCLUDE_FLOAT` if the FPU is not initialised.
3. Do **not** define `UNIT_TEST` for this build - the drivers should hit the real
   registers.
4. Run the suite once per release, or on a board in CI if you have one; the host suite
   stays the per-commit gate.

## 7. CI

`.github/workflows/ci.yml` is generic: install cmake/ninja/ruby, then
`cmake --preset` / `cmake --build --preset` / `ctest --preset`. Copy it verbatim; add
a `-m32` preset if you want the ILP32 check.
