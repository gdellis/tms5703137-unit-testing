cmake_minimum_required(VERSION 3.21)

project(tms570_unit_testing_sandbox
    VERSION 0.1.0
    DESCRIPTION "Host-based unit-testing template for TMS570LS3137 firmware (Unity + CMock + CMake)"
    LANGUAGES C)

# C11 without extensions: gcc/clang get -std=c11, TI armcl gets --c11 --strict_ansi
# (supported since CGT 18.12; 20.2.x LTS is what CCS 12.8 ships). HALCoGen's own
# sources are built with extensions on (see target/CMakeLists.txt).
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# include(CTest) defines the BUILD_TESTING option (default ON) and enables ctest.
include(CTest)

# Cross-compiling for the board (cmake --preset target): HALCoGen ships assembly
# sources (sys_core.asm, sys_intvecs.asm, ...).
if(CMAKE_CROSSCOMPILING)
    enable_language(ASM)
endif()

# Optional gcov/gcovr instrumentation of the code under test (host-coverage preset).
# Defines coverage_instrument(); a no-op unless COVERAGE=ON.
include(cmake/Coverage.cmake)

# The firmware sources are built as a plain library *without* UNIT_TEST so that the
# real register addresses are compiled at least once per build. This is a compile
# smoke test only; it is never executed on the host.
add_subdirectory(src)

if(BUILD_TESTING)
    include(cmake/FetchUnityCMock.cmake)
    include(cmake/UnityTest.cmake)
    if(CMAKE_CROSSCOMPILING)
        # Board support + Unity-over-SCI; defines tms570_target_runtime, which
        # add_unity_test() links into every test executable.
        add_subdirectory(target)
    endif()
    add_subdirectory(test)
endif()
