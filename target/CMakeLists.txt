# On-target test runtime for the TMS570LS3137: HALCoGen board support plus Unity's
# output hooks routed to an SCI. Only entered when cross-compiling (see the top-level
# CMakeLists.txt); the host presets never see this directory.
#
# HALCOGEN_DIR must point at a HALCoGen project directory (the one containing
# include/ and source/). It is not committed - generate it as described in
# docs/03-on-target.md.

set(HALCOGEN_DIR "${CMAKE_SOURCE_DIR}/target/halcogen" CACHE PATH
    "HALCoGen project directory (contains include/ and source/)")

set(TMS570_UNITY_SCI "scilinREG" CACHE STRING
    "HALCoGen SCI instance Unity prints to (scilinREG or sciREG)")

if(NOT EXISTS "${HALCOGEN_DIR}/source/sys_link.cmd")
    message(FATAL_ERROR
        "HALCOGEN_DIR='${HALCOGEN_DIR}' is not a HALCoGen project (no source/sys_link.cmd). "
        "Generate one as described in docs/03-on-target.md, or pass -DHALCOGEN_DIR=<dir>.")
endif()

# --- HALCoGen board support ------------------------------------------------------
# sys_main.c holds HALCoGen's main(); each generated Unity runner provides its own.
file(GLOB HALCOGEN_C   CONFIGURE_DEPENDS "${HALCOGEN_DIR}/source/*.c")
file(GLOB HALCOGEN_ASM CONFIGURE_DEPENDS "${HALCOGEN_DIR}/source/*.asm")
list(FILTER HALCOGEN_C EXCLUDE REGEX "/sys_main\\.c$")

# OBJECT, not STATIC: the vector table (sys_intvecs.asm) and start-up code are not
# referenced by any symbol, so they would never be pulled out of an archive. Linking
# the objects directly keeps them, exactly as the CCS project does.
add_library(tms570_bsp OBJECT ${HALCOGEN_C} ${HALCOGEN_ASM})
target_include_directories(tms570_bsp PUBLIC "${HALCOGEN_DIR}/include")
# HALCoGen sources rely on TI extensions; do not build them with --strict_ansi.
set_target_properties(tms570_bsp PROPERTIES C_EXTENSIONS ON)

# --- Unity output over SCI ---------------------------------------------------------
add_library(tms570_unity_io OBJECT unity_target_io.c)
target_include_directories(tms570_unity_io PRIVATE "${HALCOGEN_DIR}/include")
target_compile_definitions(tms570_unity_io PRIVATE UNITY_TARGET_SCI=${TMS570_UNITY_SCI})
set_target_properties(tms570_unity_io PROPERTIES C_EXTENSIONS ON)

# Make Unity itself read target/unity_config.h (ILP32 widths, output hooks).
target_compile_definitions(unity PUBLIC UNITY_INCLUDE_CONFIG_H)
# BUILD_INTERFACE because Unity's CMakeLists exports the target for install().
target_include_directories(unity PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

# --- What every test executable links ---------------------------------------------
# add_unity_test() links this interface target whenever it exists.
add_library(tms570_target_runtime INTERFACE)
target_sources(tms570_target_runtime INTERFACE
    $<TARGET_OBJECTS:tms570_bsp>
    $<TARGET_OBJECTS:tms570_unity_io>)
target_include_directories(tms570_target_runtime INTERFACE "${HALCOGEN_DIR}/include")
# A bare path after --run_linker is a linker command file to armcl.
target_link_options(tms570_target_runtime INTERFACE "${HALCOGEN_DIR}/source/sys_link.cmd")
