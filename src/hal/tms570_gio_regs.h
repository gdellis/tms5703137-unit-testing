/**
 * @file tms570_gio_regs.h
 * @brief Register overlay for the TMS570 GIO module (illustrative subset).
 *
 * Same idea as tms570_adc_regs.h: HALCoGen-style volatile structs over the peripheral
 * base addresses, with a UNIT_TEST switch that points them at RAM instead. Offsets
 * follow HALCoGen's reg_gio.h but are NOT authoritative - check the TRM.
 */
#ifndef TMS570_GIO_REGS_H
#define TMS570_GIO_REGS_H

#include <stdint.h>

/** Module-level registers (gioREG). */
typedef volatile struct gioBase
{
    uint32_t GCR0;     /**< Global control: bit 0 = 1 releases module reset */
    uint32_t rsvd1;
    uint32_t INTDET;   /**< Interrupt detect                                  */
    uint32_t POL;      /**< Interrupt polarity                                */
    uint32_t ENASET;   /**< Interrupt enable set                              */
    uint32_t ENACLR;   /**< Interrupt enable clear                            */
    uint32_t LVLSET;   /**< Interrupt level set                               */
    uint32_t LVLCLR;   /**< Interrupt level clear                             */
} gioBASE_t;

/** Per-port registers (gioPORTA, gioPORTB, ...). */
typedef volatile struct gioPort
{
    uint32_t DIR;      /**< Data direction: 1 = output                        */
    uint32_t DIN;      /**< Data input                                        */
    uint32_t DOUT;     /**< Data output                                       */
    uint32_t DSET;     /**< Data set:   writing 1 sets the output bit         */
    uint32_t DCLR;     /**< Data clear: writing 1 clears the output bit       */
    uint32_t PDR;      /**< Open-drain enable                                 */
    uint32_t PULDIS;   /**< Pull disable                                      */
    uint32_t PSL;      /**< Pull select                                       */
} gioPORT_t;

#define GIO_GCR0_RESET_RELEASE  (1UL << 0)
#define GIO_PORT_ALL_PINS       (0xFFUL)

#ifdef UNIT_TEST
extern gioBASE_t gioREG_fake;
extern gioPORT_t gioPORTA_fake;
#define gioREG   (&gioREG_fake)
#define gioPORTA (&gioPORTA_fake)
#else
#define gioREG   ((gioBASE_t *)0xFFF7BC00U)
#define gioPORTA ((gioPORT_t *)0xFFF7BC34U)
#endif

#endif /* TMS570_GIO_REGS_H */
