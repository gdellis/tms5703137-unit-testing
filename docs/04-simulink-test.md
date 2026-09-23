# Model-based testing: MIL, SIL and PIL

**Status: reference, not built or run.** Everything in this document was written without
access to MATLAB - this sandbox has none - so nothing here has been executed. Treat
`matlab/*.m` as a starting skeleton to adapt against your installed release, not a
verified tool. Scripting API calls are flagged inline wherever the exact class or method
name is known to vary between MATLAB releases.

This is a **separate, optional track**, not a replacement for `test_heater_ctrl.c` or
anything else in `test/`. It needs MATLAB + Simulink + Simulink Coder/Embedded Coder +
Simulink Test licences and, for PIL, the TI toolchain this repo already uses. None of its
jobs are in the branch ruleset's required checks, and none of it runs on GitHub-hosted CI
runners - see [§10](#10-ci-integration). Hardware-in-the-loop is a different discipline
again and has its own guide: [09-hil-testing.md](09-hil-testing.md).

Sections [5](#5-mil---model-in-the-loop), [6](#6-sil---software-in-the-loop) and
[7](#7-pil---processor-in-the-loop) each walk one stage end to end - what it proves, what
you need, setting it up, authoring, running, asserting, triage and cadence - so you can
work from a single section without reading the other two.

## 1. What this adds that Unity cannot

`test_heater_ctrl.c` answers "does the generated code behave the way I intended," by
asserting on `heater_ctrl_Y` after calling `heater_ctrl_step()`. That is a statement
about the C. Simulink Test answers a different question: **does the generated C behave
identically to the model it came from, on the compiler and target that will ship** - by
running the same input signal through several representations of the same design and
diffing the outputs.

**Trajectory equivalence** (also called back-to-back testing) runs one test case through
two or three stages and asserts the output trajectories match within tolerance. That is
the thing no amount of hand-written Unity tests can give you, because Unity tests are
themselves hand-written: they encode what you believe the correct behaviour is, not
whether generation and compilation preserved it.

> Not to be confused with *equivalence partitioning*, the test-design technique in
> [08-test-design-and-strategy.md](08-test-design-and-strategy.md) §2. Same word, unrelated
> meaning: that one is about choosing input classes, this one about comparing trajectories.

## 2. The stages are not a maturity ladder

They are usually drawn MIL → SIL → PIL → HIL, which suggests each supersedes the last.
They do not. Each answers a different question, and the middle three are the *same test*
executed in different places.

| Stage | What executes | The question it answers | Blind to |
|---|---|---|---|
| **MIL** | the model itself, interpreted in Simulink | Is the **design** right? | code generation, compiler, CPU, I/O |
| **SIL** | generated C, compiled for the host, driven from MATLAB | Did **code generation** preserve the model's behaviour? | compiler, CPU, endianness, timing |
| **PIL** | the same C, cross-compiled with `armcl` and run **on the board**, MATLAB relaying I/O over serial or JTAG | Did the **real compiler and silicon** preserve it? | real I/O, real timing, the plant |
| **HIL** | complete firmware on the ECU, plant simulated in real time | Does the **system** work - I/O, timing, faults, networks? | see [09-hil-testing.md](09-hil-testing.md) |

Two consequences worth stating plainly:

- **You do not have to run all of them.** Each earns its place separately. A project
  with no safety argument and a well-understood compiler may run MIL and stop.
- **Skipping SIL does not make PIL cover it.** PIL failures are harder to diagnose,
  because codegen *and* compiler *and* CPU all changed at once. SIL exists to remove one
  variable.

## 3. Vectors are the asset, not the harness

The expensive artefact is not the test harness - it is the set of input trajectories and
their expected outputs. Author it once and replay it at every stage.

- Keep stimulus in a form every stage can consume: a Signal Editor scenario, a
  `Simulink.SimulationData.Dataset`, or a spreadsheet of time/value pairs.
- Nothing about a test case should change when you switch stages. If it does, you are
  maintaining several suites that will drift apart, and the diff between stages stops
  meaning anything.
- Name cases after the requirement, not the stage. `overtemp_faults_after_debounce` is
  runnable at MIL, SIL and PIL; `sil_test_3` is not.

This is why the baseline-then-promote workflow in §5 and §6 matters: the SIL run reuses
the MIL case unchanged, and that reuse is the entire mechanism.

## 4. Two coverage numbers, and they are not interchangeable

| Measure | Comes from | Reports |
|---|---|---|
| **Model coverage** - decision, condition, MC/DC | a MIL run with Simulink Coverage | per Simulink block: which decisions and conditions were exercised |
| **Code coverage** - line, branch | a SIL or PIL run, or the `gcov` setup in [02-adopting-in-your-project.md](02-adopting-in-your-project.md) §8 | per line of C |

Keep both if you run this track, and never quote one as the other. Two rules follow:

- **Structural coverage for a safety argument must come from the code**, not the model.
  ISO 26262 and IEC 61508 assess the thing that ships.
- **Unreachable generated code is justified, not excluded.** Embedded Coder emits
  defensive branches that the model cannot reach; the answer is a written rationale per
  site, not a filter that hides them.
  [08-test-design-and-strategy.md](08-test-design-and-strategy.md) §4.2 covers why the
  percentage is a floor rather than a target.

## 5. MIL - model-in-the-loop

### What it proves

That the **design** is right: the model meets its requirements. It is the only stage you
can run before any code exists, which makes it the cheapest place to find a requirements
error.

It cannot tell you anything about generated code, the compiler or the target - at this
stage none of them exist yet.

### What you need

| What | Why |
|---|---|
| MATLAB + Simulink | to build and simulate the model |
| Simulink Test | Test Manager, test sequences, assessments |
| Simulink Coverage | optional; model coverage metrics |
| Simulink Requirements | optional; links cases to requirement IDs |
| A real `.slx` model | **this repo does not have one.** `src/gen/heater_ctrl_ert_rtw/` is hand-written C laid out like Embedded Coder output, so there is no `heater_ctrl.slx` to open |

### Setting it up

Build a model whose interface matches what the rest of this repo assumes, so the same
scenarios are reusable at every stage and against the existing Unity tests:

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

Set the configuration that matters for the later stages **now**, because changing it later
changes the generated interface and every test with it:

| Configuration parameter | Set to | Why |
|---|---|---|
| System target file | `ert.tlc` | bare-metal output, no `rtModel` scheduler baggage |
| Code interface packaging | *Nonreusable function* | globals `<m>_U`, `<m>_Y`, `<m>_P`, `<m>_DW` and entry point `<m>_step()`, which is what this repo assumes. *Reusable function* passes an `RT_MODEL_<m>_T*` instead, and tests then own an instance of each |
| Generate an example main program | off | `ert_main.c` would define a second `main()` next to Unity's runner |
| Default parameter behavior | *Tunable* | emits `<m>_P` so tests can move set-points, thresholds and debounce counts. *Inlined* bakes them in as constants |
| Remove internal data zero initialization | (default on) | `<m>_initialize()` then does **not** clear states and I/O; it relies on C start-up. Tests must `memset` `<m>_DW`, `<m>_U`, `<m>_Y` themselves |
| Hardware Implementation → Device | ARM Compatible / ARM Cortex-R | fixes `rtwtypes.h`: `int32_T` = `int`, big-endian, no `long long` unless the TI compiler is told to support it |

Getting the last two wrong is the classic way to make SIL and PIL pass against the wrong
code, so confirm them before going further.

### Authoring the test

Build the first test file in the Test Manager GUI - **Simulink → Test → Test Manager** -
not by hand-writing a `.mldatx`. It is a packaged archive, not a text format, and the
GUI is where signal editors and Test Sequence blocks are easiest to get right.

For each case:

1. **New Test Case → Baseline Test**, and set its System Under Test to the model.
2. Provide the stimulus, by whichever suits the scenario: **Inputs** pane with a
   `Simulink.SimulationData.Dataset`, the **Signal Editor**, or a **Test Sequence**
   block for anything with steps and conditions.
3. Add **Logical and Temporal Assessments** for the requirement being checked - these
   survive a later loosening of equivalence tolerance, which a bare baseline does not.
4. If you have Simulink Requirements, link the case to its requirement ID now.
   Retro-fitting traceability across four stages is far more work than adding it here.

`matlab/create_heater_ctrl_tests.m` is a scripted alternative for cases that only need
simple time/value stimulus. It mirrors four `test_heater_ctrl.c` scenarios directly:

| `test_heater_ctrl.c` | Test Manager equivalent |
|---|---|
| `test_heater_turns_on_at_lower_threshold` | step `temp_degC` to 39.0, assert `heater_cmd` true |
| `test_heater_turns_off_at_upper_threshold` | ramp through the band, assert the off transition at 41.0 |
| `test_overtemp_faults_only_after_debounce` | hold `temp_degC` at 61.0 for `FaultDebounce_steps` samples, assert `fault` only after the last |
| `test_overtemp_counter_saturates_without_wrapping` | hold at 61.0 for 300 samples (`uint8` counter, must not wrap) |

### Running it

- In the GUI: select the test file and press **Run**. Execution mode stays **Normal**.
- Headless: `matlab -batch "run_tests_ci('heater_ctrl_tests.mldatx')"`, which
  `matlab/run_tests_ci.m` sketches.
- For coverage, enable it per test file under **Coverage Settings** before the run.

### What to assert

- **Baseline**: the first Normal-mode run records the model's own output as the expected
  result. That is the artefact SIL and PIL later compare against.
- **Assessments**: assert the requirement explicitly as well - "fault rises within three
  samples of the threshold being exceeded". A baseline alone records *what the model did*,
  including behaviour you did not intend.
- **Tolerance** is not yet interesting here; you are comparing a run against itself.

### When it fails

A MIL failure is a **design or requirement** problem, in this order:

1. The requirement is ambiguous, and the assessment encodes a different reading than the
   model.
2. The stimulus does not reach the condition you think it does - check the signal, not
   the logic.
3. The model is genuinely wrong.

Nothing here is ever a code-generation problem, which is exactly what makes MIL cheap to
debug: only one thing exists to be wrong.

### Cadence

Every model save, and in any CI that has a MATLAB licence available. It is the fastest
of the four stages and the only one that can run before code generation.

## 6. SIL - software-in-the-loop

### What it proves

That **code generation preserved the model's behaviour**. The generated C is compiled
for the host and driven from MATLAB with the same stimulus as MIL, and the trajectories
are diffed.

It cannot see anything the host compiler and x86 hide: endianness, target word size, TI
compiler behaviour, or real timing. On this target that exclusion is significant - see §7.

### What you need

MATLAB + Simulink + Embedded Coder + Simulink Test, a host C compiler that MATLAB can
drive (`mex -setup`), and MIL baselines already recorded. No hardware.

### Setting it up

1. Generate and build the code once: `slbuild('heater_ctrl')`, or **Ctrl-B** in the model.
2. Confirm the generated interface matches §5's configuration table - nonreusable
   function, tunable parameters, `ert.tlc`. If it does not, fix the model config and
   regenerate before going further; a SIL run against the wrong interface can still
   pass.
3. Take an existing MIL baseline case and change its **System Under Test → Simulation
   Mode** to **Software-in-the-Loop (SIL)**. Scripted, this is the property flagged
   `VERIFY` in `matlab/create_heater_ctrl_tests.m`.
4. Set the case's comparison to the MIL baseline, making it an **equivalence test**.

### Authoring the test

Nothing new to author - that is the point. The case is the MIL case; only the execution
mode changed. If you find yourself writing a different stimulus for SIL, something is
wrong with how the vectors were stored (§3).

Where a genuinely SIL-only check is warranted, it is usually a data-type or overflow
concern that the model's idealised arithmetic hides.

### Running it

- GUI: **Run** on the same test file. The Test Manager reports both runs and their diff.
- Headless: identical to MIL - `matlab -batch "run_tests_ci(...)"`.
- Results appear as per-signal comparison plots; read those before the pass/fail verdict,
  because a tolerance that is too loose passes while hiding a real divergence.

### What to assert

This is where the **tolerance policy** lives, and it needs to be a deliberate decision:

| Design | Assert | Why |
|---|---|---|
| **Fixed-point** | bit-exact, zero tolerance | the arithmetic is fully specified; any difference is a defect, not noise |
| **Floating point** | explicit per-signal tolerance, derived from the requirement | rounding differs legitimately between hosts and targets |

Write the tolerance down with its justification next to the signal. A blanket relative
`1e-6` across a whole model is the single most common way for a real divergence to sit
undetected for months: it is loose enough to absorb a genuine defect in a slow-moving
signal and tight enough to produce noise failures in a fast one.

### When it fails

Triage in this order - the first two account for most cases:

1. **Tolerance too tight.** Check the comparison plot: a difference at the floating-point
   noise floor that does not grow is a tolerance problem, not a codegen problem.
2. **Data type or storage class differs** between model and generated code - a signal
   that is `double` in the model and `single` in the code will diverge slowly and
   convincingly.
3. **Sample time or solver mismatch.** A variable-step MIL run compared against
   fixed-step generated code will not line up; the model must be fixed-step for this
   comparison to mean anything.
4. **A genuine code-generation defect.** Rare, and worth a MathWorks support case with the
   model attached - but only after the three above are ruled out.

### Cadence

Per commit. SIL needs no hardware, so it is the one stage of this track that is genuinely
CI-able wherever a MATLAB licence is reachable.

## 7. PIL - processor-in-the-loop

### What it proves

That the **real compiler and the real silicon** preserved the behaviour. The same
generated C is cross-compiled with `armcl` and executed on the TMS570, with MATLAB
relaying inputs and outputs over serial or JTAG and comparing against the MIL baseline.

This is the only stage in this document that can see target-representation defects. It
still cannot see real I/O, real timing under load, or the plant - that is
[09-hil-testing.md](09-hil-testing.md).

**On this target PIL is not optional in the way it is elsewhere.** The TMS570 is
big-endian BE-32 with an ILP32 data model; the host is little-endian LP64. Union
punning, byte-packed comms frames and checksums over raw memory can pass SIL for the
wrong reason and fail here. The full difference table is in
[03-on-target.md](03-on-target.md) §"What a target run catches".

### What you need

Everything SIL needs, plus:

- TI ARM CGT 20.2.x LTS (`armcl`, `armar`) - the same compiler
  `cmake/toolchain-ti-armcl.cmake` and `tools/ci/install-ti-cgt.sh` already use.
- A TMS570LS3137 board attached to **the machine running MATLAB**, not to a CI runner.
- A debug probe or serial link MATLAB can drive.

### Setting it up

1. Create a **target connectivity configuration** - a
   `rtw.connectivity.ConfigRegistry` subclass telling Simulink how to build, download and
   communicate with the target.
2. Make its build settings match `cmake/toolchain-ti-armcl.cmake`: `-mv7R4
   --code_state=32 --float_support=VFPv3D16 --abi=eabi --endian=big --enum_type=packed`.
   If the PIL build and the firmware build disagree on any of these, the comparison is
   meaningless - endianness and `--enum_type` especially.
3. Register the configuration on the MATLAB path, and confirm the board is reachable
   independently (load and run something trivial) before involving Simulink.
4. Switch the test case's **Simulation Mode** to **Processor-in-the-Loop (PIL)**.

Expect this step to take a day the first time. It is a bench activity, not a CI activity.

### Authoring the test

Again, nothing new: the MIL case, run in a third mode. Select a *subset* of cases for PIL
rather than the whole suite - each sample period costs a round trip over the link, so a
300-sample saturation test that runs in milliseconds at SIL can take minutes here.

Choose the cases where target representation could plausibly matter: anything with byte
packing, type conversion, saturation or arithmetic near a type limit.

### Running it

- GUI: **Run**, with the board powered and connected. The first run also cross-compiles.
- Watch for a run that passes suspiciously fast - that usually means the target binary did
  not actually start and the harness compared nothing.
- Headless runs work but are rarely worth automating until the bench process is reliable.

### What to assert

The same tolerance policy as SIL (§6), with one addition: **a difference that appears
only at PIL is a target-representation finding until proven otherwise.** Do not loosen
the tolerance to make it pass; that is exactly the defect class this stage exists to
catch.

### When it fails

Assuming the same case passes at SIL, triage in this order:

1. **Endianness.** Anything that reinterprets memory - unions, `memcpy` into a typed
   buffer, comms frame packing, checksums over raw bytes.
2. **Word size.** `long` is 64-bit on the host and 32-bit on the target; `ulong_T` in
   generated code is the usual culprit. The `host-m32` preset catches much of this earlier
   and far more cheaply.
3. **Compiler optimisation.** Try `-O0` on the target build: if the difference disappears,
   suspect undefined behaviour in the generated or custom code rather than a compiler bug.
4. **FPU rounding.** VFPv3-D16 is single-precision fast, double emulated; rounding and
   any FMA contraction differ from x86. A last-bit difference in a float signal is
   expected and belongs in the tolerance, not in a defect report.

### Cadence

Per model change, or nightly if a board is permanently attached to a build machine.
Never in the per-commit path - it is slow, needs hardware, and a flaky serial link would
block merges.

## 8. Cadence and cost, all four stages

| Stage | Needs | Run it | Typical duration |
|---|---|---|---|
| MIL | MATLAB + Simulink Test | every model save; CI with a licence | seconds |
| SIL | + Embedded Coder, host compiler | every commit | seconds to minutes |
| PIL | + TI CGT, board on the MATLAB machine | per model change, or nightly | minutes to hours |
| HIL | rig, real-time plant, I/O | per integration milestone - [09](09-hil-testing.md) | hours, plus scheduling |

The gradient is steep and it is mostly about *access*, not runtime: SIL is cheap because
nothing physical is involved, PIL is expensive because one board serialises everybody.

## 9. No Simulink Test licence? The nearest equivalents

Most of what the middle two stages prove can be approximated with what this repo already
ships. You lose automatic derivation from the model - the expected values become
hand-written, so a code-generation defect that matches your misunderstanding survives -
but the compiler and CPU claims hold fully.

| Instead of | Do this | What you keep | What you lose |
|---|---|---|---|
| **SIL** | Link the generated model library into a host Unity test and drive `_U` / `_step()` / `_Y` - pattern 3, `test/test_heater_ctrl.c` | the shipped object code is what you test, per-commit and free | expectations are hand-written, so codegen bugs that match your assumption are invisible |
| **PIL** | `cmake --preset target` cross-compiles the same tests with `armcl`; `ctest --preset target` flashes and reads the verdict over SCI - [03-on-target.md](03-on-target.md) | the real compiler, endianness, word size and CPU | no automatic model comparison; you assert requirements, not equivalence |
| **MIL** | No equivalent. Design validation needs the model | - | - |

Concretely, the SIL-substitute workflow is:

1. Copy Embedded Coder's `<model>_ert_rtw/` output into `src/gen/` verbatim.
2. Add the one hand-maintained `CMakeLists.txt` that builds it as a static library, with
   its own warning policy - report, never `-Werror`.
3. `add_unity_test(test_<model> LIBS <model>_model)` - no `SOURCES`, no mocks, so the test
   links the library *as shipped* rather than recompiling it.
4. In `setUp()`, `memset` `_DW`, `_U` and `_Y`, and save `_P` for restoration in
   `tearDown()`.

[02-adopting-in-your-project.md](02-adopting-in-your-project.md) §5 has this in full.

## 10. CI integration

`.github/workflows/simulink-test.yml` exists but triggers on `workflow_dispatch` only -
it is not wired into `push`/`pull_request` and is not one of the ruleset's required
checks (`.github/rulesets/main-branch-protection.json`). Two reasons:

- **No GitHub-hosted runner has MATLAB or a Simulink Test licence.** The job needs
  `runs-on: [self-hosted, matlab]` - a machine you register yourself, with a licensed
  MATLAB install and, for PIL, the board attached.
- **Nothing here has been verified**, per the banner at the top. Wiring an unverified
  job into required checks would risk blocking every merge on a job that might not even
  run.

Once you have a self-hosted runner and have confirmed `matlab/run_tests_ci.m` works
against your release: add MIL and SIL to `push`/`pull_request`, leave PIL on
`workflow_dispatch` or a nightly schedule, and only then consider adding the job name to
the ruleset - the "job name must match exactly" caveat in `.github/rulesets/README.md`
applies.

The runner script exports JUnit-style XML (`sltest.testmanager.resultsToJUnitFormat`,
documented by MathWorks for CI integration) plus a report, and sets MATLAB's exit code
from the pass/fail result so the job fails correctly.

## 11. Where this fits with the rest of the repo

| Doc | What it covers |
|---|---|
| [01-approach-and-options.md](01-approach-and-options.md) | why Unity + CMock was the first-pass choice; this track listed as complementary |
| [02-adopting-in-your-project.md](02-adopting-in-your-project.md) §5.6 | the boundary: Simulink Test for the model, Unity for everything the model cannot see |
| [03-on-target.md](03-on-target.md) | the non-Simulink way to prove the same compiler/CPU claim PIL makes, using only `armcl` and CTest |
| [05-choosing-a-method.md](05-choosing-a-method.md) §4 | when this track earns its licence cost, and what equivalence catches that a hand-written test cannot |
| [06-new-project-setup.md](06-new-project-setup.md) track E | where this sits in setting a project up from scratch |
| [09-hil-testing.md](09-hil-testing.md) | the fourth stage: real I/O, real time, fault injection |
| this document | MIL, SIL and PIL themselves |

`test_heater_task.c` - the hand-written glue between `temp_monitor`/`gio_hal` and the
model - is *not* something Simulink Test can see, licensed or not: it contains no
generated code. It stays a Unity/CMock test regardless of whether this track is ever
built out.
