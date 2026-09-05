/**
 * @file tms570_adc_regs.h
 * @brief Register overlay for the TMS570 ADC1 module (illustrative subset).
 *
 * This mirrors the style HALCoGen uses in reg_adc.h: a volatile struct laid over the
 * peripheral's base address. Only a handful of registers are modelled here and the
 * offsets are NOT authoritative - consult the TMS570LS3137 TRM / HALCoGen reg_adc.h
 * for the real map. The point of this file is the UNIT_TEST switch at the bottom.
 */
#ifndef TMS570_ADC_REGS_H
#define TMS570_ADC_REGS_H

#include <stdint.h>

typedef volatile struct adcBase
{
    uint32_t RSTCR;    /**< Reset control                                  */
    uint32_t OPMODECR; /**< Operating mode control (enable, 10/12-bit)     */
    uint32_t CLOCKCR;  /**< Clock prescaler                                */
    uint32_t G1MODECR; /**< Group 1 mode control (channel-ID in FIFO etc.) */
    uint32_t G1SEL;    /**< Group 1 channel select - writing starts conv.  */
    uint32_t G1SR;     /**< Group 1 status                                 */
    uint32_t G1BUF;    /**< Group 1 result FIFO read port                  */
} adcBASE_t;

/* Bit definitions (subset) */
#define ADC_RSTCR_RESET       (1UL << 0)
#define ADC_OPMODECR_ENABLE   (1UL << 0)
#define ADC_OPMODECR_12BIT    (1UL << 31)
#define ADC_G1MODECR_CHID     (1UL << 5)
#define ADC_G1SR_END          (1UL << 0)
#define ADC_G1BUF_VALUE_MASK  (0x0FFFUL)
#define ADC_G1BUF_CHID_SHIFT  (16U)
#define ADC_G1BUF_CHID_MASK   (0x1FUL)

#define ADC_NUM_CHANNELS      (24U)

/*
 * On the target the overlay sits on the real peripheral address. In a host unit test
 * the same name resolves to an ordinary RAM struct owned by the test, so the driver
 * code is compiled unchanged and the test can pre-load and inspect "registers".
 */
#ifdef UNIT_TEST
extern adcBASE_t adcREG1_fake;
#define adcREG1 (&adcREG1_fake)
#else
#define adcREG1 ((adcBASE_t *)0xFFF7C000U)
#endif

#endif /* TMS570_ADC_REGS_H */
