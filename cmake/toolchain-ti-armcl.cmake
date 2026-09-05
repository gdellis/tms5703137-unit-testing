# CMake toolchain file: TI ARM Code Generation Tools (armcl) targeting the TMS570LS3137.
#
#   cmake --preset target        # selects this file via "toolchainFile"
#
# Needs TI_CGT_ARM_ROOT (cache variable or environment variable) pointing at the
# compiler root, e.g.
#   C:/ti/ccs1280/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS
#   /opt/ti/ccs1280/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS
# If it is unset, armcl/armar must be on PATH.
#
# CMake has native support for TI compilers (Modules/Compiler/TI*.cmake): it already
# knows --compile_only / --run_linker, --include_path=, --library=, --search_path=,
# --c11, depfiles and the archiver. This file only picks the tools and the
# CPU/ABI flags, which match what HALCoGen puts in its CCS project for TMS570LS31x.

set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

if(NOT TI_CGT_ARM_ROOT AND DEFINED ENV{TI_CGT_ARM_ROOT})
    set(TI_CGT_ARM_ROOT "$ENV{TI_CGT_ARM_ROOT}")
endif()
set(TI_CGT_ARM_ROOT "${TI_CGT_ARM_ROOT}" CACHE PATH
    "TI ARM CGT root directory (contains bin/, lib/, include/)")

# find_program() keeps a value that is already in the cache, which is how the
# target-dryrun preset substitutes its stand-in tools.
find_program(CMAKE_C_COMPILER   armcl HINTS "${TI_CGT_ARM_ROOT}/bin" REQUIRED)
find_program(CMAKE_ASM_COMPILER armcl HINTS "${TI_CGT_ARM_ROOT}/bin" REQUIRED)
find_program(CMAKE_AR           armar HINTS "${TI_CGT_ARM_ROOT}/bin" REQUIRED)

# Cortex-R4F, ARM (32-bit) instruction state, VFPv3-D16, EABI, big-endian (BE-32).
set(TMS570_CPU_FLAGS "-mv7R4 --code_state=32 --float_support=VFPv3D16 --abi=eabi --endian=big")

# --enum_type=packed is the HALCoGen/CCS default for this device. It makes enums as
# small as their values allow, so sizeof(enum) differs from the host - one of the
# things an on-target run is there to catch.
set(CMAKE_C_FLAGS_INIT
    "${TMS570_CPU_FLAGS} --enum_type=packed --diag_warning=225 --diag_wrap=off --display_error_number")
set(CMAKE_ASM_FLAGS_INIT
    "${TMS570_CPU_FLAGS} --diag_wrap=off --display_error_number")

# libc.a is the index that lets the linker pick the matching run-time library
# (rtsv7R4_A_be_v3D16_eabi.lib). The HALCoGen linker command file (sys_link.cmd) is
# added per executable by target/CMakeLists.txt.
set(CMAKE_EXE_LINKER_FLAGS_INIT
    "--search_path=${TI_CGT_ARM_ROOT}/lib --library=libc.a --reread_libs --rom_model --warn_sections --heap_size=0x800 --stack_size=0x800")

# CMake's compiler sanity check would otherwise try to link an executable without a
# linker command file.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_EXECUTABLE_SUFFIX_C   .out)
set(CMAKE_EXECUTABLE_SUFFIX_ASM .out)

# Host tools (ruby, git, python) are still found on the host; nothing else is.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
