/**
 * @file gio_hal.h
 * @brief Minimal GIO (digital output) hardware-abstraction interface for port A.
 *
 * Mocking boundary for application code that drives discrete outputs. Prototypes
 * only, so CMock can parse it.
 */
#ifndef GIO_HAL_H
#define GIO_HAL_H

#include <stdbool.h>
#include <stdint.h>

/** Pins on gioPORTA. */
#define GIO_HAL_NUM_PINS (8U)

typedef enum
{
    GIO_HAL_OK = 0,
    GIO_HAL_ERR_BAD_ARG /**< Pin number out of range */
} gio_hal_status_t;

/** Release the GIO module from reset, clear interrupts, all port A pins input/low. */
void gio_hal_init(void);

/** Configure @p pin (0 .. GIO_HAL_NUM_PINS-1) on port A as an output. */
gio_hal_status_t gio_hal_set_output(uint8_t pin);

/** Drive @p pin high or low. */
gio_hal_status_t gio_hal_write_pin(uint8_t pin, bool high);

#endif /* GIO_HAL_H */
