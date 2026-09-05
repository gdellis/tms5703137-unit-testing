set(SRC ${CMAKE_SOURCE_DIR}/src)

# --- Mocks ---------------------------------------------------------------------
# One mock per HAL header. Add more here as the HAL grows (e.g. gio_hal.h, sci_hal.h).
add_cmock_mock(${SRC}/hal/adc_hal.h)

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
