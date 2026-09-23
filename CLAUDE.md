# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository. <!-- markdownlint-disable-line MD013 -->

## What this repository is

A template, not a product. It exists to settle *how* to unit-test TMS570LS3137
firmware and to be copied into real projects. The `docs/` tree is the main
deliverable; `src/` and `test/` are worked examples that demonstrate each pattern.
Keep that in mind when judging scope: a change to an example usually implies a change
to the document that teaches it.

## Commands

```sh
cmake --preset host            # configure (fetches Unity + CMock via FetchContent)
cmake --build --preset host    # generate mocks + runners, compile
ctest --preset host            # run all five test executables
```

Other presets: `host-clang` (compiler neutrality), `host-m32` (ILP32, needs
`gcc-multilib`), `host-coverage`, `target`, `target-ci`, `target-dryrun`.

```sh
ctest --preset host -R test_adc_hal --output-on-failure   # one test binary
./build/host/test/test_adc_hal                            # same, with Unity's raw output
```

**There is no way to run a single test *function*.** `UNITY_USE_COMMAND_LINE_ARGS` is
not defined, so the runner takes no `-n` filter; one binary per `test/test_*.c` is the
finest granularity.

```sh
cmake --preset host-coverage && cmake --build --preset host-coverage
cmake --build --preset host-coverage-report   # runs ctest, writes build/host-coverage/coverage/
```

Gate with `-DCOVERAGE_FAIL_UNDER_LINE=<pct>`. gcc only — the preset refuses clang.

```sh
npx --yes markdownlint-cli2@0.23.3            # lint docs; config supplies globs + ignores
```

Pin that version. markdownlint adds rules in minor releases (MD060 arrived in 0.41 and
flags every `|---|---|` row by default), so an unpinned run fails on someone else's
release. `.markdownlint-cli2.jsonc` explains each non-default rule.

## Architecture

### Two builds of the same sources

`src/` is compiled twice. `add_subdirectory(src)` builds `tms570_app`, a library that
is **never executed** — it exists so the real register addresses and production code
paths compile at least once per build. Each test executable recompiles the sources it
needs **with `-DUNIT_TEST`**, which is what redirects the register overlays. If a
change compiles in tests but not in `tms570_app` (or vice versa), that split is why.

### The four test patterns

Each cuts a different seam; a change should redden exactly one test file.

| Pattern | Example | Mechanism |
|---|---|---|
| Mock the HAL boundary | `test_temp_monitor.c` | CMock generates `mock_adc_hal.c` from the header |
| Redirect the register overlay | `test_adc_hal.c`, `test_gio_hal.c` | `-DUNIT_TEST` points `adcREG1` at a RAM struct the test defines |
| Drive generated code as shipped | `test_heater_ctrl.c` | links the model library, sets `_U`, calls `_step()`, asserts `_Y` |
| Mock generated code from its caller | `test_heater_task.c` | model mocked so glue and model fail for different reasons |

`docs/05-choosing-a-method.md` is the decision guide for which to use.

### The CMake test helpers

`cmake/UnityTest.cmake` defines the whole contract:

- `add_cmock_mock(<header>)` — runs `cmock.rb`, wraps the output in a `mock_<name>` library.
- `add_unity_test(<name> SOURCES … MOCKS … INCLUDES … LIBS …)` — generates the runner,
  compiles `SOURCES` with `-DUNIT_TEST`, registers with CTest.

**`SOURCES` and `LIBS` are not interchangeable.** `SOURCES` recompiles with
`UNIT_TEST` defined; `LIBS` links a prebuilt library *as shipped*, which is how the
Embedded Coder model is tested against the same object code the firmware links.

`cmake/Coverage.cmake` must be `include()`d before `add_subdirectory(src)` — both
`UnityTest.cmake` and the generated-model `CMakeLists.txt` call `coverage_instrument()`
unconditionally, so omitting it fails configure with `Unknown CMake command`.

### Generated code (`src/gen/heater_ctrl_ert_rtw/`)

A hand-written stand-in laid out exactly like Embedded Coder ERT output, so the
mechanics work without MATLAB. **Never edit it** — the only file there that is yours is
its `CMakeLists.txt`. Its warning policy is `-Wall -Wextra` without `-Werror`: you
cannot fix generated code, you change model settings and regenerate.

