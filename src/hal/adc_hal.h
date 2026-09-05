/**
 * @file adc_hal.h
 * @brief Minimal ADC hardware-abstraction interface.
 *
 * This header is the mocking boundary: application code (temp_monitor.c) only ever
 * talks to the ADC through these functions, so CMock can replace the whole module in
 * application-level tests. Keep it plain C - prototypes only, no inline code - so the
 * CMock parser stays happy.
 */
#ifndef ADC_HAL_H
#define ADC_HAL_H

#include <stdint.h>

/** Full-scale reading of the 12-bit converter. */
#define ADC_HAL_MAX_COUNTS (4095U)

typedef enum
{
    ADC_HAL_OK = 0,           /**< Conversion complete, result valid            */
    ADC_HAL_ERR_BAD_ARG,      /**< Channel out of range or NULL result pointer  */
    ADC_HAL_ERR_TIMEOUT,      /**< Conversion did not finish in time            */
    ADC_HAL_ERR_CHID_MISMATCH /**< FIFO returned a result for a different channel */
} adc_hal_status_t;

/** Reset and enable the converter in 12-bit mode. */
void adc_hal_init(void);

/**
 * Trigger a single conversion on @p channel and block until it completes.
 * @param[in]  channel  0 .. ADC_NUM_CHANNELS-1
 * @param[out] counts   Raw 12-bit result on ADC_HAL_OK; untouched otherwise.
 */
adc_hal_status_t adc_hal_read_channel(uint8_t channel, uint16_t *counts);

#endif /* ADC_HAL_H */
