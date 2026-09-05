# Embedded Coder output for model 'heater_ctrl', built once as a library and linked
# into both the firmware smoke build (tms570_app) and the model-level test.
#
# Generated code is compiled *as shipped*: no UNIT_TEST, and warnings are reported
# but not fatal. Generated code should be warning-clean at -Wall, but you do not get
# to fix it when it is not - re-generate with different settings instead.
#
# This CMakeLists.txt is the only hand-maintained file in this directory; everything
# else is (a stand-in for) generated output and gets overwritten on regeneration.
add_library(heater_ctrl_model STATIC
    heater_ctrl.c
    heater_ctrl_data.c
)

target_include_directories(heater_ctrl_model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(heater_ctrl_model PRIVATE $<$<C_COMPILER_ID:GNU,Clang>:-Wall;-Wextra>)
