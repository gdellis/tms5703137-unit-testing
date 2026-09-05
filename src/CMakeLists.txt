# Firmware sources, built exactly as they would be for the target (no UNIT_TEST).
# On the host this is only a compile check; the tests re-compile these files with
# -DUNIT_TEST via add_unity_test().
add_library(tms570_app STATIC
    app/temp_monitor.c
    hal/adc_hal.c
)

target_include_directories(tms570_app PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/hal
)

target_compile_options(tms570_app PRIVATE -Wall -Wextra -Werror -pedantic)
