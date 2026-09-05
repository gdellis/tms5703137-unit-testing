/**
 * @file test_gio_hal.c
 * @brief Driver-level tests: gio_hal.c against a RAM copy of the GIO registers.
 *
 * The overlay is plain memory, so DSET/DCLR keep whatever was last written instead of
 * folding into DOUT the way hardware does. Tests therefore assert on the *write*
 * (which register got which bit), which is exactly the driver's contract.
 */
#include <string.h>
#include "unity.h"
#include "gio_hal.h"
#include "tms570_gio_regs.h"

gioBASE_t gioREG_fake;
gioPORT_t gioPORTA_fake;

void setUp(void)
{
    memset((void *)&gioREG_fake,   0, sizeof(gioREG_fake));
    memset((void *)&gioPORTA_fake, 0, sizeof(gioPORTA_fake));
}

void tearDown(void)
{
}

void test_init_releases_reset_and_clears_interrupts(void)
{
    gioREG_fake.ENASET = 0x5AU; /* noise to prove init does not touch it */

    gio_hal_init();

    TEST_ASSERT_EQUAL_HEX32(GIO_GCR0_RESET_RELEASE, gioREG_fake.GCR0);
    TEST_ASSERT_EQUAL_HEX32(GIO_PORT_ALL_PINS, gioREG_fake.ENACLR);
    TEST_ASSERT_EQUAL_HEX32(GIO_PORT_ALL_PINS, gioREG_fake.LVLCLR);
    TEST_ASSERT_EQUAL_HEX32(0x5AU, gioREG_fake.ENASET);
}

void test_init_makes_all_pins_inputs_and_low(void)
{
    gioPORTA_fake.DIR  = 0xFFU;
    gioPORTA_fake.DOUT = 0xFFU;

    gio_hal_init();

    TEST_ASSERT_EQUAL_HEX32(0U, gioPORTA_fake.DIR);
    TEST_ASSERT_EQUAL_HEX32(0U, gioPORTA_fake.DOUT);
}

void test_set_output_sets_only_that_direction_bit(void)
{
    gioPORTA_fake.DIR = 0x10U;

    TEST_ASSERT_EQUAL(GIO_HAL_OK, gio_hal_set_output(2U));

    TEST_ASSERT_EQUAL_HEX32(0x14U, gioPORTA_fake.DIR);
}

void test_set_output_rejects_bad_pin(void)
{
    TEST_ASSERT_EQUAL(GIO_HAL_ERR_BAD_ARG, gio_hal_set_output(GIO_HAL_NUM_PINS));
    TEST_ASSERT_EQUAL_HEX32(0U, gioPORTA_fake.DIR);
}

void test_write_high_uses_set_register(void)
{
    TEST_ASSERT_EQUAL(GIO_HAL_OK, gio_hal_write_pin(2U, true));

    TEST_ASSERT_EQUAL_HEX32(0x04U, gioPORTA_fake.DSET);
    TEST_ASSERT_EQUAL_HEX32(0U,    gioPORTA_fake.DCLR);
}

void test_write_low_uses_clear_register(void)
{
    TEST_ASSERT_EQUAL(GIO_HAL_OK, gio_hal_write_pin(2U, false));

    TEST_ASSERT_EQUAL_HEX32(0x04U, gioPORTA_fake.DCLR);
    TEST_ASSERT_EQUAL_HEX32(0U,    gioPORTA_fake.DSET);
}

void test_write_rejects_bad_pin_without_touching_registers(void)
{
    TEST_ASSERT_EQUAL(GIO_HAL_ERR_BAD_ARG, gio_hal_write_pin(GIO_HAL_NUM_PINS, true));

    TEST_ASSERT_EQUAL_HEX32(0U, gioPORTA_fake.DSET);
    TEST_ASSERT_EQUAL_HEX32(0U, gioPORTA_fake.DCLR);
}
