/**
 * @file test_heater_ctrl.c
 * @brief Model-level tests: drive the Embedded Coder model 'heater_ctrl' directly.
 *
 * Pattern: set heater_ctrl_U, call heater_ctrl_step(), assert on heater_ctrl_Y.
 * No mocks - generated code talks to nothing but its own globals. The test links
 * the same heater_ctrl_model library the firmware build uses.
 *
 * Two things to know about Embedded Coder output before writing tests like this:
 *  1. heater_ctrl_initialize() does NOT zero the states/IO (ERT default "Remove
 *     internal data zero initialization"); it relies on C start-up. setUp() must.
 *  2. heater_ctrl_P (tunable parameters) is a global the tests can change. Restore it
 *     in tearDown() or later tests inherit the change.
 */
#include <string.h>
#include "unity.h"
#include "heater_ctrl.h"

/* Default parameters: Setpoint 40, Hysteresis 2 -> on at <= 39.0, off at >= 41.0;
 * OverTemp 60, debounce 3 steps. */
#define T_COLD       (30.0F)
#define T_ON_EDGE    (39.0F)
#define T_IN_BAND    (40.0F)
#define T_OFF_EDGE   (41.0F)
#define T_OVER       (61.0F)

/* The relay edges above sit exactly on the band, which pins them. The over-temperature
 * compare had no such pair: T_OVER is a whole degree clear of the threshold, so any
 * threshold between the band and 61.0 passed, and > was indistinguishable from >=.
 * These two straddle it. See docs/08-test-design-and-strategy.md section 2. */
#define T_OVER_EDGE  (60.0F)   /* exactly OverTemp_degC - the compare is >, so no count */
#define T_JUST_OVER  (60.1F)   /* over by 0.1 degC: the resolution this pins to        */

/* temp_degC is continuous, so unlike temp_monitor's ADC counts there is no "one unit
 * above" to test - the pair above pins the trip point to 0.1 degC, and a threshold
 * error smaller than that is deliberately out of scope. Pick the gap from the
 * requirement, not from what is convenient. */

/* ---- fixture ------------------------------------------------------------------ */

static P_heater_ctrl_T P_defaults;
static boolean_T       P_defaults_captured;

void setUp(void)
{
    if (!P_defaults_captured)
    {
        P_defaults          = heater_ctrl_P;
        P_defaults_captured = true;
    }

    memset(&heater_ctrl_DW, 0, sizeof(heater_ctrl_DW));
    memset(&heater_ctrl_U,  0, sizeof(heater_ctrl_U));
    memset(&heater_ctrl_Y,  0, sizeof(heater_ctrl_Y));
    heater_ctrl_initialize();
}

void tearDown(void)
{
    heater_ctrl_P = P_defaults;
}

/* ---- helpers ------------------------------------------------------------------ */

/** One sample period with the given inputs; returns heater_cmd. */
static boolean_T step(real32_T temp_degC, boolean_T enable)
{
    heater_ctrl_U.temp_degC = temp_degC;
    heater_ctrl_U.enable    = enable;
    heater_ctrl_step();
    return heater_ctrl_Y.heater_cmd;
}

static void step_n(real32_T temp_degC, boolean_T enable, unsigned n)
{
    while (n-- > 0U)
    {
        (void)step(temp_degC, enable);
    }
}

/* ---- initialisation ----------------------------------------------------------- */

void test_initialize_leaves_outputs_off(void)
{
    TEST_ASSERT_FALSE(heater_ctrl_Y.heater_cmd);
    TEST_ASSERT_FALSE(heater_ctrl_Y.fault);
    TEST_ASSERT_NULL(rtmGetErrorStatus(heater_ctrl_M));
}

void test_default_parameters_are_as_documented(void)
{
    TEST_ASSERT_EQUAL_FLOAT(40.0F, heater_ctrl_P.Setpoint_degC);
    TEST_ASSERT_EQUAL_FLOAT(2.0F,  heater_ctrl_P.Hysteresis_degC);
    TEST_ASSERT_EQUAL_FLOAT(60.0F, heater_ctrl_P.OverTemp_degC);
    TEST_ASSERT_EQUAL_UINT8(3U,    heater_ctrl_P.FaultDebounce_steps);
}

/* ---- hysteresis --------------------------------------------------------------- */

void test_heater_turns_on_at_lower_threshold(void)
{
    TEST_ASSERT_TRUE(step(T_ON_EDGE, true));
}

void test_heater_turns_off_at_upper_threshold(void)
{
    (void)step(T_COLD, true);
    TEST_ASSERT_TRUE(step(T_IN_BAND, true));     /* still on while warming */
    TEST_ASSERT_FALSE(step(T_OFF_EDGE, true));
}

void test_heater_holds_state_inside_band_when_cooling(void)
{
    (void)step(T_OFF_EDGE, true);                /* off */
    TEST_ASSERT_FALSE(step(T_IN_BAND, true));    /* in band: stays off */
    TEST_ASSERT_FALSE(step(T_ON_EDGE + 0.5F, true));
    TEST_ASSERT_TRUE(step(T_ON_EDGE, true));     /* only at the edge */
}

void test_heater_starts_off_inside_band(void)
{
    TEST_ASSERT_FALSE(step(T_IN_BAND, true));
}

/* ---- enable ------------------------------------------------------------------- */

