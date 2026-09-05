/**
 * @file temp_monitor.c
 * @brief Example application module - see temp_monitor.h.
 */
#include "temp_monitor.h"
#include "adc_hal.h"

#define ADC_VREF_MV        (3300)
#define SENSOR_OFFSET_MV   (500)  /* 500 mV at 0 degC, 10 mV/degC -> 1 mV = 0.1 degC */

static struct
{
    int16_t               temp_dc;
    temp_monitor_status_t status;
    bool                  fault_latched;
    bool                  warn_active;
    uint8_t               error_count;
} s;

static void resolve_status(void)
{
    if (s.fault_latched)
    {
        s.status = TEMP_MONITOR_FAULT;
    }
    else if (s.error_count >= TEMP_MONITOR_MAX_SENSOR_ERRORS)
    {
        s.status = TEMP_MONITOR_SENSOR_ERROR;
    }
    else if (s.warn_active)
    {
        s.status = TEMP_MONITOR_WARN;
    }
    else
    {
        s.status = TEMP_MONITOR_OK;
    }
}

void temp_monitor_init(void)
{
    s.temp_dc       = 0;
    s.status        = TEMP_MONITOR_OK;
    s.fault_latched = false;
    s.warn_active   = false;
    s.error_count   = 0U;

    adc_hal_init();
}

int16_t temp_monitor_counts_to_dc(uint16_t counts)
{
    int32_t mv;

    if (counts > ADC_HAL_MAX_COUNTS)
    {
        counts = ADC_HAL_MAX_COUNTS;
    }

    mv = ((int32_t)counts * ADC_VREF_MV) / (int32_t)ADC_HAL_MAX_COUNTS;
    return (int16_t)(mv - SENSOR_OFFSET_MV);
}

temp_monitor_status_t temp_monitor_update(void)
{
    uint16_t counts = 0U;

    if (adc_hal_read_channel(TEMP_MONITOR_ADC_CHANNEL, &counts) != ADC_HAL_OK)
    {
        if (s.error_count < TEMP_MONITOR_MAX_SENSOR_ERRORS)
        {
            s.error_count++;
        }
        resolve_status();
        return s.status;
    }

    s.error_count = 0U;
    s.temp_dc     = temp_monitor_counts_to_dc(counts);

    if (s.temp_dc >= TEMP_MONITOR_FAULT_DC)
    {
        s.fault_latched = true;
    }

    if (s.temp_dc >= TEMP_MONITOR_WARN_SET_DC)
    {
        s.warn_active = true;
    }
    else if (s.temp_dc < TEMP_MONITOR_WARN_CLEAR_DC)
    {
        s.warn_active = false;
    }
    else
    {
        /* Inside the hysteresis band: keep the previous warn state. */
    }

    resolve_status();
    return s.status;
}

int16_t temp_monitor_get_temp_dc(void)
{
    return s.temp_dc;
}

temp_monitor_status_t temp_monitor_get_status(void)
{
    return s.status;
}

bool temp_monitor_fault_latched(void)
{
    return s.fault_latched;
}

void temp_monitor_clear_fault(void)
{
    s.fault_latched = false;
    resolve_status();
}
