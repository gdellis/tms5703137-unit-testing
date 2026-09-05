# Fetches pinned versions of Unity (test framework) and CMock (mock generator) at
# configure time and exposes them as CMake targets:
#
#   unity::framework   - Unity's own CMake target (static lib + include dir)
#   cmock              - CMock runtime (src/cmock.c), links unity::framework
#
# Also locates the Ruby interpreter that CMock and Unity's runner generator need.

include(FetchContent)

set(UNITY_GIT_TAG v2.7.0 CACHE STRING "Unity git tag to fetch")
set(CMOCK_GIT_TAG v2.7.0 CACHE STRING "CMock git tag to fetch")

FetchContent_Declare(unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG        ${UNITY_GIT_TAG}
    GIT_SHALLOW    TRUE
)

# CMock ships no CMakeLists.txt, so FetchContent only downloads it and we build the
# single runtime source file ourselves below. GIT_SUBMODULES "" skips CMock's vendored
# copies of Unity/CException, which we do not need.
FetchContent_Declare(cmock
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/CMock.git
    GIT_TAG        ${CMOCK_GIT_TAG}
    GIT_SHALLOW    TRUE
    GIT_SUBMODULES ""
)

FetchContent_MakeAvailable(unity cmock)

add_library(cmock STATIC ${cmock_SOURCE_DIR}/src/cmock.c)
target_include_directories(cmock PUBLIC ${cmock_SOURCE_DIR}/src)
target_link_libraries(cmock PUBLIC unity::framework)

find_program(RUBY_EXECUTABLE ruby REQUIRED
    DOC "Ruby interpreter, needed by CMock (mock generation) and Unity (runner generation)")

message(STATUS "Unity ${UNITY_GIT_TAG}: ${unity_SOURCE_DIR}")
message(STATUS "CMock ${CMOCK_GIT_TAG}: ${cmock_SOURCE_DIR}")
message(STATUS "Ruby: ${RUBY_EXECUTABLE}")
