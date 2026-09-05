/**
 * @file gio_hal.c
 * @brief GIO driver - see gio_hal.h. Tested against a RAM register overlay.
 */
#include "gio_hal.h"
#include "tms570_gio_regs.h"

void gio_hal_init(void)
{
    gioREG->GCR0   = GIO_GCR0_RESET_RELEASE;
    gioREG->ENACLR = GIO_PORT_ALL_PINS;
    gioREG->LVLCLR = GIO_PORT_ALL_PINS;

    gioPORTA->DIR  = 0U;
    gioPORTA->DOUT = 0U;
}

gio_hal_status_t gio_hal_set_output(uint8_t pin)
{
    if (pin >= GIO_HAL_NUM_PINS)
    {
        return GIO_HAL_ERR_BAD_ARG;
    }

    gioPORTA->DIR |= (1UL << pin);
    return GIO_HAL_OK;
}

gio_hal_status_t gio_hal_write_pin(uint8_t pin, bool high)
{
    if (pin >= GIO_HAL_NUM_PINS)
    {
        return GIO_HAL_ERR_BAD_ARG;
    }

    if (high)
    {
        gioPORTA->DSET = (1UL << pin);
    }
    else
    {
        gioPORTA->DCLR = (1UL << pin);
    }
    return GIO_HAL_OK;
}
