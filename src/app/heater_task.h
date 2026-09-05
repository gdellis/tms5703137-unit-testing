/**
 * @file heater_task.h
 * @brief Glue between hand-written code and the Embedded Coder model 'heater_ctrl'.
 *
 * This is the integration seam the model-level tests cannot see: it samples the
 * sensor through temp_monitor, converts units into the model's input struct, steps
 * the model, and pushes the model's output struct out to a GIO pin. Call
 * heater_task_run() at the model's sample rate (100 ms).
 */
#ifndef HEATER_TASK_H
#define HEATER_TASK_H

#include <stdbool.h>
#include <stdint.h>

/** gioPORTA pin wired to the heater relay driver. */
#define HEATER_TASK_GIO_PIN (2U)

typedef enum
{
    HEATER_TASK_OK = 0,      /**< Model running, sensor healthy                      */
    HEATER_TASK_DISABLED,    /**< Sensor error or temp_monitor fault: model disabled */
    HEATER_TASK_MODEL_FAULT  /**< Model's own over-temperature fault is latched      */
} heater_task_status_t;

/** Bring up the sensor, the output pin and the model (in that order). */
void heater_task_init(void);

/** One control step. */
heater_task_status_t heater_task_run(void);

#endif /* HEATER_TASK_H */
