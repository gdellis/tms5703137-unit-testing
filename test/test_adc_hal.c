/**
 * @file test_adc_hal.c
 * @brief Driver-level tests: adc_hal.c running against a RAM copy of the ADC registers.
 *
 * No mocks here. adc_hal.c was compiled with -DUNIT_TEST, so every adcREG1-> access in
 * the driver lands in adcREG1_fake below. The test pre-loads "hardware" state (e.g. an
 * end-of-conversion flag and a FIFO word) and then inspects what the driver wrote.
 */
#include "unity.h"
#include "adc_hal.h"
#include "tms570_adc_regs.h"

/* Definition of the symbol tms570_adc_regs.h declares extern under UNIT_TEST. */
adcBASE_t adcREG1_fake;

static void clear_registers(void)
{
    adcREG1_fake.RSTCR    = 0UL;
    adcREG1_fake.OPMODECR = 0UL;
    adcREG1_fake.CLOCKCR  = 0UL;
    adcREG1_fake.G1MODECR = 0UL;
    adcREG1_fake.G1SEL    = 0UL;
    adcREG1_fake.G1SR     = 0UL;
    adcREG1_fake.G1BUF    = 0UL;
}

static uint32_t fifo_word(uint8_t channel, uint16_t value)
{
    return ((uint32_t)channel << ADC_G1BUF_CHID_SHIFT) | (value & ADC_G1BUF_VALUE_MASK);
}

void setUp(void)
{
    clear_registers();
}

void tearDown(void)
{
}

/* ---- init --------------------------------------------------------------------- */

void test_init_enables_converter_in_12bit_mode(void)
{
    adc_hal_init();

    TEST_ASSERT_EQUAL_HEX32(ADC_OPMODECR_ENABLE | ADC_OPMODECR_12BIT, adcREG1_fake.OPMODECR);
}

void test_init_enables_channel_id_in_fifo(void)
{
    adc_hal_init();

    TEST_ASSERT_BITS_HIGH(ADC_G1MODECR_CHID, adcREG1_fake.G1MODECR);
}

void test_init_leaves_module_out_of_reset(void)
{
    adcREG1_fake.RSTCR = ADC_RSTCR_RESET;

    adc_hal_init();

    TEST_ASSERT_EQUAL_HEX32(0UL, adcREG1_fake.RSTCR);
}

/* ---- read_channel ------------------------------------------------------------- */

void test_read_selects_requested_channel_and_returns_fifo_value(void)
{
    uint16_t counts = 0xFFFFU;
    adcREG1_fake.G1SR  = ADC_G1SR_END;
    adcREG1_fake.G1BUF = fifo_word(5U, 0x0123U);

    TEST_ASSERT_EQUAL(ADC_HAL_OK, adc_hal_read_channel(5U, &counts));

    TEST_ASSERT_EQUAL_HEX32(1UL << 5, adcREG1_fake.G1SEL);
    TEST_ASSERT_EQUAL_HEX16(0x0123U, counts);
}

void test_read_masks_result_to_12_bits(void)
{
    uint16_t counts = 0U;
    adcREG1_fake.G1SR  = ADC_G1SR_END;
    adcREG1_fake.G1BUF = fifo_word(0U, 0xFFFFU) | 0xF000UL; /* junk above bit 11 */

    TEST_ASSERT_EQUAL(ADC_HAL_OK, adc_hal_read_channel(0U, &counts));
    TEST_ASSERT_EQUAL_HEX16(0x0FFFU, counts);
}

void test_read_times_out_when_conversion_never_completes(void)
{
    uint16_t counts = 0x5555U;
    adcREG1_fake.G1SR = 0UL; /* END never set */

    TEST_ASSERT_EQUAL(ADC_HAL_ERR_TIMEOUT, adc_hal_read_channel(3U, &counts));
    TEST_ASSERT_EQUAL_HEX16(0x5555U, counts); /* untouched on error */
}

void test_read_detects_channel_id_mismatch(void)
{
    uint16_t counts = 0x5555U;
    adcREG1_fake.G1SR  = ADC_G1SR_END;
    adcREG1_fake.G1BUF = fifo_word(3U, 0x0400U); /* asked for 5, got 3 */

    TEST_ASSERT_EQUAL(ADC_HAL_ERR_CHID_MISMATCH, adc_hal_read_channel(5U, &counts));
    TEST_ASSERT_EQUAL_HEX16(0x5555U, counts);
}

void test_read_rejects_channel_out_of_range_without_touching_hardware(void)
{
    uint16_t counts = 0U;

    TEST_ASSERT_EQUAL(ADC_HAL_ERR_BAD_ARG, adc_hal_read_channel((uint8_t)ADC_NUM_CHANNELS, &counts));
    TEST_ASSERT_EQUAL_HEX32(0UL, adcREG1_fake.G1SEL);
}

void test_read_rejects_null_result_pointer(void)
{
    TEST_ASSERT_EQUAL(ADC_HAL_ERR_BAD_ARG, adc_hal_read_channel(0U, NULL));
    TEST_ASSERT_EQUAL_HEX32(0UL, adcREG1_fake.G1SEL);
}
