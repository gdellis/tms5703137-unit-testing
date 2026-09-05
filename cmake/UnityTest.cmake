# Helper functions for defining Unity/CMock tests.
#
#   add_cmock_mock(<path/to/header.h>)
#       Generates mock_<header>.c/.h with CMock and wraps them in a static library
#       target named mock_<header>. Link that target into any test that needs it.
#
#   add_unity_test(<name> SOURCES <src...> [MOCKS <mock targets...>]
#                  [INCLUDES <dirs...>] [LIBS <targets...>])
#       Builds test/<name>.c together with a generated Unity runner and the listed
#       firmware sources (compiled with -DUNIT_TEST), links the mocks and registers
#       the executable with CTest. LIBS links pre-built libraries *as shipped* (no
#       UNIT_TEST recompile) - used for generated code such as Embedded Coder models.

set(CMOCK_CONFIG_DIR  ${CMAKE_SOURCE_DIR}/test/support)
set(CMOCK_CONFIG_FILE cmock_config.yml)

function(add_cmock_mock HEADER)
    get_filename_component(_name    ${HEADER} NAME_WE)
    get_filename_component(_hdr_dir ${HEADER} DIRECTORY)
    set(_out_dir ${CMAKE_CURRENT_BINARY_DIR}/mocks)
    set(_mock_c  ${_out_dir}/mock_${_name}.c)
    set(_mock_h  ${_out_dir}/mock_${_name}.h)

    # cmock.rb is run from the directory holding the YAML config so that the -o
    # argument stays a bare filename (the -o parser is picky about path characters).
    # UNITY_DIR points CMock's generator at the Unity checkout we already fetched
    # instead of its vendored submodule copy, so there is exactly one Unity in play.
    add_custom_command(
        OUTPUT  ${_mock_c} ${_mock_h}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${_out_dir}
        COMMAND ${CMAKE_COMMAND} -E env UNITY_DIR=${unity_SOURCE_DIR}
                ${RUBY_EXECUTABLE} ${cmock_SOURCE_DIR}/lib/cmock.rb
                -o${CMOCK_CONFIG_FILE}
                --mock_path=${_out_dir}
                ${HEADER}
        WORKING_DIRECTORY ${CMOCK_CONFIG_DIR}
        DEPENDS ${HEADER} ${CMOCK_CONFIG_DIR}/${CMOCK_CONFIG_FILE}
        COMMENT "CMock: generating mock_${_name}"
        VERBATIM)

    add_library(mock_${_name} STATIC ${_mock_c})
    target_include_directories(mock_${_name} PUBLIC ${_out_dir} ${_hdr_dir})
    target_link_libraries(mock_${_name} PUBLIC cmock unity::framework)
endfunction()

function(add_unity_test NAME)
    cmake_parse_arguments(UT "" "" "SOURCES;MOCKS;INCLUDES;LIBS" ${ARGN})

    set(_test_src ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}.c)
    set(_runner   ${CMAKE_CURRENT_BINARY_DIR}/runners/${NAME}_runner.c)

    # The runner scans the test file for test_* functions and #include "mock_*.h"
    # lines, then emits main() with the Unity/CMock init-verify-destroy boilerplate.
    # It is given the CMock config so that options which affect the runner (mock
    # prefix, enforce_strict_ordering -> GlobalExpectCount/GlobalVerifyOrder) match
    # what the mocks were generated with.
    add_custom_command(
        OUTPUT  ${_runner}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/runners
        COMMAND ${RUBY_EXECUTABLE} ${unity_SOURCE_DIR}/auto/generate_test_runner.rb
                ${CMOCK_CONFIG_DIR}/${CMOCK_CONFIG_FILE} ${_test_src} ${_runner}
        DEPENDS ${_test_src} ${CMOCK_CONFIG_DIR}/${CMOCK_CONFIG_FILE}
        COMMENT "Unity: generating runner for ${NAME}"
        VERBATIM)

    add_executable(${NAME} ${_test_src} ${_runner} ${UT_SOURCES})
    target_compile_definitions(${NAME} PRIVATE UNIT_TEST)
    target_compile_options(${NAME} PRIVATE $<$<C_COMPILER_ID:GNU,Clang>:-Wall;-Wextra>)
    target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support ${UT_INCLUDES})
    target_link_libraries(${NAME} PRIVATE unity::framework ${UT_MOCKS} ${UT_LIBS})

    # On the board every test binary also needs the HALCoGen start-up code, the SCI
    # output hooks and the linker command file (see target/CMakeLists.txt).
    if(TARGET tms570_target_runtime)
        target_link_libraries(${NAME} PRIVATE tms570_target_runtime)
    endif()

    # When cross-compiling, CTest prefixes the command with CMAKE_CROSSCOMPILING_EMULATOR
    # (tools/run_on_target.sh in the 'target' preset), which flashes and captures.
    add_test(NAME ${NAME} COMMAND ${NAME})
endfunction()
