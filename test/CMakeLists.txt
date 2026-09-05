set(SRC ${CMAKE_SOURCE_DIR}/src)
set(GEN ${SRC}/gen/heater_ctrl_ert_rtw)

# --- Mocks ---------------------------------------------------------------------
# One mock per header that sits on a boundary. HAL headers, hand-written app
# modules and generated model headers all work the same way.
add_cmock_mock(${SRC}/hal/adc_hal.h)
add_cmock_mock(${SRC}/hal/gio_hal.h)
add_cmock_mock(${SRC}/app/temp_monitor.h)
add_cmock_mock(${GEN}/heater_ctrl.h)

# --- Application-level tests (HAL mocked) ---------------------------------------
add_unity_test(test_temp_monitor
    SOURCES  ${SRC}/app/temp_monitor.c
    MOCKS    mock_adc_hal
    INCLUDES ${SRC}/app ${SRC}/hal
)

# --- Driver-level tests (register overlay redirected to RAM, no mocks) ----------
add_unity_test(test_adc_hal
    SOURCES  ${SRC}/hal/adc_hal.c
    INCLUDES ${SRC}/hal
)

add_unity_test(test_gio_hal
    SOURCES  ${SRC}/hal/gio_hal.c
    INCLUDES ${SRC}/hal
)

# --- Embedded Coder model, driven directly ---------------------------------------
# No SOURCES: the model is linked as the same library the firmware build uses, so
# the object code under test is byte-for-byte what ships (minus the compiler).
add_unity_test(test_heater_ctrl
    LIBS heater_ctrl_model
)

# --- Glue between hand-written code and the model (everything mocked) -------------
add_unity_test(test_heater_task
    SOURCES  ${SRC}/app/heater_task.c
    MOCKS    mock_temp_monitor mock_gio_hal mock_heater_ctrl
    INCLUDES ${SRC}/app ${SRC}/hal
)
