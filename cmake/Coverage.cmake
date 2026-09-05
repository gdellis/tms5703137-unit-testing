# Host-only line/branch coverage of the code under test, with gcc's --coverage (gcov)
# and gcovr for the report.
#
#   cmake --preset host-coverage
#   cmake --build --preset host-coverage
#   cmake --build --preset host-coverage-report     # runs the tests, writes the report
#
# Only the code under test is instrumented: the firmware sources compiled into each
# test executable (add_unity_test SOURCES) and the Embedded Coder model library. The
# report is filtered to src/, so Unity, CMock, mocks, runners and the test files
# themselves never appear in it. The firmware smoke library (tms570_app) is not
# instrumented - it is never executed.

option(COVERAGE
    "Instrument the code under test with --coverage (gcc only) and add a 'coverage' target"
    OFF)

set(COVERAGE_FAIL_UNDER_LINE 0 CACHE STRING
    "Fail the 'coverage' target when line coverage of src/ is below this percentage (0 = no gate)")

# Call on every target whose sources should count. No-op unless COVERAGE is on.
function(coverage_instrument TARGET)
    if(COVERAGE)
        target_compile_options(${TARGET} PRIVATE $<$<C_COMPILER_ID:GNU>:--coverage;-O0>)
        target_link_options(${TARGET}    PRIVATE $<$<C_COMPILER_ID:GNU>:--coverage>)
    endif()
endfunction()

if(NOT COVERAGE)
    return()
endif()

if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR
        "COVERAGE=ON needs gcc (gcov data format); use the 'host-coverage' preset. "
        "Compiler is ${CMAKE_C_COMPILER_ID}.")
endif()

find_program(GCOVR_EXECUTABLE gcovr REQUIRED
    DOC "gcovr, turns gcov data into HTML/Cobertura reports (pip install gcovr)")

set(COVERAGE_DIR ${CMAKE_BINARY_DIR}/coverage)

# Runs the whole suite, then gcovr. --delete removes the .gcda counters afterwards so
# every run of this target starts from zero instead of accumulating.
add_custom_target(coverage
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${COVERAGE_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_DIR}
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMAND ${GCOVR_EXECUTABLE}
            --root ${CMAKE_SOURCE_DIR}
            --object-directory ${CMAKE_BINARY_DIR}
            --filter ${CMAKE_SOURCE_DIR}/src/
            --delete
            --html-details ${COVERAGE_DIR}/index.html
            --cobertura ${COVERAGE_DIR}/coverage.xml
            --txt ${COVERAGE_DIR}/summary.txt
            --print-summary
            --fail-under-line ${COVERAGE_FAIL_UNDER_LINE}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running the tests and writing the coverage report to ${COVERAGE_DIR}"
    VERBATIM
    USES_TERMINAL)

message(STATUS "Coverage: ON (gcovr: ${GCOVR_EXECUTABLE}; report in ${COVERAGE_DIR})")
