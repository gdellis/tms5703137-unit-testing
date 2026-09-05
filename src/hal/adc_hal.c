/**
 * @file adc_hal.c
 * @brief ADC driver written against the register overlay in tms570_adc_regs.h.
 *
 * Deliberately simple polling driver. It is tested on the host by pointing adcREG1 at
 * a RAM struct (see test/test_adc_hal.c) - no mocks involved.
 */
#include "adc_hal.h"
#include "tms570_adc_regs.h"

#include <stddef.h>

/** Prescaler value: illustrative only. */
#define ADC_HAL_CLOCK_PRESCALE (7UL)

/** Poll iterations before giving up on the end-of-conversion flag. */
#define ADC_HAL_TIMEOUT_SPINS (10000UL)

void adc_hal_init(void)
{
    adcREG1->RSTCR    = ADC_RSTCR_RESET;
    adcREG1->RSTCR    = 0UL;
    adcREG1->CLOCKCR  = ADC_HAL_CLOCK_PRESCALE;
    adcREG1->G1MODECR = ADC_G1MODECR_CHID;
    adcREG1->OPMODECR = ADC_OPMODECR_ENABLE | ADC_OPMODECR_12BIT;
}

adc_hal_status_t adc_hal_read_channel(uint8_t channel, uint16_t *counts)
{
    uint32_t spins = 0UL;
    uint32_t buf;

    if ((channel >= ADC_NUM_CHANNELS) || (counts == NULL))
    {
        return ADC_HAL_ERR_BAD_ARG;
    }

    /* Writing the channel select starts the group-1 conversion. */
    adcREG1->G1SEL = (1UL << channel);

    while ((adcREG1->G1SR & ADC_G1SR_END) == 0UL)
    {
        spins++;
        if (spins >= ADC_HAL_TIMEOUT_SPINS)
        {
            return ADC_HAL_ERR_TIMEOUT;
        }
    }

    buf = adcREG1->G1BUF;

    if (((buf >> ADC_G1BUF_CHID_SHIFT) & ADC_G1BUF_CHID_MASK) != channel)
    {
        return ADC_HAL_ERR_CHID_MISMATCH;
    }

    *counts = (uint16_t)(buf & ADC_G1BUF_VALUE_MASK);
    return ADC_HAL_OK;
}
