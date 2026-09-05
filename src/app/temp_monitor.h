/**
 * @file temp_monitor.h
 * @brief Example application module: over-temperature monitor fed by one ADC channel.
 *
 * Assumes a TMP36-style analog sensor (500 mV at 0 degC, 10 mV/degC) on a 3.3 V, 12-bit
 * converter, which conveniently makes 1 mV == 0.1 degC. Temperatures are reported in
 * deci-degrees Celsius (dC) to avoid floating point.
 */
#ifndef TEMP_MONITOR_H
#define TEMP_MONITOR_H

#include <stdbool.h>
#include <stdint.h>

/** ADC channel the sensor is wired to. */
#define TEMP_MONITOR_ADC_CHANNEL (7U)

/** Thresholds in deci-degC. WARN has hysteresis; FAULT latches until cleared. */
#define TEMP_MONITOR_WARN_SET_DC     (850)
#define TEMP_MONITOR_WARN_CLEAR_DC   (800)
#define TEMP_MONITOR_FAULT_DC        (1000)

/** Consecutive ADC failures tolerated before the sensor is declared faulty. */
#define TEMP_MONITOR_MAX_SENSOR_ERRORS (3U)

typedef enum
{
    TEMP_MONITOR_OK = 0,
    TEMP_MONITOR_WARN,
    TEMP_MONITOR_FAULT,
    TEMP_MONITOR_SENSOR_ERROR
} temp_monitor_status_t;

/** Reset internal state and initialise the ADC. */
void temp_monitor_init(void);

/** Sample the sensor once and re-evaluate the status. Call periodically. */
temp_monitor_status_t temp_monitor_update(void);

/** Last successfully converted temperature in deci-degC. */
int16_t temp_monitor_get_temp_dc(void);

/** Current status without taking a new sample. */
temp_monitor_status_t temp_monitor_get_status(void);

/** True while an over-temperature fault is latched. */
bool temp_monitor_fault_latched(void);

/** Operator acknowledgement: unlatch the fault. Re-latches on the next hot sample. */
void temp_monitor_clear_fault(void);

/** Pure conversion helper, exposed for direct testing. */
int16_t temp_monitor_counts_to_dc(uint16_t counts);

#endif /* TEMP_MONITOR_H */
