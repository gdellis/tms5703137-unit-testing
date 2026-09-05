/**
 * @file test_temp_monitor.c
 * @brief Application-level tests: temp_monitor.c with the ADC HAL mocked by CMock.
 *
 * Including "mock_adc_hal.h" is what tells the generated runner to Init/Verify/Destroy
 * the mock around every test. Any call into adc_hal_* that was not Expect-ed fails
 * the test; any Expect-ed call that never happened fails it at Verify.
 */
#include "unity.h"
#include "mock_adc_hal.h"
#include "temp_monitor.h"

/* ---- helpers ------------------------------------------------------------------ */

/* ReturnThruPtr copies from the pointer *when the mocked call executes*, so the
 * values must outlive the helper that queues them. A small static stash does it. */
static uint16_t stash[16];
static unsigned stash_used;

static void given_adc_reads(uint16_t counts)
{
    TEST_ASSERT_LESS_THAN_MESSAGE(sizeof(stash) / sizeof(stash[0]), stash_used, "stash full");
    stash[stash_used] = counts;

    adc_hal_read_channel_ExpectAndReturn(TEMP_MONITOR_ADC_CHANNEL, NULL, ADC_HAL_OK);
    adc_hal_read_channel_IgnoreArg_counts();
    adc_hal_read_channel_ReturnThruPtr_counts(&stash[stash_used]);
    stash_used++;
}

static void given_adc_fails(adc_hal_status_t err)
{
    adc_hal_read_channel_ExpectAndReturn(TEMP_MONITOR_ADC_CHANNEL, NULL, err);
    adc_hal_read_channel_IgnoreArg_counts();
}

/* Sensor model: 1 mV == 0.1 degC, 500 mV offset, 3300 mV / 4095 counts.
 * Chosen so the integer maths lands exactly on the value named. */
#define COUNTS_FOR_M500_DC  (0U)      /*  -50.0 degC */
#define COUNTS_FOR_500_DC   (1241U)   /*   50.0 degC */
#define COUNTS_FOR_780_DC   (1589U)   /*   78.0 degC - below WARN_CLEAR */
#define COUNTS_FOR_820_DC   (1638U)   /*   82.0 degC - inside hysteresis band */
#define COUNTS_FOR_900_DC   (1738U)   /*   90.0 degC - above WARN_SET */
#define COUNTS_FOR_1050_DC  (1924U)   /*  105.0 degC - above FAULT */
#define COUNTS_FOR_2800_DC  (4095U)   /*  280.0 degC - full scale */

/* ---- fixture ------------------------------------------------------------------ */

void setUp(void)
{
    stash_used = 0U;
    adc_hal_init_Expect();
    temp_monitor_init();
}

void tearDown(void)
{
}

/* ---- pure conversion ---------------------------------------------------------- */

void test_counts_to_dc_at_zero_scale(void)
{
    TEST_ASSERT_EQUAL_INT16(-500, temp_monitor_counts_to_dc(COUNTS_FOR_M500_DC));
}

void test_counts_to_dc_at_full_scale(void)
{
    TEST_ASSERT_EQUAL_INT16(2800, temp_monitor_counts_to_dc(COUNTS_FOR_2800_DC));
}

void test_counts_to_dc_mid_scale(void)
{
    TEST_ASSERT_EQUAL_INT16(500, temp_monitor_counts_to_dc(COUNTS_FOR_500_DC));
}

void test_counts_to_dc_clamps_out_of_range_input(void)
{
    TEST_ASSERT_EQUAL_INT16(2800, temp_monitor_counts_to_dc(0xFFFFU));
}

/* ---- status logic (mock-driven) ------------------------------------------------ */

void test_init_reports_ok_and_no_fault(void)
{
    TEST_ASSERT_EQUAL(TEMP_MONITOR_OK, temp_monitor_get_status());
    TEST_ASSERT_FALSE(temp_monitor_fault_latched());
}

void test_update_reads_configured_channel_and_stores_temperature(void)
{
    given_adc_reads(COUNTS_FOR_500_DC);

    TEST_ASSERT_EQUAL(TEMP_MONITOR_OK, temp_monitor_update());
    TEST_ASSERT_EQUAL_INT16(500, temp_monitor_get_temp_dc());
}

void test_update_warns_above_warn_threshold(void)
{
    given_adc_reads(COUNTS_FOR_900_DC);

    TEST_ASSERT_EQUAL(TEMP_MONITOR_WARN, temp_monitor_update());
    TEST_ASSERT_FALSE(temp_monitor_fault_latched());
}