Consequences for tests, all covered in `docs/02` §5:

- `heater_ctrl_initialize()` does **not** zero state (ERT's "remove internal data zero
  initialization"); `setUp()` must `memset` `_DW`, `_U`, `_Y`.
- `heater_ctrl_P` is a tunable global — save it once and restore it in `tearDown()`.
- Include hand-written headers **before** the generated one. `rtwtypes.h` defines
  `true`/`false` as `1U`/`0U` only if undefined; the other order makes `<stdbool.h>`
  redefine them, which is a build break under `-Werror` on stricter front ends.

### CMock configuration (`test/support/cmock_config.yml`)

Three settings are load-bearing:

- `:treat_externs: :include` — every Embedded Coder entry point is `extern`; without
  this the model mock generates empty and the link fails on `<m>_step_Expect`.
- `:enforce_strict_ordering: true` — call order is part of the test, globally.
- `:treat_as` — project enums and `rtwtypes.h` types (`boolean_T`, `real32_T`).

Add project enum types here or CMock cannot compare them.

### On-target build

`cmake --preset target` cross-compiles the *same* test sources with TI `armcl`;
`tools/run_on_target.sh` is registered as `CMAKE_CROSSCOMPILING_EMULATOR`, so `ctest`
flashes each binary and reads Unity's verdict back over the SCI. `UNIT_TEST` stays
defined there — the overlay tests are driver-*logic* tests, not hardware tests.
`target-ci` uses the real compiler against `tools/halcogen-stub` (compile and link
only); `target-dryrun` exercises the plumbing with stand-in `armcl` scripts.
`target/halcogen/` is generated by you and git-ignored.

## Conventions that will bite

- **Tests must stay target-portable.** The same files cross-compile: no `malloc`, no
  `printf`/`<stdio.h>`, no `<time.h>`, `<stdint.h>` widths rather than bare `long`,
  large fixtures at file scope. `docs/05` §5 has the list.
- **`-Werror` on hand-written firmware only** — never on generated code, mocks,
  runners or test executables.
- **Threshold logic gets boundary tests, not partition representatives.** The suite
  tests at each threshold and adjacent to it, because a representative executes every
  line but cannot tell `>=` from `>`. For a quantised input adjacent means the next
  representable value; for a continuous one it is a gap you choose from the
  requirement and write down. See `docs/08` §2.1 and §3.2.
- **Verify a new test can fail.** Break the behaviour, watch it go red, revert. The
  repo's habit is a quick mutation check (change an operator, rebuild, run); `docs/08`
  §4.3 covers it, including which survivors are *not* gaps.
- **Coverage is a floor.** The suite has sat at 100% lines while real defects were
  invisible, twice. Gate on lines, read branches.
- **CI job names are coupled to the ruleset.** `.github/rulesets/main-branch-protection.json`
  lists required checks by job *name*; renaming a job in `ci.yml` without updating it
  blocks every merge. That JSON is not live — GitHub does not read it from the repo, so
  it gates nothing until re-applied with the `PUT` in `.github/rulesets/README.md`.

## Docs

| Doc | Covers |
|---|---|
| `01-approach-and-options.md` | decision record: why Unity + CMock; why SPNU615 and Simulink SIL are not first-pass |
| `02-adopting-in-your-project.md` | retrofitting an existing HALCoGen / Embedded Coder project |
| `03-on-target.md` | running the same tests on the board |
| `04-simulink-test.md` | MIL/SIL/PIL sketch — **unverified, never executed**, needs licences |
| `05-choosing-a-method.md` | picking a pattern and a venue |
| `06-new-project-setup.md` | green-field setup, track A plus add-ons |
| `07-unity-best-practices.md` | writing the test: lifecycle, assertions, CMock traps |
| `08-test-design-and-strategy.md` | deriving cases, how much is enough, designing for testability |

Prose wraps at 88 columns (markdownlint allows 90; tables and code blocks are exempt).
`matlab/` is an unverified sketch written without MATLAB — every API call there is
flagged `VERIFY`.
