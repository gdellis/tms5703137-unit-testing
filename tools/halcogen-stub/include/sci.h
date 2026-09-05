/* Stand-in for HALCoGen's sci.h / reg_sci.h: the API target/unity_target_io.c uses,
 * with the real TMS570LS3137 base addresses. The implementation in ../source/sci.c is
 * a no-op. See ../README.md. */
#ifndef __SCI_H__
#define __SCI_H__

#include "sys_common.h"

/* Register overlay (offsets as in HALCoGen's reg_sci.h; only the layout matters here). */
typedef volatile struct sciBase
{
    uint32 GCR0;
    uint32 GCR1;
    uint32 GCR2;
    uint32 SETINT;
    uint32 CLEARINT;
    uint32 SETINTLVL;
    uint32 CLEARINTLVL;
    uint32 FLR;
    uint32 INTVECT0;
    uint32 INTVECT1;
    uint32 FORMAT;
    uint32 BRS;
    uint32 ED;
    uint32 RD;
    uint32 TD;
    uint32 PIO0;
    uint32 PIO1;
    uint32 PIO2;
    uint32 PIO3;
    uint32 PIO4;
    uint32 PIO5;
    uint32 PIO6;
    uint32 PIO7;
    uint32 PIO8;
} sciBASE_t;

#define sciREG    ((sciBASE_t *)0xFFF7E500U)   /* SCI2 in the TRM, "SCI" in HALCoGen    */
#define scilinREG ((sciBASE_t *)0xFFF7E400U)   /* SCI1/LIN,        "SCILIN" in HALCoGen */

void   sciInit(void);
void   sciSendByte(sciBASE_t *sci, uint8 byte);
uint32 sciIsTxReady(sciBASE_t *sci);

#endif /* __SCI_H__ */
