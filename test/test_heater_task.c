/**
 * @file test_heater_task.c
 * @brief Glue-level tests: heater_task.c with sensor, GIO *and the model* mocked.
 *
 * Mocking a generated model from the caller's side has one wrinkle: CMock mocks
 * heater_ctrl.h's *functions*, but the model's inputs/outputs are extern globals
 * (heater_ctrl_U / heater_ctrl_Y) that the mock library does not define. The test
 * defines them and installs a callback on heater_ctrl_step() that snapshots U and
 * writes whatever Y the scenario needs. That makes the test independent of the
 * model's behaviour - which is the point: model behaviour is test_heater_ctrl.c's job.
 */
#include <string.h>
#include "unity.h"
#include "mock_temp_monitor.h" /* first: pulls in <stdbool.h> ahead of rtwtypes.h */
#include "mock_gio_hal.h"
#include "mock_heater_ctrl.h"
#include "heater_task.h"

/* The model's data objects, normally defined in heater_ctrl.c. */
ExtU_heater_ctrl_T heater_ctrl_U;
ExtY_heater_ctrl_T heater_ctrl_Y;

/* ---- fake model --------------------------------------------------------------- */

static ExtU_heater_ctrl_T seen_U;      /* inputs as they were when step() ran */
static ExtY_heater_ctrl_T next_Y;      /* outputs the fake will produce        */
static int                step_calls;

static void fake_heater_ctrl_step(int num_calls)
{
    (void)num_calls;
    seen_U = heater_ctrl_U;
    heater_ctrl_Y = next_Y;
    step_calls++;
}

/* ---- helpers ------------------------------------------------------------------ */

static void given_sensor(temp_monitor_status_t status, int16_t temp_dc)
{
    temp_monitor_update_ExpectAndReturn(status);
    temp_monitor_get_temp_dc_ExpectAndReturn(temp_dc);
}

static void given_model_outputs(bool heater_cmd, bool fault)
{
    next_Y.heater_cmd = heater_cmd ? 1U : 0U;
    next_Y.fault      = fault ? 1U : 0U;
}

static void expect_heater_pin(bool high)
{
    gio_hal_write_pin_ExpectAndReturn(HEATER_TASK_GIO_PIN, high, GIO_HAL_OK);
}

/* ---- fixture ------------------------------------------------------------------ */

void setUp(void)
{
    memset(&heater_ctrl_U, 0, sizeof(heater_ctrl_U));
    memset(&heater_ctrl_Y, 0, sizeof(heater_ctrl_Y));
    memset(&seen_U, 0, sizeof(seen_U));
    memset(&next_Y, 0, sizeof(next_Y));
    step_calls = 0;

    heater_ctrl_step_Stub(fake_heater_ctrl_step);
}

void tearDown(void)
{
}

/* ---- init --------------------------------------------------------------------- */

void test_init_brings_up_sensor_then_pin_then_model(void)
{
    /* enforce_strict_ordering makes this sequence part of the contract */
    temp_monitor_init_Expect();
    gio_hal_init_Expect();
    gio_hal_set_output_ExpectAndReturn(HEATER_TASK_GIO_PIN, GIO_HAL_OK);
    heater_ctrl_initialize_Expect();

    heater_task_init();
}

/* ---- data flow into the model ------------------------------------------------- */

void test_run_converts_deci_degc_to_model_degc(void)
{
    given_sensor(TEMP_MONITOR_OK, 235);          /* 23.5 degC */
    expect_heater_pin(false);

    (void)heater_task_run();

    TEST_ASSERT_EQUAL_INT(1, step_calls);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 23.5F, seen_U.temp_degC);
    TEST_ASSERT_TRUE(seen_U.enable);
}

void test_run_handles_negative_temperatures(void)
{
    given_sensor(TEMP_MONITOR_OK, -50);
    expect_heater_pin(false);

    (void)heater_task_run();

    TEST_ASSERT_FLOAT_WITHIN(0.001F, -5.0F, seen_U.temp_degC);
}

void test_run_keeps_model_enabled_during_sensor_warning(void)
{
    given_sensor(TEMP_MONITOR_WARN, 870);
    expect_heater_pin(false);

    TEST_ASSERT_EQUAL(HEATER_TASK_OK, heater_task_run());
    TEST_ASSERT_TRUE(seen_U.enable);
}

void test_run_disables_model_on_sensor_error(void)
{
    given_sensor(TEMP_MONITOR_SENSOR_ERROR, 235);
    expect_heater_pin(false);

    TEST_ASSERT_EQUAL(HEATER_TASK_DISABLED, heater_task_run());
    TEST_ASSERT_FALSE(seen_U.enable);
}

void test_run_disables_model_on_temp_monitor_fault(void)
{
    given_sensor(TEMP_MONITOR_FAULT, 1050);
    expect_heater_pin(false);

    TEST_ASSERT_EQUAL(HEATER_TASK_DISABLED, heater_task_run());
    TEST_ASSERT_FALSE(seen_U.enable);
}

/* ---- data flow out of the model ----------------------------------------------- */

void test_run_drives_pin_high_when_model_commands_heat(void)
{
    given_sensor(TEMP_MONITOR_OK, 300);
    given_model_outputs(true, false);
    expect_heater_pin(true);

    TEST_ASSERT_EQUAL(HEATER_TASK_OK, heater_task_run());
}

void test_run_drives_pin_low_when_model_commands_off(void)
{
    given_sensor(TEMP_MONITOR_OK, 450);
    given_model_outputs(false, false);
    expect_heater_pin(false);

    TEST_ASSERT_EQUAL(HEATER_TASK_OK, heater_task_run());
}

void test_run_reports_model_fault_and_still_writes_pin(void)
{
    given_sensor(TEMP_MONITOR_OK, 650);
    given_model_outputs(false, true);
    expect_heater_pin(false);

    TEST_ASSERT_EQUAL(HEATER_TASK_MODEL_FAULT, heater_task_run());
}

void test_model_fault_outranks_sensor_error(void)
{
    given_sensor(TEMP_MONITOR_SENSOR_ERROR, 0);
    given_model_outputs(false, true);
    expect_heater_pin(false);

    TEST_ASSERT_EQUAL(HEATER_TASK_MODEL_FAULT, heater_task_run());
}
