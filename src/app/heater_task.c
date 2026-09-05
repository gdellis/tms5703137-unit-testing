/**
 * @file heater_task.c
 * @brief Glue between temp_monitor / gio_hal and the generated heater_ctrl model.
 */
#include "heater_task.h"
#include "temp_monitor.h"
#include "gio_hal.h"

/* Generated header last: rtwtypes.h defines true/false only if <stdbool.h> has not
 * already; the other order makes <stdbool.h> redefine them (see docs/02, 5.3). */
#include "heater_ctrl.h"

#define DECI_DEGC_PER_DEGC (10.0F)

void heater_task_init(void)
{
    temp_monitor_init();
    gio_hal_init();
    (void)gio_hal_set_output(HEATER_TASK_GIO_PIN);
    heater_ctrl_initialize();
}

heater_task_status_t heater_task_run(void)
{
    const temp_monitor_status_t sensor = temp_monitor_update();
    const bool sensor_usable = (sensor == TEMP_MONITOR_OK) || (sensor == TEMP_MONITOR_WARN);

    /* Inputs: units are the model's (degC as real32_T), not the sensor's (deci-degC). */
    heater_ctrl_U.temp_degC = (real32_T)temp_monitor_get_temp_dc() / DECI_DEGC_PER_DEGC;
    heater_ctrl_U.enable    = sensor_usable ? true : false;

    heater_ctrl_step();

    /* Outputs */
    (void)gio_hal_write_pin(HEATER_TASK_GIO_PIN, heater_ctrl_Y.heater_cmd != 0U);

    if (heater_ctrl_Y.fault != 0U)
    {
        return HEATER_TASK_MODEL_FAULT;
    }
    if (!sensor_usable)
    {
        return HEATER_TASK_DISABLED;
    }
    return HEATER_TASK_OK;
}