void test_disable_forces_heater_off(void)
{
    (void)step(T_COLD, true);
    TEST_ASSERT_FALSE(step(T_COLD, false));
}

void test_disable_resets_relay_state(void)
{
    (void)step(T_COLD, true);                    /* on */
    (void)step(T_COLD, false);                   /* disabled -> state reset */
    TEST_ASSERT_FALSE(step(T_IN_BAND, true));    /* re-enabled in band: off, not on */
}

/* ---- over-temperature fault --------------------------------------------------- */

void test_overtemp_faults_only_after_debounce(void)
{
    step_n(T_OVER, true, 2U);
    TEST_ASSERT_FALSE(heater_ctrl_Y.fault);

    (void)step(T_OVER, true);
    TEST_ASSERT_TRUE(heater_ctrl_Y.fault);
}

void test_overtemp_does_not_count_exactly_at_threshold(void)
{
    /* The compare is strictly greater, so sitting on the threshold for far longer
     * than the debounce must never latch. Holding at 60.0 rather than passing
     * through it is what makes >= (and a threshold nudged down) visible. */
    step_n(T_OVER_EDGE, true, 10U);

    TEST_ASSERT_FALSE(heater_ctrl_Y.fault);
}

void test_overtemp_counts_just_above_threshold(void)
{
    /* The other side of the same edge. Without this a threshold nudged *up* would go
     * unnoticed, because 61.0 would still trip it. */
    step_n(T_JUST_OVER, true, 3U);

    TEST_ASSERT_TRUE(heater_ctrl_Y.fault);
}

void test_disable_restarts_the_debounce_counter(void)
{
    /* Disabling resets OverTemp_count, so the debounce starts from zero on
     * re-enable. test_fault_latches_until_disabled cannot see this: by the time it
     * disables, the counter has already been cleared by a cold sample. */
    step_n(T_OVER, true, 2U);                    /* one short of the debounce */
    (void)step(T_COLD, false);                   /* disabled: states reset     */

    step_n(T_OVER, true, 2U);                    /* counting again, from zero  */
    TEST_ASSERT_FALSE(heater_ctrl_Y.fault);

    (void)step(T_OVER, true);                    /* third in a row: now it latches */
    TEST_ASSERT_TRUE(heater_ctrl_Y.fault);
}

void test_overtemp_glitch_shorter_than_debounce_is_ignored(void)
{
    step_n(T_OVER, true, 2U);
    (void)step(T_COLD, true);                    /* counter resets */
    step_n(T_OVER, true, 2U);

    TEST_ASSERT_FALSE(heater_ctrl_Y.fault);
}

void test_overtemp_counter_saturates_without_wrapping(void)
{
    /* Found by the coverage report: the "already at debounce" side of the counter
     * guard. A long over-temperature must keep the fault, not wrap the counter. */
    step_n(T_OVER, true, 300U);          /* uint8 counter: > 255 steps */
    TEST_ASSERT_TRUE(heater_ctrl_Y.fault);
    TEST_ASSERT_EQUAL_UINT8(heater_ctrl_P.FaultDebounce_steps, heater_ctrl_DW.OverTemp_count);
}

void test_fault_forces_heater_off_even_when_cold(void)
{
    step_n(T_OVER, true, 3U);
    TEST_ASSERT_FALSE(step(T_COLD, true));
    TEST_ASSERT_TRUE(heater_ctrl_Y.fault);
}

void test_fault_latches_until_disabled(void)
{
    step_n(T_OVER, true, 3U);
    step_n(T_COLD, true, 10U);
    TEST_ASSERT_TRUE(heater_ctrl_Y.fault);

    (void)step(T_COLD, false);
    TEST_ASSERT_FALSE(heater_ctrl_Y.fault);

    TEST_ASSERT_TRUE(step(T_COLD, true));        /* back in service */
}

void test_disabled_model_does_not_accumulate_fault(void)
{
    step_n(T_OVER, false, 10U);
    TEST_ASSERT_FALSE(heater_ctrl_Y.fault);
    (void)step(T_OVER, true);                    /* first counted sample */
    TEST_ASSERT_FALSE(heater_ctrl_Y.fault);
}

/* ---- tunable parameters ------------------------------------------------------- */

void test_setpoint_is_tunable(void)
{
    heater_ctrl_P.Setpoint_degC = 20.0F;         /* band is now 19 .. 21 */

    TEST_ASSERT_FALSE(step(T_COLD, true));       /* 30 is hot for this setpoint */
    TEST_ASSERT_TRUE(step(19.0F, true));
}

void test_debounce_is_tunable(void)
{
    heater_ctrl_P.FaultDebounce_steps = 1U;

    (void)step(T_OVER, true);
    TEST_ASSERT_TRUE(heater_ctrl_Y.fault);
}

void test_parameters_restored_between_tests(void)
{
    /* Runs after the two tests above (runner keeps file order). */
    TEST_ASSERT_EQUAL_FLOAT(40.0F, heater_ctrl_P.Setpoint_degC);
    TEST_ASSERT_EQUAL_UINT8(3U, heater_ctrl_P.FaultDebounce_steps);
}

/* ---- housekeeping ------------------------------------------------------------- */

void test_terminate_is_callable(void)
{
    heater_ctrl_terminate();
    TEST_ASSERT_NULL(rtmGetErrorStatus(heater_ctrl_M));
}
