# Stand-in for a HALCoGen project (compiles and links, does not run)

The smallest thing that satisfies `target/CMakeLists.txt` in place of real HALCoGen
output, so the cross-build can be exercised without HALCoGen (Windows-only):

| File | What it is | What it is not |
|---|---|---|
| `include/sys_common.h` | the HALCoGen integer typedefs (`uint8`, `uint32`, …) | |
| `include/sci.h`, `source/sci.c` | the SCI API Unity's output hooks call, as **no-ops** | a UART driver: nothing is configured, nothing is transmitted |
| `source/sys_link.cmd` | the TMS570LS3137 memory map and section placement HALCoGen 4.07 generates | |
| `source/sys_intvecs.asm` | a vector table whose reset entry branches to the TI RTS `_c_int00` | HALCoGen start-up: no PLL, no RAM/ECC init, no VFP enable, no mode stacks |
| `source/sys_main.c` | a `main()` that would collide with the Unity runner's | included by mistake: it proves the build excludes `sys_main.c` |

Used by two presets:

- `target-dryrun` - stand-in `armcl`/`armar` scripts, nothing is compiled.
- `target-ci` - the **real** TI compiler (installed by `tools/ci/install-ti-cgt.sh`)
  compiles and links every test binary against this stub. That checks the whole tree
  under `armcl` (big-endian, ILP32, `--strict_ansi`, packed enums) on every commit.
  The resulting `.out` files must not be flashed: they would start, print nothing and
  fault on the first float instruction.

For a board, generate the real thing: [docs/03-on-target.md](../../docs/03-on-target.md).
