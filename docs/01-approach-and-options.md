# Unit-testing approach for TMS570LS3137 firmware

Decision record for this sandbox. Read this first if you want to know *why* the
template looks the way it does.

## The target and what makes it awkward to test

| Property | TMS570LS3137 | Typical host PC | Why it matters |
|---|---|---|---|
| Core | ARM Cortex-R4F, lock-step | x86-64 | different compiler, intrinsics |
| Endianness | **Big-endian (BE-32)** | Little-endian | byte-packing, unions, comms frames behave differently |
| Data model | ILP32 (`long` = 32 bit) | LP64 (`long` = 64 bit) | overflow / width bugs hide on host |
| Toolchain | TI ARM (`armcl`) via CCS / HALCoGen | gcc / clang | `#pragma`s, `__attribute__`, `_disable_IRQ_interrupt_()` etc. |
| Peripherals | memory-mapped `volatile` structs at fixed addresses | none | driver code dereferences `0xFFF7C000` |
| Some code | Simulink Embedded Coder generated | - | plain C, but with its own types (`rtU`, `rtY`, `real32_T`) |

Anything that talks to hardware has to be **isolated behind a boundary** before it can
run on a host, and anything that is host-tested still needs one eventual run on the
real compiler/endianness before it is trusted.

## Where tests can execute

1. **Host-native.** Compile the code under test with gcc/clang and run it as a normal
   process. Seconds per run, free, trivially CI-able, debuggable with any host tool.
   Cannot see target-specific behaviour (see table).
2. **On-target / simulator.** Cross-compile the same tests with `armcl` and run on a
   board (XDS probe, results over SCI/UART or semihosting) or the CCS simulator. Slow
   and awkward to automate, but it is the only place endianness and compiler quirks are
   real. Unity is designed to run here too - it is a couple of `.c` files with a
   pluggable `UNITY_OUTPUT_CHAR`.
3. **Simulink SIL / PIL.** For Embedded Coder output only: Simulink Test runs the
   generated C (SIL, on host) or the target binary (PIL) against the model and checks
   equivalence. Excellent for the model, useless for hand-written drivers and glue.

## Options weighed

### Unity + CMock + CMake, host-native  -  **chosen for pass 1**

- Unity: tiny xUnit framework for C, portable to anything with a C compiler.
- CMock: reads a header, emits `mock_<header>.c/.h` with `_Expect`, `_ExpectAndReturn`,
  `_Ignore`, `_ReturnThruPtr_` etc. Ruby is needed at generation time only.
- CMake + CTest: the build system the user already wants; FetchContent pins both
  libraries by git tag (v2.7.0 each) so nothing is vendored.
- Familiar to the team, huge community, MIT licensed.

Costs: Ruby on the dev/CI machine; hardware access must be behind a mockable interface
or a redirectable register overlay (both patterns are in this repo).

### Unity on-target (CCS + armcl)  -  pass 2

Same tests, cross-compiled. Needs a CCS project (or a CMake toolchain file for
`armcl`), a linker script with a small RAM/Flash budget for the test binary, and an
output channel. Worth doing once for a representative module to validate the host
results; not worth doing for every commit.

### Simulink Embedded Coder SIL/PIL  -  complementary

Use Simulink Test for model-vs-code equivalence of generated modules. Separately,
Embedded Coder output is ordinary C, so Unity on the host can call
`Model_initialize()` / `Model_step()` with `rtU` inputs and assert on `rtY` outputs.
That is the cheap way to integration-test generated code with hand-written code.
Requires MATLAB/Simulink/Embedded Coder/Simulink Test licences and is heavier than a
"first pass" tool.

### TI SPNU615 "Hercules Software Diagnostic Library Test Automation Unit"  -  out of scope

Despite the name this is **not a unit-testing framework for your code**. It is a
Windows/CCS tool that runs TI's *predefined* test cases against the
**SafeTI Diagnostic Library** (CPU self-test, ECC, PBIST, etc.) and produces
LDRAunit-based coverage and regression reports as ISO 26262 / IEC 61508 assessment
evidence. Requirements listed by TI: Windows, CCS 6+, SafeTI Diagnostic Library 2.2+,
Microsoft Office, and an LDRAunit-TI-Qual licence. It is only relevant if the product
integrates the SafeTI Diagnostic Library and needs its qualification package, and even
then it sits *alongside* your own unit tests rather than replacing them.

Sources: [SPNU615B (PDF)](https://www.ti.com/lit/ml/spnu615b/spnu615b.pdf),
[SPNU614 TAU installation guide](https://www.ti.com/lit/pdf/spnu614),
[SafeTI Diagnostic Library](https://www.ti.com/tool/SAFETI_DIAG_LIB),
[SPNU592 software safety manual](https://www.ti.com/lit/pdf/spnu592).

### Others considered

- **Ceedling** - Unity + CMock behind a Rake build with excellent defaults. The right
  answer if you do not want to own the CMake glue; rejected here only because CMake was
  a requirement. Note that `generate_test_runner.rb` and `cmock.rb`, which this repo
  drives from CMake, are exactly what Ceedling calls.
- **FFF (Fake Function Framework)** - header-only fakes with no Ruby. Good fallback for
  a project that cannot install Ruby; less expressive than CMock (no strict ordering,
  manual return-through-pointer).
- **CppUTest / GoogleTest** - C++ frameworks. Fine for testing C, but they pull a C++
  toolchain into the picture and are harder to run on-target later. Unity keeps the
  host and target stories identical.

## Decision

Pass 1 = this repository: **Unity v2.7.0 + CMock v2.7.0, host-native, CMake/CTest,
GitHub Actions**. Two patterns demonstrated: mock the HAL interface; redirect the
register overlay.

Follow-ups: on-target run of the same tests; Embedded Coder folder convention and
`Model_step()` tests; coverage.

## Pitfalls to design around from day one

- **Endianness.** Never rely on host results for byte-order-sensitive code (union
  punning, packing CAN/SCI frames, checksums over raw memory). Write those routines with
  explicit shifts/masks, and schedule them for the on-target run.
- **Integer widths.** Use `<stdint.h>` types everywhere. Optionally build the host
  tests with `-m32` (gcc-multilib) to get an ILP32 data model that matches the target.
- **`volatile` register access.** Keep it inside the driver layer; that is the layer
  you test with the overlay trick, and its callers with mocks.
- **TI intrinsics and pragmas.** `_disable_IRQ_interrupt_()`, `_enable_interrupt_()`,
  `#pragma CODE_STATE`, `#pragma INTERRUPT` do not exist on gcc. Put them behind a
  `sys_core.h`-style shim and give the test build a stub header (or mock them).
- **HALCoGen regeneration.** Do not hand-edit generated files to insert `#ifdef
  UNIT_TEST`. See the include-guard technique in
  [02-adopting-in-your-project.md](02-adopting-in-your-project.md).
- **Static state.** Modules with file-scope state (like `temp_monitor.c`) need an
  `_init()` the test can call from `setUp`; otherwise tests leak state into each other.
