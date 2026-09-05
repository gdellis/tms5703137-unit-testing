# HALCoGen stub for the dry-run preset

Empty files with the names HALCoGen generates, just enough for
`target/CMakeLists.txt` to glob sources, find `sys_link.cmd`, and exclude
`sys_main.c`. Nothing here is compiled: the dry-run `armcl` only creates empty
outputs. Do not use this with a real toolchain.
