cmake_minimum_required(VERSION 3.21)

project(tms570_unit_testing_sandbox
    VERSION 0.1.0
    DESCRIPTION "Host-based unit-testing template for TMS570LS3137 firmware (Unity + CMock + CMake)"
    LANGUAGES C)

# Match what the TI ARM compiler is normally configured for on TMS570 projects.
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# include(CTest) defines the BUILD_TESTING option (default ON) and enables ctest.
include(CTest)

# The firmware sources are built as a plain library *without* UNIT_TEST so that the
# real register addresses are compiled at least once per build. This is a compile
# smoke test only; it is never executed on the host.
add_subdirectory(src)

if(BUILD_TESTING)
    include(cmake/FetchUnityCMock.cmake)
    include(cmake/UnityTest.cmake)
    add_subdirectory(test)
endif()