void test_warn_has_hysteresis(void)
{
    given_adc_reads(COUNTS_FOR_900_DC); /* enters WARN            */
    given_adc_reads(COUNTS_FOR_820_DC); /* in band: still WARN    */
    given_adc_reads(COUNTS_FOR_780_DC); /* below clear: back to OK */

    TEST_ASSERT_EQUAL(TEMP_MONITOR_WARN, temp_monitor_update());
    TEST_ASSERT_EQUAL(TEMP_MONITOR_WARN, temp_monitor_update());
    TEST_ASSERT_EQUAL(TEMP_MONITOR_OK,   temp_monitor_update());
}

void test_fault_latches_until_cleared(void)
{
    given_adc_reads(COUNTS_FOR_1050_DC); /* hot: FAULT               */
    given_adc_reads(COUNTS_FOR_500_DC);  /* cool again: still FAULT  */
    given_adc_reads(COUNTS_FOR_500_DC);  /* after clear: OK          */

    TEST_ASSERT_EQUAL(TEMP_MONITOR_FAULT, temp_monitor_update());
    TEST_ASSERT_EQUAL(TEMP_MONITOR_FAULT, temp_monitor_update());
    TEST_ASSERT_TRUE(temp_monitor_fault_latched());

    temp_monitor_clear_fault();
    TEST_ASSERT_FALSE(temp_monitor_fault_latched());
    TEST_ASSERT_EQUAL(TEMP_MONITOR_OK, temp_monitor_update());
}

void test_clear_fault_relatches_if_still_hot(void)
{
    given_adc_reads(COUNTS_FOR_1050_DC);
    given_adc_reads(COUNTS_FOR_1050_DC);

    TEST_ASSERT_EQUAL(TEMP_MONITOR_FAULT, temp_monitor_update());
    temp_monitor_clear_fault();
    TEST_ASSERT_EQUAL(TEMP_MONITOR_FAULT, temp_monitor_update());
    TEST_ASSERT_TRUE(temp_monitor_fault_latched());
}

void test_sensor_error_only_after_consecutive_failures(void)
{
    given_adc_fails(ADC_HAL_ERR_TIMEOUT);
    given_adc_fails(ADC_HAL_ERR_TIMEOUT);
    given_adc_fails(ADC_HAL_ERR_CHID_MISMATCH);

    TEST_ASSERT_EQUAL(TEMP_MONITOR_OK,           temp_monitor_update());
    TEST_ASSERT_EQUAL(TEMP_MONITOR_OK,           temp_monitor_update());
    TEST_ASSERT_EQUAL(TEMP_MONITOR_SENSOR_ERROR, temp_monitor_update());
}

void test_error_counter_saturates_without_wrapping(void)
{
    /* Found by the coverage report: the "already at max" side of the counter guard.
     * Many more failures than the threshold must neither wrap nor clear the error. */
    unsigned i;
    for (i = 0U; i < 10U; i++)
    {
        given_adc_fails(ADC_HAL_ERR_TIMEOUT);
    }

    for (i = 0U; i < 10U; i++)
    {
        (void)temp_monitor_update();
    }
    TEST_ASSERT_EQUAL(TEMP_MONITOR_SENSOR_ERROR, temp_monitor_get_status());
}

void test_good_read_resets_error_counter(void)
{
    given_adc_fails(ADC_HAL_ERR_TIMEOUT);
    given_adc_fails(ADC_HAL_ERR_TIMEOUT);
    given_adc_reads(COUNTS_FOR_500_DC);
    given_adc_fails(ADC_HAL_ERR_TIMEOUT);
    given_adc_fails(ADC_HAL_ERR_TIMEOUT);

    (void)temp_monitor_update();
    (void)temp_monitor_update();
    TEST_ASSERT_EQUAL(TEMP_MONITOR_OK, temp_monitor_update());
    (void)temp_monitor_update();
    TEST_ASSERT_EQUAL(TEMP_MONITOR_OK, temp_monitor_update()); /* only 2 in a row */
}

void test_latched_fault_dominates_sensor_error(void)
{
    given_adc_reads(COUNTS_FOR_1050_DC);
    given_adc_fails(ADC_HAL_ERR_TIMEOUT);
    given_adc_fails(ADC_HAL_ERR_TIMEOUT);
    given_adc_fails(ADC_HAL_ERR_TIMEOUT);

    TEST_ASSERT_EQUAL(TEMP_MONITOR_FAULT, temp_monitor_update());
    (void)temp_monitor_update();
    (void)temp_monitor_update();
    TEST_ASSERT_EQUAL(TEMP_MONITOR_FAULT, temp_monitor_update());
}
