# Testing with Simulink Test (MATLAB/Simulink)

**Status: sketch, not built or run.** Everything in this document was written without
access to MATLAB - this sandbox has none - so nothing here has been executed. Treat
`matlab/*.m` as a starting skeleton to adapt against your installed release, not a
verified tool. The scripting API calls are flagged inline wherever the exact
class/method name is known to vary between MATLAB releases.

This is a **separate, optional track**, not a replacement for `test_heater_ctrl.c` or
anything else in `test/`. It needs a MATLAB + Simulink + Simulink Coder/Embedded
Coder + Simulink Test license and, for PIL, the TI toolchain this repo already uses.
None of its jobs are in the branch ruleset's required checks, and none of it runs on
GitHub-hosted CI runners - see [§6](#6-ci-integration).

## 1. What this adds that Unity cannot

`test_heater_ctrl.c` answers "does the generated code behave the way I intended,"
by asserting on `heater_ctrl_Y` after calling `heater_ctrl_step()`. That is a
statement about the C. Simulink Test answers a different question: **does the
generated C behave identically to the model it came from, on the compiler and
target that will ship** - by running the same input signal through three
representations of the same design and diffing the outputs.

| Mode | What runs | What it validates |
|---|---|---|
| **Normal (MIL)** | The Simulink model itself, interpreted | The design, before code generation exists |
| **SIL** | `heater_ctrl.c` as Embedded Coder emits it, compiled for the host, driven from MATLAB | Code generation didn't change the behaviour - the same claim `test_heater_ctrl.c` makes, but derived automatically from the model instead of hand-written |
| **PIL** | The same C, cross-compiled with `armcl` and the flags in `cmake/toolchain-ti-armcl.cmake`, running *on the board*, with MATLAB relaying I/O over serial or JTAG | The real compiler and CPU didn't change the behaviour - the model-level equivalent of `docs/03-on-target.md` |

**Equivalence testing** runs one test case through two or three of these modes and
asserts the output trajectories match within tolerance. That is the thing no amount
of hand-written Unity tests can give you, because Unity tests are themselves
hand-written: they encode what you believe the correct behaviour is, not whether
generation and compilation preserved it.

**Model coverage** (decision, condition, MC/DC) is computed from these same runs,
reported per Simulink block. It is a different number from the `gcov` coverage in
`docs/02` §8, which is per line of C; keep both if you use this track, and do not
conflate their percentages.

## 2. Prerequisites

| What | Notes |
|---|---|
| MATLAB, Simulink, Simulink Coder or Embedded Coder | to build and generate code from the model |
| Simulink Test | Test Manager, MIL/SIL/PIL execution, equivalence checking |
| Simulink Coverage | optional, for model coverage reporting |
| A real `.slx` model | **this repo does not have one.** `src/gen/heater_ctrl_ert_rtw/` is hand-written C laid out like Embedded Coder output (see `docs/02` §5) - there is no `heater_ctrl.slx` to open. Build one with matching I/O and parameters (§3) before any of this runs |
| TI ARM CGT, for PIL only | already covered by `docs/03-on-target.md` and `tools/ci/install-ti-cgt.sh` |
| A self-hosted CI runner, for automated runs | see §6 |

## 3. Building a model that matches the existing example

To make MIL/SIL/PIL equivalence testing meaningful *and* to reuse the scenarios
already validated in `test_heater_ctrl.c`, give the model the same interface and
default parameters `src/gen/heater_ctrl_ert_rtw/` documents:

| Port / parameter | Type | Value |
|---|---|---|
| Inport `temp_degC` | `single` (`real32_T`) | - |
| Inport `enable` | `boolean` | - |
| Outport `heater_cmd` | `boolean` | - |
| Outport `fault` | `boolean` | - |
| `Setpoint_degC` | tunable, `single` | 40.0 |
| `Hysteresis_degC` | tunable, `single` | 2.0 |
| `OverTemp_degC` | tunable, `single` | 60.0 |
| `FaultDebounce_steps` | tunable, `uint8` | 3 |

Configuration that matters for equivalence, from `docs/02` §5.2 - restated here
because getting it wrong makes SIL/PIL silently pass on the wrong code: System
target file `ert.tlc`; Code interface packaging *Nonreusable function*; Hardware
Implementation → Device *ARM Compatible / ARM Cortex-R*; Default parameter behavior
*Tunable*. If you generate from this model, its output can replace
`src/gen/heater_ctrl_ert_rtw/` entirely - the folder convention in `docs/02` §5.1 is
what a real Embedded Coder output directory looks like.

## 4. Authoring test cases

Build the first test file in the Test Manager GUI (**Simulink → Test → Test
Manager**), not by hand-writing a `.mldatx` - it is a packaged archive, not a text
format, and the GUI is where Test Sequence blocks and signal editors are easiest to
get right. Once a test file exists, `matlab/run_tests_ci.m` runs it headlessly for
CI.

`matlab/create_heater_ctrl_tests.m` is a scripted alternative for the small number of
scenarios that only need simple time/value input signals - it mirrors these
`test_heater_ctrl.c` cases directly:

| `test_heater_ctrl.c` | Test Manager equivalent |
|---|---|
| `test_heater_turns_on_at_lower_threshold` | step `temp_degC` to 39.0, assert `heater_cmd` true |
| `test_heater_turns_off_at_upper_threshold` | ramp through the band, assert the off transition at 41.0 |
| `test_overtemp_faults_only_after_debounce` | hold `temp_degC` at 61.0 for `FaultDebounce_steps` samples, assert `fault` only after the last one |
| `test_overtemp_counter_saturates_without_wrapping` | hold `temp_degC` at 61.0 for 300 samples (`uint8` counter, must not wrap) |

Run each as a **baseline test** first (record the model's own output as the expected
result), then change its execution mode to SIL and re-run as an **equivalence test**
against that baseline. That is the whole mechanism; nothing about the test case
changes between modes.

## 5. Running the modes

- **MIL** needs nothing beyond Simulink itself.
- **SIL** needs the model built once (`slbuild`), then the test case's System Under
  Test panel (or, scripted, the property flagged in `create_heater_ctrl_tests.m`) set
  to Software-in-the-Loop.
- **PIL** additionally needs a target connectivity configuration pointing at
  `cmake/toolchain-ti-armcl.cmake`'s flags and a board reachable from the machine
  running MATLAB - not from a GitHub Actions runner. This is almost certainly a
  manual, on-your-bench step rather than something to automate first.

## 6. CI integration

`.github/workflows/simulink-test.yml` exists but triggers on `workflow_dispatch`
only - it is not wired into `push`/`pull_request` and is not one of the ruleset's
required checks (`.github/rulesets/main-branch-protection.json`). Two reasons:

- **No GitHub-hosted runner has MATLAB or a Simulink Test license.** The job needs
  `runs-on: [self-hosted, matlab]` - a machine you register yourself, with a licensed
  MATLAB install and (for PIL) the board attached.
- **Nothing here has been verified**, per the top of this document. Wiring an
  unverified job into required checks would risk blocking every merge on a job that
  might not even run.

Once you have a self-hosted runner and have confirmed `matlab/run_tests_ci.m` works
against your MATLAB release: change the trigger to include `push`/`pull_request`,
and only then consider adding its job name to the ruleset - the same "job name must
match exactly" caveat from `.github/rulesets/README.md` applies.

The runner script exports JUnit-style XML (`sltest.testmanager.resultsToJUnitFormat`,
documented by MathWorks specifically for CI integration) plus a coverage report, and
sets MATLAB's exit code from the pass/fail result so the job fails correctly.

## 7. Where this fits with the rest of the repo

| Doc | What it covers |
|---|---|
| `docs/01-approach-and-options.md` | why Unity+CMock was the first-pass choice; Simulink Test listed as complementary |
| `docs/02-adopting-in-your-project.md` §5.6 | the boundary: Simulink Test for the model, Unity for everything the model can't see |
| `docs/03-on-target.md` | the non-Simulink way to prove the same compiler/CPU claim PIL makes, using only `armcl` and CTest |
| this document | the Simulink Test track itself |

`test_heater_task.c` (the hand-written glue between `temp_monitor`/`gio_hal` and the
model) is *not* something Simulink Test can see, licensed or not - it contains no
generated code. It stays a Unity/CMock test regardless of whether this track is ever
built out.
