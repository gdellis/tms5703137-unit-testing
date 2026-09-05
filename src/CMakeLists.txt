# Firmware sources, built exactly as they would be for the target (no UNIT_TEST).
# On the host this is only a compile check; the tests re-compile these files with
# -DUNIT_TEST via add_unity_test().

# Embedded Coder output lives in its own directory with its own (relaxed) warning
# policy. One add_subdirectory() per generated model.
add_subdirectory(gen/heater_ctrl_ert_rtw)

add_library(tms570_app STATIC
    app/temp_monitor.c
    app/heater_task.c
    hal/adc_hal.c
    hal/gio_hal.c
)

target_include_directories(tms570_app PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/hal
)

target_link_libraries(tms570_app PUBLIC heater_ctrl_model)

target_compile_options(tms570_app PRIVATE -Wall -Wextra -Werror -pedantic)
