# Running the same tests on the TMS570LS3137

The host suite is the per-commit gate. This page is the once-per-release (or
once-per-board-in-CI) run of the **same test binaries** cross-compiled with the TI
ARM compiler and executed on the real Cortex-R4F, with Unity's output coming back
over an SCI (UART).

What a target run catches that the host cannot:

| Difference | Host | TMS570LS3137 |
|---|---|---|
| Compiler | gcc / clang | TI `armcl` 20.2.x LTS: its own diagnostics, optimiser, intrinsics |
| Endianness | little | **big (BE-32)**: unions, byte-packing, checksums over raw memory |
| Data model | LP64 | ILP32: `long` and pointers are 32-bit |
| `sizeof(enum)` | 4 | 1/2/4 (`--enum_type=packed`, the HALCoGen/CCS default) |
| Floating point | x87/SSE | VFPv3-D16, single precision fast, double emulated |

Nothing in this page is needed to *write* tests. It is a second way to *run* them.

## 1. Prerequisites

| What | Where | Notes |
|---|---|---|
| TI ARM Code Generation Tools 20.2.x LTS (`armcl`, `armar`) | ships with [CCS 12.8](https://www.ti.com/tool/CCSTUDIO) under `ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS` | set `TI_CGT_ARM_ROOT` to that directory |
| HALCoGen 04.07.01 | [ti.com/tool/HALCOGEN](https://www.ti.com/tool/HALCOGEN) | Windows only; generates the start-up code, linker command file and SCI driver |
| A way to flash and start the board | UniFlash (`dslite`) or CCS (Debug Server Scripting) | see section 4 |
| Serial link to the board's SCI | USB-UART on the HDK, or any 3.3 V adapter on the SCI TX pin | 115200 8N1 |
| Ruby, CMake ≥ 3.21, Ninja, Python 3 + pyserial | as for the host build | Ruby still generates mocks/runners; Python captures the output |

## 2. Generate the HALCoGen project

Create a HALCoGen project for **TMS570LS3137ZWT** and generate it into
`target/halcogen/` (so that `target/halcogen/include/` and `target/halcogen/source/`
exist). That directory is git-ignored except for its README.

Settings that matter:

- **Driver Enable:** tick only *SCI* (or *SCILIN*). Everything else off keeps the
  binary small and the start-up short.
- **SCI:** 115200 baud, 8 data bits, no parity, 1 stop bit, TX enabled. The baud
  divider HALCoGen writes depends on the VCLK you configured; leave the PLL/clock tree
  at HALCoGen's defaults for the HDK unless your board differs.
- **PINMUX:** route the SCI TX pin you actually have wired. On the TMS570LS31x HDK the
  on-board USB serial port is on SCI1, which HALCoGen calls `scilinREG`; that is the
  default `TMS570_UNITY_SCI`. Pass `-DTMS570_UNITY_SCI=sciREG` for SCI2.
- Leave **VFP enable**, **RAM initialisation (`_memInit_`)** and the default
  `sys_link.cmd` alone. The tests use `float`, and reading un-initialised ECC RAM is
  a data abort.
- `sys_main.c` is generated but ignored by the build: each Unity runner brings its own
  `main()`.

If the real firmware project already has a HALCoGen directory, point at it instead:
`-DHALCOGEN_DIR=<path>`. Extra drivers there do no harm as long as nothing in the tests
enables them.

## 3. Build

```sh
export TI_CGT_ARM_ROOT=/opt/ti/ccs1280/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS   # or set in the shell/CI
cmake --preset target
cmake --build --preset target
```

Result: one `build/target/test/test_<module>.out` per test file, plus a `.map` next to
the build root. Same test sources, same mocks, same generated runners as the host
build; only the compiler, the CPU and the output channel differ.

How it is wired:

- `cmake/toolchain-ti-armcl.cmake` picks `armcl`/`armar` and the CPU flags
  (`-mv7R4 --code_state=32 --float_support=VFPv3D16 --abi=eabi --endian=big`,
  `--enum_type=packed`). CMake's own TI compiler support provides everything else
  (`--compile_only`, `--run_linker`, `--include_path=`, `--c11`, depfiles). It also
  puts `$TI_CGT_ARM_ROOT/include` and `/lib` on the search paths: armcl locates its
  own `stddef.h`, `setjmp.h` and run-time libraries through `C_DIR`/`TI_ARM_C_DIR`,
  which CCS sets and a plain shell or CI runner does not.
- `target/CMakeLists.txt` compiles the HALCoGen sources (`C_EXTENSIONS ON` →
  `--relaxed_ansi`; the rest of the tree is `--strict_ansi`) as an **object** library,
  so the vector table and start-up code are always linked even though no symbol
  references them, and adds `sys_link.cmd` to every link.
- `target/unity_config.h` + `target/unity_target_io.c` tell Unity the integer widths
  and route `UNITY_OUTPUT_CHAR` to `sciSendByte()`. `UNITY_OUTPUT_COMPLETE` parks the
  core in an endless loop after the summary, with `unity_target_finished` set to 1.
- `add_unity_test()` links the `tms570_target_runtime` interface target whenever it
  exists, so `test/CMakeLists.txt` is untouched by the port.

## 4. Run

### Manually

Load a `.out` in CCS (Debug → Load Program) and open a terminal on the board's serial
port at 115200 8N1 before pressing Run. You get the same text the host prints:

```
test/test_heater_ctrl.c:79:test_initialize_leaves_outputs_off:PASS
...
-----------------------
17 Tests 0 Failures 0 Ignored
OK
```

### Automatically, through CTest

`ctest --preset target` runs every binary through `tools/run_on_target.sh`, which
CMake registers as the *cross-compiling emulator*. The script opens the serial port,
runs the flash command you give it, relays Unity's output, and turns the final
`OK`/`FAIL` into the test's exit status.

```sh
export TMS570_SERIAL_PORT=/dev/ttyUSB0            # COM5 on Windows works too (pyserial)
export TMS570_FLASH_CMD='dslite.sh --mode flash --config=tools/tms570ls3137.ccxml -e -f -v {image}'
ctest --preset target
```

`{image}` is replaced by the path of each `.out`. The flash command must leave the
board *running*:

- **UniFlash:** the `dslite` syntax above is the modern one (`--mode flash --config=…`);
  older versions use `dslite flash -c … -e -f -v <file>`. The reliable way to get the
  right line is UniFlash → *Standalone Command Line* → *Generate Package*, which emits a
  `dslite` script for your version. UniFlash resets and releases the core after
  programming.
- **CCS Debug Server Scripting:** `tools/dss_flash_and_run.js` loads the image and
  calls `runAsynch()`, e.g.
  `TMS570_FLASH_CMD='<ccs>/ccs/ccs_base/scripting/bin/dss.sh tools/dss_flash_and_run.js board.ccxml {image}'`.

`tools/unity_serial_capture.py` is usable on its own too (`--port`, `--baud`,
`--timeout`); it exits 0 for `OK`, 1 for `FAIL`, 2 on timeout.

## 5. What the tests mean on the target

- **`UNIT_TEST` is still defined.** The overlay-redirect tests (`test_adc_hal.c`,
  `test_gio_hal.c`) keep poking their RAM "registers" on the board as well. They are
  *driver-logic* tests, now compiled by the real compiler, not hardware tests. Real
  hardware-in-the-loop tests (does the ADC actually convert?) are a different category
  with different fixtures; do not try to make the same file do both.
- Mock-based tests (`test_temp_monitor.c`, `test_heater_task.c`) and the model test
  (`test_heater_ctrl.c`) are the ones that earn their keep here: pure logic, compiled
  big-endian, ILP32, packed enums, VFP.
- Unity's float asserts work because HALCoGen's `_c_int00` enables the VFP. If your
  start-up code does not, add `#define UNITY_EXCLUDE_FLOAT` to `unity_config.h`.
- Heap is 40 KB and stack 4 KB (`--heap_size=0xA000 --stack_size=0x1000` in the
  toolchain file). The heap matters: CMock `malloc`s a 32 KB pool for expectations
  (`CMOCK_MEM_SIZE`), so the CCS default of 2 KB links fine and then fails every
  mock-based test at run time. Shrink `CMOCK_MEM_SIZE` (a compile definition on the
  `cmock` target) rather than the heap if RAM is tight.
- Anything that fails only on the target is the finding you ran this for. Typical:
  a `union` used for byte access, `sizeof` of an enum baked into a frame layout,
  `long` assumed 64-bit, an `int` shift past 31.

## 6. Building for the target in CI

Two presets build against `tools/halcogen-stub/`, a stand-in HALCoGen project that
compiles and links but must never be flashed (no-op SCI, no start-up code; see its
README):

- **`target-ci` - the real compiler.** `tools/ci/install-ti-cgt.sh` downloads TI's
  Linux installer for ARM CGT 20.2.7.LTS and installs it unattended
  (`--mode unattended --prefix …`); the `Target build` CI job caches the result by
  version and exports `TI_CGT_ARM_ROOT`. The script also pre-builds the run-time
  library variant this project links (`rtsv7R4_A_be_v3D16_eabi.lib`): TI ships the
  run-time as source and the linker otherwise builds it on first use, which costs
  minutes on every clean build. Then `cmake --preset target-ci` compiles and
  links every test binary for the Cortex-R4F: big-endian, ILP32, `--c11 --strict_ansi`,
  packed enums, `--emit_warnings_as_errors` on the hand-written code. The `.out` and
  `.map` files are uploaded as a build artifact. This is the per-commit check that the
  tree is *acceptable to the TI toolchain*; whether it *behaves* on the board is
  section 4.

  ```sh
  tools/ci/install-ti-cgt.sh ~/ti/ti-cgt-arm_20.2.7.LTS    # once
  export TI_CGT_ARM_ROOT=~/ti/ti-cgt-arm_20.2.7.LTS
  cmake --preset target-ci && cmake --build --preset target-ci
  ```

- **`target-dryrun` - no compiler at all.** Stand-in `armcl`/`armar` scripts
  (`tools/dryrun/`) accept the real command lines and write empty outputs, so the
  toolchain file, `target/CMakeLists.txt`, the link lines and the CTest/emulator wiring
  are checked even on a machine that cannot download the TI tools. `ctest --preset
  target-dryrun` then "runs" each `.out` through a stub emulator that only prints what
  it would flash. It proves the plumbing, never the code. CI runs it as the
  `Target build dry run` job.

## 7. Troubleshooting

| Symptom | Look at |
|---|---|
| `armcl` not found | `TI_CGT_ARM_ROOT` unset or pointing above `bin/` |
| `HALCOGEN_DIR … is not a HALCoGen project` | generate into `target/halcogen/` (section 2) or pass `-DHALCOGEN_DIR` |
| Link error about `.intvecs`, `_c_int00`, `.stack` | `sys_link.cmd` missing from the link, or HALCoGen sources not in the object library; check `cmake --build --preset target -- -v` |
| `warning #10211-D: cannot resolve archive libc.a ... no input files have been encountered`, then `_c_int00 undefined` and `entry-point ... setting to 0` | `--library=libc.a` came *before* the object files. It is an index library and must be last: keep it in `CMAKE_C_STANDARD_LIBRARIES`, not in the linker flags. The link still "succeeds" with no run-time at all, so check the map, not the exit code |
| Nothing on the serial port | wrong SCI instance (`TMS570_UNITY_SCI`), TX pin not muxed, baud vs. VCLK mismatch, adapter on RX instead of TX |
| Output stops mid-run, `unity_target_finished` still 0 | a data abort (ESM, ECC, unaligned access): connect CCS and look at the `_dabort` handler |
| Garbage characters | baud rate; HALCoGen computes the divider from the configured VCLK |
| `unity_serial_capture: no Unity summary` | board never reached `UnityEnd()`: see the two rows above |
| Flash command returns before the board runs | use the DSS script, or the post-flash *run* option of your UniFlash version |
